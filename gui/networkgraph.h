#ifndef NETWORK_GRAPH_H
#define NETWORK_GRAPH_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

class Node;
class Edge;

//class for manage graph construction
class NetworkGraph : public QGraphicsView
{
public:
    NetworkGraph();
    void CreateNodeAtPosition( QPointF pos );

protected:

protected:
    QGraphicsScene* mScene;

    std::list<Node*> mNodes;
    std::list<Edge*> mEdges;

    bool bNewEdgeIsCreating = false;

    /////Timer staff for animation purpose
    QTimer* mTimer;
};

#endif
