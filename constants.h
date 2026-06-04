#ifndef CONSTANTS_H
#define CONSTANTS_H

const int TILE_SIZE = 40;
const int BALL_RADIUS = 12;

const int BALL_SPEED = 3;
const int SLOW_SPEED = 1;
const int CONVEYOR_SPEED = 2;

const int DATA_FRAGMENT_RADIUS = 7;

const int TIMER_INTERVAL = 20;

// 激光门：亮时阻挡并弹回角色，灭时可通行。
const int LASER_ACTIVE_MS = 1200;
const int LASER_INACTIVE_MS = 3000;

#endif // CONSTANTS_H
