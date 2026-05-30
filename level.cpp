#include "level.h"

Level::Level()
    : name("")
    , targetReverseCount(0)
{
}

Level::Level(const QString &levelName,
             const QStringList &levelMapData,
             int targetCount)
    : name(levelName)
    , mapData(levelMapData)
    , targetReverseCount(targetCount)
{
}