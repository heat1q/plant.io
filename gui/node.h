#ifndef NODE_H
#define NODE_H

#include <functional>
#include <QGraphicsItem>

class Node : public QGraphicsItem
{
public:
    Node( int number, QGraphicsItem* parent = nullptr );

    bool GetNodeIsAboutToTerminate() const { return bIsAboutToTerminate; }
    void SetNodeIsAboutToTerimate( bool in ) { bIsAboutToTerminate = in; }

    QPointF GetCenterPosition() const;
    bool GetIsPressed() const
    {
        return bIsPressed;
    }

protected:
    virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget ) override;
    virtual QRectF boundingRect() const override;

    virtual void mousePressEvent( QGraphicsSceneMouseEvent * event ) override;
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event ) override;
    virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event ) override;

    virtual void advance( int phase ) override;

private:
    //animation states
    void Start01();
    void Start02();
    void End01();

protected:
    const int mNumber;
    bool bIsPressed;

    //Flag to play dying animation and to any observers it is going to terminate itself
    bool bIsAboutToTerminate;

    float mScale;

    std::function<void ()> mAnimationState;

};


#endif //NODE_H
