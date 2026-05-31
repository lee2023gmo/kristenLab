#include "ball.h"
#include "constants.h"

Ball::Ball()
    : item(nullptr)
    , position(0, 0)
    , radius(BALL_RADIUS)
    , collisionHalfSize(BALL_RADIUS)
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
        item->setPos(position);
    }
}

void Ball::setPixmap(const QPixmap &pixmap)
{
    if (item != nullptr) {
        item->setPixmap(pixmap);
        item->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    }
}

QRectF Ball::collisionRect() const
{
    return QRectF(
        position.x() - collisionHalfSize,
        position.y() - collisionHalfSize,
        collisionHalfSize * 2.0,
        collisionHalfSize * 2.0
    );
}

QRectF Ball::collisionRectAt(const QPointF &pos) const
{
    return QRectF(
        pos.x() - collisionHalfSize,
        pos.y() - collisionHalfSize,
        collisionHalfSize * 2.0,
        collisionHalfSize * 2.0
    );
}
