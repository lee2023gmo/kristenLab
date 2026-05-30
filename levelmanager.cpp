#include "levelmanager.h"
#include "tiledefs.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

LevelManager::LevelManager()
{
}

void LevelManager::loadDefaultLevels()
{
    levels.clear();

    QString builtInFolder = levelFolderPath("levels");
    QString customFolder = levelFolderPath("custom_levels");

    QDir().mkpath(builtInFolder);
    QDir().mkpath(customFolder);

    qDebug() << "Built-in levels folder:" << builtInFolder;
    qDebug() << "Custom levels folder:" << customFolder;

    int builtInCount = loadLevelsFromFolder(builtInFolder);
    int customCount = loadLevelsFromFolder(customFolder);

    qDebug() << "Built-in levels loaded:" << builtInCount;
    qDebug() << "Custom levels loaded:" << customCount;
    qDebug() << "Total valid levels:" << levels.size();

    if (levels.isEmpty()) {
        qWarning() << "No external valid levels found. Loading fallback level.";
        addFallbackLevel();
    }
}

int LevelManager::levelCount() const
{
    return levels.size();
}

bool LevelManager::isValidLevelIndex(int index) const
{
    return index >= 0 && index < levels.size();
}

Level LevelManager::levelAt(int index) const
{
    if (!isValidLevelIndex(index)) {
        return Level();
    }

    return levels[index];
}


void LevelManager::addLevelIfValid(const Level &level)
{
    QString errorMessage;

    if (!validateLevel(level, &errorMessage)) {
        qWarning() << "Level rejected:" << level.name << errorMessage;
        return;
    }

    levels.append(level);
    qDebug() << "Level loaded:" << level.name;
}


bool LevelManager::validateLevel(const Level &level, QString *errorMessage) const
{
    if (level.mapData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图不能为空";
        }
        return false;
    }

    int expectedColumnCount = level.mapData[0].size();

    if (expectedColumnCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图第一行不能为空";
        }
        return false;
    }

    int startCount = 0;
    int endCount = 0;

    for (int row = 0; row < level.mapData.size(); ++row) {
        QString line = level.mapData[row];

        if (line.size() != expectedColumnCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("第 %1 行长度不一致，应该是 %2，实际是 %3")
                                    .arg(row + 1)
                                    .arg(expectedColumnCount)
                                    .arg(line.size());
            }
            return false;
        }

        for (int col = 0; col < line.size(); ++col) {
            QChar tile = line[col];

            if (!TileDefs::isKnownTile(tile)) {
                if (errorMessage != nullptr) {
                    *errorMessage = QString("第 %1 行第 %2 列出现非法字符：%3")
                                        .arg(row + 1)
                                        .arg(col + 1)
                                        .arg(tile);
                }
                return false;
            }

            if (TileDefs::isStart(tile)) {
                startCount++;
            }

            if (TileDefs::isEnd(tile)) {
                endCount++;
            }
        }
    }

    if (startCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图缺少起点 2";
        }
        return false;
    }

    if (startCount > 1) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("地图只能有一个起点 2，但当前有 %1 个").arg(startCount);
        }
        return false;
    }

    if (endCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图至少需要一个终点 3";
        }
        return false;
    }

    return true;
}

QString LevelManager::levelFolderPath(const QString &folderName) const
{
    QStringList candidatePaths;

    QDir appDir(QCoreApplication::applicationDirPath());
    candidatePaths.append(appDir.filePath(folderName));

    QDir currentDir(QDir::currentPath());
    candidatePaths.append(currentDir.filePath(folderName));

    QDir searchDir(QCoreApplication::applicationDirPath());

    for (int i = 0; i < 8; ++i) {
        candidatePaths.append(searchDir.filePath(folderName));

        if (!searchDir.cdUp()) {
            break;
        }
    }

    QString firstExistingPath;

    for (const QString &path : candidatePaths) {
        QString cleanPath = QDir::cleanPath(path);
        QDir dir(cleanPath);

        if (!dir.exists()) {
            continue;
        }

        if (firstExistingPath.isEmpty()) {
            firstExistingPath = cleanPath;
        }

        QStringList filters;
        filters << "*.json";

        QFileInfoList jsonFiles = dir.entryInfoList(
            filters,
            QDir::Files,
            QDir::Name
            );

        if (!jsonFiles.isEmpty()) {
            return cleanPath;
        }
    }

    if (!firstExistingPath.isEmpty()) {
        return firstExistingPath;
    }

    return appDir.filePath(folderName);
}

int LevelManager::loadLevelsFromFolder(const QString &folderPath)
{
    QDir dir(folderPath);

    if (!dir.exists()) {
        qWarning() << "Level folder does not exist:" << folderPath;
        return 0;
    }

    QStringList filters;
    filters << "*.json";

    QFileInfoList fileList = dir.entryInfoList(
        filters,
        QDir::Files,
        QDir::Name
        );

    int loadedCount = 0;

    for (const QFileInfo &fileInfo : fileList) {
        if (loadLevelFromFile(fileInfo.absoluteFilePath())) {
            loadedCount++;
        }
    }

    return loadedCount;
}

bool LevelManager::loadLevelFromFile(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open level file:" << filePath;
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << filePath << parseError.errorString();
        return false;
    }

    if (!document.isObject()) {
        qWarning() << "Level file root is not an object:" << filePath;
        return false;
    }

    QJsonObject object = document.object();

    QString name = object.value("name").toString();

    if (name.isEmpty()) {
        name = QFileInfo(filePath).baseName();
    }

    int targetReverseCount = object.value("targetReverseCount").toInt(0);

    QJsonArray mapArray = object.value("map").toArray();

    if (mapArray.isEmpty()) {
        qWarning() << "Level file has no map array:" << filePath;
        return false;
    }

    QStringList mapData;

    for (const QJsonValue &value : mapArray) {
        if (!value.isString()) {
            qWarning() << "Map row is not string:" << filePath;
            return false;
        }

        mapData.append(value.toString());
    }

    Level level(
        name,
        mapData,
        targetReverseCount
        );

    QString errorMessage;

    if (!validateLevel(level, &errorMessage)) {
        qWarning() << "Invalid external level:" << filePath << errorMessage;
        return false;
    }

    levels.append(level);

    qDebug() << "External level loaded:" << name << "from" << filePath;

    return true;
}

void LevelManager::addFallbackLevel()
{
    addLevelIfValid(Level(
        "备用关卡",
        {
            "111111111111",
            "120000000001",
            "100000000031",
            "100000040001",
            "111111111111"
        },
        6
        ));
}