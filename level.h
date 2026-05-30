#ifndef LEVEL_H
#define LEVEL_H

#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>

class Level
{
public:
    QString name;
    QStringList mapData;
    int targetReverseCount;

    // 阶段 20：固定候选编辑点
    QVector<QPoint> editablePoints;

    // 阶段 22：区分内置关卡和自定义关卡
    bool isCustomLevel;

    Level();

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount);

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount,
          const QVector<QPoint> &levelEditablePoints);

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount,
          const QVector<QPoint> &levelEditablePoints,
          bool customLevel);
};

#endif // LEVEL_H
