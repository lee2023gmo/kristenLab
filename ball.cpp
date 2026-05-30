#include "ball.h"
#include "constants.h"

Ball::Ball()
    : item(nullptr)
    , position(0, 0)
    , radius(BALL_RADIUS)
{
}

void Ball::moveBy(double dx, double dy)
{
    setPosition(QPointF(position.x() + dx, position.y() + dy));
}

void Ball::setPosition(const QPointF &newPosition)
{
    position = newPosition;

    if (item != nullptr) {
        item->setRect(
            position.x() - radius,
            position.y() - radius,
            radius * 2,
            radius * 2
            );
    }
}