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
    // QPoint(x, y) 中 x 是列 col，y 是行 row
    QVector<QPoint> editablePoints;

    Level();

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount);

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount,
          const QVector<QPoint> &levelEditablePoints);
};

#endif // LEVEL_H
