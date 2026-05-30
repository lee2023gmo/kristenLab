#include "level.h"

Level::Level()
    : name("")
    , targetReverseCount(0)
    , isCustomLevel(false)
{
}

Level::Level(const QString &levelName,
             const QStringList &levelMapData,
             int targetCount)
    : name(levelName)
    , mapData(levelMapData)
    , targetReverseCount(targetCount)
    , isCustomLevel(false)
{
}

Level::Level(const QString &levelName,
             const QStringList &levelMapData,
             int targetCount,
             const QVector<QPoint> &levelEditablePoints)
    : name(levelName)
    , mapData(levelMapData)
    , targetReverseCount(targetCount)
    , editablePoints(levelEditablePoints)
    , isCustomLevel(false)
{
}

Level::Level(const QString &levelName,
             const QStringList &levelMapData,
             int targetCount,
             const QVector<QPoint> &levelEditablePoints,
             bool customLevel)
    : name(levelName)
    , mapData(levelMapData)
    , targetReverseCount(targetCount)
    , editablePoints(levelEditablePoints)
    , isCustomLevel(customLevel)
{
}
