#ifndef LEVEL_H
#define LEVEL_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPoint>

class Level
{
public:
    QString name;
    QStringList mapData;
    int targetReverseCount;
    QVector<QPoint> editablePoints;

    bool isCustomLevel;

    Level();

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount,
          const QVector<QPoint> &levelEditablePoints = QVector<QPoint>(),
          bool customLevel = false);
};

#endif // LEVEL_H

