#ifndef BALL_H
#define BALL_H

#include <QGraphicsEllipseItem>
#include <QPointF>

class Ball
{
public:
    QGraphicsEllipseItem *item;
    QPointF position;
    int radius;

    Ball();

    void moveBy(double dx, double dy);
    void setPosition(const QPointF &newPosition);
};

#endif // BALL_H