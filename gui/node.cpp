#include "node.h"

Node::Node( int id, double x, double y)
{
    this->id = id;
    this->x = x;
    this->y = y;
}

void Node::set_xy(double x, double y)
{
    this->x = x;
    this->y = y;
}

double Node::get_x()
{
    return this->x;
}

double Node::get_y()
{
    return this->y;
}

int Node::get_id()
{
    return this->id;
}
