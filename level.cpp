#include "level.h"

Level::Level()
    : name("")
    , targetReverseCount(0)
    , isCustomLevel(false)
    , sourceFilePath("")
{
}

Level::Level(const QString &levelName,
             const QStringList &levelMapData,
             int targetCount,
             const QVector<QPoint> &levelEditablePoints,
             bool customLevel,
             const QString &sourcePath)
    : name(levelName)
    , mapData(levelMapData)
    , targetReverseCount(targetCount)
    , editablePoints(levelEditablePoints)
    , isCustomLevel(customLevel)
    , sourceFilePath(sourcePath)
{
}

