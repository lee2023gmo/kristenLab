#include "progressmanager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

ProgressManager::ProgressManager()
    : highestUnlockedBuiltIn(1)
{
    load();
}

void ProgressManager::load()
{
    records.clear();
    highestUnlockedBuiltIn = 1;

    QFile file(progressFilePath());
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open progress file:" << file.errorString();
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "Invalid progress file:" << parseError.errorString();
        return;
    }

    QJsonObject root = document.object();
    highestUnlockedBuiltIn = qMax(1, root.value(QStringLiteral("highestUnlockedBuiltInLevel")).toInt(1));

    QJsonObject recordObject = root.value(QStringLiteral("records")).toObject();
    for (auto it = recordObject.begin(); it != recordObject.end(); ++it) {
        if (!it.value().isObject()) {
            continue;
        }

        records.insert(it.key(), recordFromJson(it.value().toObject()));
    }
}

bool ProgressManager::save(QString *errorMessage) const
{
    const QString folderPath = progressFolderPath();
    if (!QDir().mkpath(folderPath)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建进度保存目录：%1").arg(folderPath);
        }
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("highestUnlockedBuiltInLevel"), highestUnlockedBuiltIn);

    QJsonObject recordObject;
    for (auto it = records.constBegin(); it != records.constEnd(); ++it) {
        recordObject.insert(it.key(), recordToJson(it.value()));
    }
    root.insert(QStringLiteral("records"), recordObject);

    QSaveFile file(progressFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法写入进度文件：%1").arg(file.errorString());
        }
        return false;
    }

    QJsonDocument document(root);
    file.write(document.toJson(QJsonDocument::Indented));

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("提交进度文件失败：%1").arg(file.errorString());
        }
        return false;
    }

    return true;
}

bool ProgressManager::reset(QString *errorMessage)
{
    records.clear();
    highestUnlockedBuiltIn = 1;

    QFile file(progressFilePath());
    if (file.exists() && !file.remove()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("删除进度文件失败：%1").arg(file.errorString());
        }
        return false;
    }

    return true;
}

bool ProgressManager::isBuiltInLevelUnlocked(int zeroBasedLevelIndex) const
{
    return zeroBasedLevelIndex >= 0 && zeroBasedLevelIndex + 1 <= highestUnlockedBuiltIn;
}

int ProgressManager::highestUnlockedBuiltInLevelNumber() const
{
    return highestUnlockedBuiltIn;
}

bool ProgressManager::hasRecordForLevel(const Level &level) const
{
    return records.contains(levelKey(level));
}

ProgressManager::Record ProgressManager::recordForLevel(const Level &level) const
{
    return records.value(levelKey(level));
}

bool ProgressManager::updateAfterCompletion(const Level &level,
                                            int zeroBasedLevelIndex,
                                            int stars,
                                            int elapsedMs,
                                            int reverseCount,
                                            int deathCount,
                                            QString *errorMessage)
{
    const QString key = levelKey(level);
    Record record = records.value(key);

    record.completed = true;
    record.bestStars = qMax(record.bestStars, stars);

    if (record.bestTimeMs < 0 || elapsedMs < record.bestTimeMs) {
        record.bestTimeMs = elapsedMs;
    }

    if (record.bestReverseCount < 0 || reverseCount < record.bestReverseCount) {
        record.bestReverseCount = reverseCount;
    }

    if (record.bestDeathCount < 0 || deathCount < record.bestDeathCount) {
        record.bestDeathCount = deathCount;
    }

    record.lastCompletedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    records.insert(key, record);

    if (!level.isCustomLevel && zeroBasedLevelIndex >= 0) {
        // 通关第 N 关后，解锁第 N+1 关。最后一关通关时该值可能比总关卡数多 1，显示时不会有副作用。
        highestUnlockedBuiltIn = qMax(highestUnlockedBuiltIn, zeroBasedLevelIndex + 2);
    }

    return save(errorMessage);
}

