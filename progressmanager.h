#ifndef PROGRESSMANAGER_H
#define PROGRESSMANAGER_H

#include <QHash>
#include <QJsonObject>
#include <QString>

#include "level.h"

class ProgressManager
{
public:
    struct Record {
        bool completed = false;
        int bestStars = 0;
        int bestTimeMs = -1;
        int bestReverseCount = -1;
        int bestDeathCount = -1;
        QString lastCompletedAt;
    };

    ProgressManager();

    void load();
    bool save(QString *errorMessage = nullptr) const;
    bool reset(QString *errorMessage = nullptr);

    bool isBuiltInLevelUnlocked(int zeroBasedLevelIndex) const;
    int highestUnlockedBuiltInLevelNumber() const;

    bool hasRecordForLevel(const Level &level) const;
    Record recordForLevel(const Level &level) const;

    bool updateAfterCompletion(const Level &level,
                               int zeroBasedLevelIndex,
                               int stars,
                               int elapsedMs,
                               int reverseCount,
                               int deathCount,
                               QString *errorMessage = nullptr);

    QString progressTextForLevel(const Level &level,
                                 int zeroBasedLevelIndex) const;

    static QString levelKey(const Level &level);
    static QString starText(int stars);
    static QString formatTimeMs(int ms);
    static QString progressFilePath();
    static QString progressFolderPath();

private:
    QHash<QString, Record> records;
    int highestUnlockedBuiltIn;

    static QJsonObject recordToJson(const Record &record);
    static Record recordFromJson(const QJsonObject &object);
};

#endif // PROGRESSMANAGER_H
