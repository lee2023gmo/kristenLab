#ifndef BALL_H
#define BALL_H

#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QRectF>

class Ball
{
public:
    QGraphicsPixmapItem *item;
    QPointF position;
    int radius;
    int collisionHalfSize;   // 方形碰撞体的半边长

    Ball();

    void moveBy(double dx, double dy);
    void setPosition(const QPointF &newPosition);
    void setPixmap(const QPixmap &pixmap);

    QRectF collisionRect() const;
    QRectF collisionRectAt(const QPointF &pos) const;
};

#endif // BALL_H
