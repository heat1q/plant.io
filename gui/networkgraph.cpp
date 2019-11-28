#include "networkgraph.h"
#include "node.h"
#include "edge.h"
#include <QTimer>

NetworkGraph::NetworkGraph() : QGraphicsView()
{
    mTimer = new QTimer( mScene );
    QGraphicsScene::connect( mTimer, SIGNAL( timeout() ), mScene, SLOT( advance() ) );
    mTimer->start( 10 );
}

void NetworkGraph::CreateNodeAtPosition( QPointF pos )
{
    mNodes.push_back( new Node( mNodes.size() + 1 ) );
    mNodes.back()->setPos( pos );
    mScene->addItem( mNodes.back() );
}
