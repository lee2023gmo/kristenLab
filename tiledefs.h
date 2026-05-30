#ifndef TILEDEFS_H
#define TILEDEFS_H

#include <QChar>
#include <QString>

namespace TileDefs
{
// 基础地图元素
static const QChar Empty = QChar('0');        // 空地
static const QChar Wall = QChar('1');         // 墙体
static const QChar Start = QChar('2');        // 起点
static const QChar End = QChar('3');          // 终点
static const QChar Death = QChar('4');        // 死亡区

// 机关 / 道具
static const QChar Bounce = QChar('5');       // 弹射块
static const QChar Slow = QChar('6');         // 缓冲区
static const QChar Conveyor = QChar('7');     // 传送带
static const QChar Data = QChar('8');         // 数据碎片

// 以后可以继续加
static const QChar Key = QChar('9');          // 钥匙
static const QChar Door = QChar('A');         // 门
static const QChar Portal = QChar('B');       // 传送门
static const QChar SpeedUp = QChar('C');      // 加速区
static const QChar Reverse = QChar('D');      // 反向区
static const QChar Switch = QChar('E');       // 开关

inline bool isWall(QChar tile)
{
    return tile == Wall;
}

inline bool isEmpty(QChar tile)
{
    return tile == Empty;
}

inline bool isStart(QChar tile)
{
    return tile == Start;
}

inline bool isEnd(QChar tile)
{
    return tile == End;
}

inline bool isDeath(QChar tile)
{
    return tile == Death;
}

inline bool isBounce(QChar tile)
{
    return tile == Bounce;
}

inline bool isSlow(QChar tile)
{
    return tile == Slow;
}

inline bool isConveyor(QChar tile)
{
    return tile == Conveyor;
}

inline bool isData(QChar tile)
{
    return tile == Data;
}

inline QString nameOf(QChar tile)
{
    if (tile == Empty) {
        return "空地";
    }
    if (tile == Wall) {
        return "墙体";
    }
    if (tile == Start) {
        return "起点";
    }
    if (tile == End) {
        return "终点";
    }
    if (tile == Death) {
        return "死亡区";
    }
    if (tile == Bounce) {
        return "弹射块";
    }
    if (tile == Slow) {
        return "缓冲区";
    }
    if (tile == Conveyor) {
        return "传送带";
    }
    if (tile == Data) {
        return "数据碎片";
    }

    return "未知元素";
}
inline bool isKnownTile(QChar tile)
{
    return tile == Empty
           || tile == Wall
           || tile == Start
           || tile == End
           || tile == Death
           || tile == Bounce
           || tile == Slow
           || tile == Conveyor
           || tile == Data;
}

}

#endif // TILEDEFS_H
