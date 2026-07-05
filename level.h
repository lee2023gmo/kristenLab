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
    QString sourceFilePath;

    Level();

    Level(const QString &levelName,
          const QStringList &levelMapData,
          int targetCount,
          const QVector<QPoint> &levelEditablePoints = QVector<QPoint>(),
          bool customLevel = false,
          const QString &sourcePath = QString());
};

#endif // LEVEL_H

