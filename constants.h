#ifndef CONSTANTS_H
#define CONSTANTS_H

const int TILE_SIZE = 40;
const int BALL_RADIUS = 12;

const int BALL_SPEED = 3;
const int SLOW_SPEED = 1;
const int CONVEYOR_SPEED = 2;

const int DATA_FRAGMENT_RADIUS = 7;

const int TIMER_INTERVAL = 20;

// 激光门周期：亮 1.2 秒，灭 3.0 秒。
// 亮的时候等同于临时墙体并把角色弹回；灭的时候完全不阻挡，且开门时间更长。
const int LASER_ACTIVE_MS = 1200;
const int LASER_INACTIVE_MS = 3000;

#endif // CONSTANTS_H