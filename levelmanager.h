#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include <QVector>
#include <QString>

#include "level.h"

class LevelManager
{
public:
    LevelManager();

    void loadDefaultLevels();

    int levelCount() const;
    bool isValidLevelIndex(int index) const;
    Level levelAt(int index) const;

    bool loadLevelFromFile(const QString &filePath, bool isCustomLevel);
    int loadLevelsFromFolder(const QString &folderPath, bool isCustomLevel);


    bool readLevelFromFile(const QString &filePath,
                           Level *level,
                           QString *errorMessage = nullptr) const;

    bool saveLevelToFile(const Level &level,
                         const QString &filePath,
                         QString *errorMessage = nullptr) const;

    bool validateLevelForSave(const Level &level,
                              QString *errorMessage = nullptr) const;

    QString customLevelFolderPath() const;

    QString customLevelFilePathForName(const QString &levelName) const;
    bool deleteCustomLevelFile(const Level &level,
                               QString *errorMessage = nullptr) const;

    bool isCustomLevelIndex(int index) const;
    QString levelSelectTextAt(int index) const;
private:
    QVector<Level> levels;

    bool validateLevel(const Level &level, QString *errorMessage = nullptr) const;
    void addLevelIfValid(const Level &level);

    QString levelFolderPath(const QString &folderName) const;
    void addFallbackLevel();

    bool isFileInFolder(const QString &filePath, const QString &folderPath) const;
    QString safeLevelFileName(const QString &name) const;
};

#endif // LEVELMANAGER_H

