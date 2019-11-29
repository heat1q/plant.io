#include "networkgraph.h"

NetworkGraph::NetworkGraph() : QGraphicsView()
{
    mTimer = new QTimer( mScene );
    QGraphicsScene::connect( mTimer, SIGNAL( timeout() ), mScene, SLOT( advance() ) );
    mTimer->start( 10 );
}

void NetworkGraph::CreateNodeAtPosition( QPointF pos )
{
}