QString ProgressManager::progressTextForLevel(const Level &level,
                                              int zeroBasedLevelIndex) const
{
    if (!level.isCustomLevel && !isBuiltInLevelUnlocked(zeroBasedLevelIndex)) {
        return QStringLiteral("🔒 未解锁");
    }

    const Record record = recordForLevel(level);
    if (!record.completed) {
        return level.isCustomLevel ? QStringLiteral("自定义关卡 · 未通关")
                                   : QStringLiteral("已解锁 · 未通关");
    }

    QStringList parts;
    parts << starText(record.bestStars);

    if (record.bestTimeMs >= 0) {
        parts << QStringLiteral("最快 %1 秒").arg(formatTimeMs(record.bestTimeMs));
    }

    if (record.bestReverseCount >= 0) {
        parts << QStringLiteral("最少反转 %1").arg(record.bestReverseCount);
    }

    if (record.bestDeathCount >= 0) {
        parts << QStringLiteral("最少死亡 %1").arg(record.bestDeathCount);
    }

    return parts.join(QStringLiteral("  |  "));
}

QString ProgressManager::levelKey(const Level &level)
{
    QByteArray raw;
    raw += level.isCustomLevel ? "custom\n" : "builtin\n";
    raw += level.name.toUtf8();
    raw += '\n';
    raw += level.mapData.join(QStringLiteral("\n")).toUtf8();

    const QByteArray digest = QCryptographicHash::hash(raw, QCryptographicHash::Sha1).toHex();
    return QStringLiteral("%1:%2").arg(level.isCustomLevel ? QStringLiteral("custom")
                                                            : QStringLiteral("builtin"),
                                       QString::fromLatin1(digest));
}

QString ProgressManager::starText(int stars)
{
    stars = qBound(0, stars, 3);

    QString text;
    for (int i = 0; i < stars; ++i) {
        text += QStringLiteral("★");
    }
    for (int i = stars; i < 3; ++i) {
        text += QStringLiteral("☆");
    }

    return text;
}

QString ProgressManager::formatTimeMs(int ms)
{
    if (ms < 0) {
        return QStringLiteral("--");
    }

    return QString::number(ms / 1000.0, 'f', 2);
}

QString ProgressManager::progressFolderPath()
{
    QDir dir(QCoreApplication::applicationDirPath());

    const QString currentFolderName = dir.dirName();
    if (currentFolderName.startsWith(QStringLiteral("Desktop_"), Qt::CaseInsensitive)
        || currentFolderName.contains(QStringLiteral("Qt"), Qt::CaseInsensitive)
        || currentFolderName.contains(QStringLiteral("Debug"), Qt::CaseInsensitive)
        || currentFolderName.contains(QStringLiteral("Release"), Qt::CaseInsensitive)) {
        dir.cdUp();
    }

    if (dir.dirName().compare(QStringLiteral("build"), Qt::CaseInsensitive) == 0) {
        dir.cdUp();
    }

    return dir.filePath(QStringLiteral("save"));
}

QString ProgressManager::progressFilePath()
{
    return QDir(progressFolderPath()).filePath(QStringLiteral("progress.json"));
}

QJsonObject ProgressManager::recordToJson(const Record &record)
{
    QJsonObject object;
    object.insert(QStringLiteral("completed"), record.completed);
    object.insert(QStringLiteral("bestStars"), record.bestStars);
    object.insert(QStringLiteral("bestTimeMs"), record.bestTimeMs);
    object.insert(QStringLiteral("bestReverseCount"), record.bestReverseCount);
    object.insert(QStringLiteral("bestDeathCount"), record.bestDeathCount);
    object.insert(QStringLiteral("lastCompletedAt"), record.lastCompletedAt);
    return object;
}

ProgressManager::Record ProgressManager::recordFromJson(const QJsonObject &object)
{
    Record record;
    record.completed = object.value(QStringLiteral("completed")).toBool(false);
    record.bestStars = object.value(QStringLiteral("bestStars")).toInt(0);
    record.bestTimeMs = object.value(QStringLiteral("bestTimeMs")).toInt(-1);
    record.bestReverseCount = object.value(QStringLiteral("bestReverseCount")).toInt(-1);
    record.bestDeathCount = object.value(QStringLiteral("bestDeathCount")).toInt(-1);
    record.lastCompletedAt = object.value(QStringLiteral("lastCompletedAt")).toString();
    return record;
}
