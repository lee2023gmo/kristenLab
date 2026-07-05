#include "levelmanager.h"
#include "levelvalidator.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
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

    int builtInCount = loadLevelsFromFolder(builtInFolder, false);

    if (builtInCount == 0) {
        qWarning() << "No built-in levels found. Loading fallback built-in level.";
        addFallbackLevel();
        builtInCount = 1;
    }

    int customCount = loadLevelsFromFolder(customFolder, true);

    qDebug() << "Built-in levels loaded:" << builtInCount;
    qDebug() << "Custom levels loaded:" << customCount;
    qDebug() << "Total valid levels:" << levels.size();
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
bool LevelManager::isCustomLevelIndex(int index) const
{
    if (!isValidLevelIndex(index)) {
        return false;
    }

    return levels[index].isCustomLevel;
}

QString LevelManager::levelSelectTextAt(int index) const
{
    if (!isValidLevelIndex(index)) {
        return QString("未知关卡");
    }

    const Level &level = levels[index];

    if (level.isCustomLevel) {
        return QString("[自定义] %1").arg(level.name);
    }

    return QString("第 %1 关：%2")
        .arg(index + 1)
        .arg(level.name);
}


void LevelManager::addLevelIfValid(const Level &level)
{
    QString errorMessage;

    if (!LevelValidator::validateLevel(level, &errorMessage, LevelValidator::runtimeOptions())) {
        qWarning() << "Level rejected:" << level.name << errorMessage;
        return;
    }

    levels.append(level);
    qDebug() << "Level loaded:" << level.name << "isCustomLevel:" << level.isCustomLevel;
}



QString LevelManager::levelFolderPath(const QString &folderName) const
{
    QDir dir(QCoreApplication::applicationDirPath());

    // 与关卡设计师保存路径保持一致：
    // 从 build/Desktop_Qt_xxx-Debug 回到项目根目录，再读取 levels / custom_levels。
    QString currentFolderName = dir.dirName();

    if (currentFolderName.startsWith("Desktop_", Qt::CaseInsensitive)
        || currentFolderName.contains("Qt", Qt::CaseInsensitive)
        || currentFolderName.contains("Debug", Qt::CaseInsensitive)
        || currentFolderName.contains("Release", Qt::CaseInsensitive)) {
        dir.cdUp();
    }

    if (dir.dirName().compare("build", Qt::CaseInsensitive) == 0) {
        dir.cdUp();
    }

    return dir.filePath(folderName);
}
bool LevelManager::isFileInFolder(const QString &filePath, const QString &folderPath) const
{
    QString absoluteFilePath = QDir::fromNativeSeparators(
        QDir::cleanPath(QFileInfo(filePath).absoluteFilePath())
        );

    QString absoluteFolderPath = QDir::fromNativeSeparators(
        QDir::cleanPath(QDir(folderPath).absolutePath())
        );

    if (absoluteFilePath == absoluteFolderPath) {
        return true;
    }

    return absoluteFilePath.startsWith(absoluteFolderPath + "/");
}

QString LevelManager::customLevelFolderPath() const
{
    return levelFolderPath("custom_levels");
}
int LevelManager::loadLevelsFromFolder(const QString &folderPath, bool isCustomLevel)
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
        if (loadLevelFromFile(fileInfo.absoluteFilePath(), isCustomLevel)) {
            loadedCount++;
        }
    }

    return loadedCount;
}

bool LevelManager::loadLevelFromFile(const QString &filePath, bool isCustomLevel)
{
    Level level;
    QString errorMessage;

    if (!readLevelFromFile(filePath, &level, &errorMessage)) {
        qWarning() << "Invalid external level:" << filePath << errorMessage;
        return false;
    }

    // 这里必须使用调用者传进来的来源标记。
    // builtInFolder 调用时传 false，customFolder 调用时传 true。
    // 这样关卡选择界面才能稳定分成“内置关卡 / 自定义关卡”。
    level.isCustomLevel = isCustomLevel;

    levels.append(level);

    qDebug() << "Level loaded:"
             << level.name
             << "from"
             << filePath
             << "isCustomLevel:"
             << level.isCustomLevel;

    return true;
}
bool LevelManager::validateLevelForSave(const Level &level,
                                        QString *errorMessage) const
{
    return LevelValidator::validateLevel(level, errorMessage, LevelValidator::runtimeOptions());
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
        editablePoints,
        false
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

    bool customLevel = isFileInFolder(filePath, customLevelFolderPath());

    Level loadedLevel(
        name,
        mapData,
        targetReverseCount,
        editablePoints,
        customLevel
        );

    if (!LevelValidator::validateLevel(loadedLevel, errorMessage, LevelValidator::runtimeOptions())) {
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

    if (!LevelValidator::validateLevel(level, &validateError, LevelValidator::runtimeOptions())) {
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
