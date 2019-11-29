#ifndef NETWORK_GRAPH_H
#define NETWORK_GRAPH_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTimer>
#include "node.h"
#include "edge.h"

class Node;
class Edge;

//class for manage graph construction
class NetworkGraph : public QGraphicsView
{
public:
    NetworkGraph();
    void CreateNodeAtPosition( QPointF pos );

private:
    static double node_pos [20][2]; // Node_ID: y-axis // [1,:] xpos // [2,:] ypos

protected:
    QGraphicsScene* mScene;

    std::list<Node*> mNodes;
    std::list<Edge*> mEdges;

    bool bNewEdgeIsCreating = false;

    /////Timer staff for animation purpose
    QTimer* mTimer;
};

#endif
