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
#include <QSet>
#include <QSaveFile>

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

    // 阶段 20：候选编辑点校验
    QSet<QString> usedEditablePointKeys;

    for (const QPoint &point : level.editablePoints) {
        int col = point.x();
        int row = point.y();

        if (row < 0 || row >= level.mapData.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("候选编辑点行号越界：row=%1 col=%2")
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        if (col < 0 || col >= level.mapData[row].size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("候选编辑点列号越界：row=%1 col=%2")
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        QString key = QString("%1,%2").arg(col).arg(row);

        if (usedEditablePointKeys.contains(key)) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("候选编辑点重复：row=%1 col=%2")
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        usedEditablePointKeys.insert(key);

        QChar tile = level.mapData[row][col];

        // 候选点不能放在关键结构上
        if (TileDefs::isWall(tile)
            || TileDefs::isStart(tile)
            || TileDefs::isEnd(tile)
            || TileDefs::isDeath(tile)
            || TileDefs::isData(tile)) {

            if (errorMessage != nullptr) {
                *errorMessage = QString("候选编辑点不能放在 %1 上：row=%2 col=%3")
                                    .arg(TileDefs::nameOf(tile))
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }
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
QString LevelManager::customLevelFolderPath() const
{
    return levelFolderPath("custom_levels");
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
    Level level;
    QString errorMessage;

    if (!readLevelFromFile(filePath, &level, &errorMessage)) {
        qWarning() << "Invalid external level:" << filePath << errorMessage;
        return false;
    }

    levels.append(level);

    qDebug() << "External level loaded:" << level.name << "from" << filePath
             << "editable points:" << level.editablePoints.size();

    return true;
}
bool LevelManager::validateLevelForSave(const Level &level,
                                        QString *errorMessage) const
{
    return validateLevel(level, errorMessage);
}
void LevelManager::addFallbackLevel()
{
    QVector<QPoint> editablePoints;

    editablePoints.append(QPoint(3, 1));
    editablePoints.append(QPoint(5, 2));
    editablePoints.append(QPoint(8, 3));

    addLevelIfValid(Level(
        "备用关卡",
        {
            "111111111111",
            "120000000001",
            "100000000031",
            "100000040001",
            "111111111111"
        },
        6,
        editablePoints
        ));
}
bool LevelManager::readLevelFromFile(const QString &filePath,
                                     Level *level,
                                     QString *errorMessage) const
{
    if (level == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = "读取关卡失败：level 指针为空";
        }
        return false;
    }

    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("无法打开文件：%1").arg(file.errorString());
        }
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("JSON 解析失败：%1").arg(parseError.errorString());
        }
        return false;
    }

    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = "关卡文件根节点必须是 JSON 对象";
        }
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
        if (errorMessage != nullptr) {
            *errorMessage = "关卡文件缺少 map 数组";
        }
        return false;
    }

    QStringList mapData;

    for (const QJsonValue &value : mapArray) {
        if (!value.isString()) {
            if (errorMessage != nullptr) {
                *errorMessage = "map 数组中的每一行都必须是字符串";
            }
            return false;
        }

        mapData.append(value.toString());
    }

    QVector<QPoint> editablePoints;

    QJsonValue editablePointsValue = object.value("editablePoints");

    if (!editablePointsValue.isUndefined()) {
        if (!editablePointsValue.isArray()) {
            if (errorMessage != nullptr) {
                *errorMessage = "editablePoints 必须是数组";
            }
            return false;
        }

        QJsonArray pointsArray = editablePointsValue.toArray();

        for (const QJsonValue &pointValue : pointsArray) {
            if (pointValue.isObject()) {
                QJsonObject pointObject = pointValue.toObject();

                int row = pointObject.value("row").toInt(-1);
                int col = pointObject.value("col").toInt(-1);

                editablePoints.append(QPoint(col, row));
            }
            else if (pointValue.isArray()) {
                QJsonArray pointArray = pointValue.toArray();

                if (pointArray.size() != 2) {
                    if (errorMessage != nullptr) {
                        *errorMessage = "editablePoints 简写格式必须是 [col, row]";
                    }
                    return false;
                }

                int col = pointArray[0].toInt(-1);
                int row = pointArray[1].toInt(-1);

                editablePoints.append(QPoint(col, row));
            }
            else {
                if (errorMessage != nullptr) {
                    *errorMessage = "editablePoints 中存在非法候选点格式";
                }
                return false;
            }
        }
    }

    Level loadedLevel(
        name,
        mapData,
        targetReverseCount,
        editablePoints
        );

    if (!validateLevel(loadedLevel, errorMessage)) {
        return false;
    }

    *level = loadedLevel;
    return true;
}
bool LevelManager::saveLevelToFile(const Level &level,
                                   const QString &filePath,
                                   QString *errorMessage) const
{
    QString validateError;

    if (!validateLevel(level, &validateError)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("地图不合法，拒绝保存：%1").arg(validateError);
        }
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString folderPath = fileInfo.absolutePath();

    if (!QDir().mkpath(folderPath)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("无法创建保存目录：%1").arg(folderPath);
        }
        return false;
    }

    QJsonObject object;

    object["name"] = level.name;
    object["targetReverseCount"] = level.targetReverseCount;

    QJsonArray mapArray;

    for (const QString &line : level.mapData) {
        mapArray.append(line);
    }

    object["map"] = mapArray;

    // 阶段 20 的候选点也一起保存，方便以后重新编辑。
    // 阶段 21 要求的核心内容仍然是 name / targetReverseCount / map。
    if (!level.editablePoints.isEmpty()) {
        QJsonArray editablePointsArray;

        for (const QPoint &point : level.editablePoints) {
            QJsonObject pointObject;
            pointObject["row"] = point.y();
            pointObject["col"] = point.x();

            editablePointsArray.append(pointObject);
        }

        object["editablePoints"] = editablePointsArray;
    }

    QJsonDocument document(object);

    QSaveFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("无法写入文件：%1").arg(file.errorString());
        }
        return false;
    }

    file.write(document.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("保存文件失败：%1").arg(file.errorString());
        }
        return false;
    }

    return true;
}
