#include "node.h"
#include <QtGui>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

Node::Node( int number, QGraphicsItem* parent )
    :
    QGraphicsItem( parent ),
    mNumber( number ),
    bIsPressed( false ),
    bIsAboutToTerminate( false ),
    mScale( 0.1f ),
    mAnimationState( nullptr )
{
    setFlag( QGraphicsItem::ItemIsMovable );
    mAnimationState = std::bind( &Node::Start01, this );
}

QPointF Node::GetCenterPosition() const
{
    return pos() + boundingRect().center();

}

void Node::paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget )
{
    //get node's bounding rectangle
    QRectF rect = boundingRect();

    //infrastructure for shape
    QPen grayPen( Qt::GlobalColor::gray );
    grayPen.setWidth( 6 );
    QPen blackPen( Qt::GlobalColor::black );
    blackPen.setWidth( 6 );

    if( bIsPressed )
    {
        painter->setPen( blackPen );
    }
    else
    {
        painter->setPen( grayPen );
    }

    //draw shape
    painter->drawEllipse( rect );

    //draw texture
    QPixmap texture = QPixmap( "Node50x50.png" );
    painter->drawPixmap( 0, 0, 50, 50, texture );

    //draw number
    QFont font;
    font.setWeight( QFont::Bold );
    painter->setFont( font );
    painter->drawText ( rect, Qt::AlignCenter | Qt::AlignVCenter, QString::number( mNumber ) );

    update();

    setScale( mScale );
}
QRectF Node::boundingRect() const
{
    return QRectF( 0, 0, 50, 50 );
}

void Node::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    bIsPressed = true;
    update();
    QGraphicsItem::mousePressEvent( event );
}
void Node::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
    bIsPressed = false;
    update();
    QGraphicsItem::mouseReleaseEvent( event );
}
void Node::mouseMoveEvent( QGraphicsSceneMouseEvent * event )
{
    update();
    QGraphicsItem::mouseMoveEvent( event );
}

void Node::advance( int phase )
{
    if( mAnimationState )
    {
        mAnimationState();
    }

    if( GetNodeIsAboutToTerminate() )
    {
        mAnimationState = std::bind( &Node::End01, this );
    }

    update();
}

void Node::Start01()
{
    mScale += 0.1f;

    if( mScale > 1.5f )
    {
        mAnimationState = std::bind( &Node::Start02, this );
    }
}
void Node::Start02()
{
    mScale -= 0.1f;

    if( mScale < 1.1f )
    {
        mAnimationState = nullptr;
    }
}
void Node::End01()
{
    mScale -= 0.1f;

    setPos( pos() + QPointF( 4, 4 ) * mScale );
    if( mScale < 0.3 )
    {
        //Legal way to remove item from scene by itself
        this->scene()->removeItem( this );

    }
}
