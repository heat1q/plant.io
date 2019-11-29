#ifndef NODE_H
#define NODE_H

#include <functional>
#include <QGraphicsItem>
#include <QtGui>
#include <QGraphicsScene>

class Node
{
public:
    Node( int id, double x, double y );
    void set_xy(double x, double y);
    double get_x();
    double get_y();
    int get_id();

private:
    int id;
    double x;
    double y;
};

#endif //NODE_H
