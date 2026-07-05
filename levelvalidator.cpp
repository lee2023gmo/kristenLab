#include "levelvalidator.h"
#include "tiledefs.h"

#include <QSet>

namespace {
QString sentenceSuffix(const LevelValidator::Options &options)
{
    return options.editorFriendlyMessages ? QStringLiteral("。") : QString();
}

QString pointKey(int col, int row)
{
    return QStringLiteral("%1,%2").arg(col).arg(row);
}

bool rejectEditablePointTile(QChar tile, const LevelValidator::Options &options)
{
    if (TileDefs::isWall(tile)
        || TileDefs::isStart(tile)
        || TileDefs::isEnd(tile)
        || TileDefs::isDeath(tile)
        || TileDefs::isData(tile)) {
        return true;
    }

    if (options.rejectLegacyMechanismEditablePoints
        && (TileDefs::isBounce(tile) || TileDefs::isConveyor(tile))) {
        return true;
    }

    return false;
}
}

LevelValidator::Options LevelValidator::runtimeOptions()
{
    Options options;
    options.editablePointLabel = QStringLiteral("候选编辑点");
    return options;
}

LevelValidator::Options LevelValidator::editorOptions()
{
    Options options;
    options.minWidth = 5;
    options.maxWidth = 150;
    options.minHeight = 5;
    options.maxHeight = 150;
    options.minTargetReverseCount = 0;
    options.maxTargetReverseCount = 999;
    options.checkTargetReverseCount = true;
    options.rejectLegacyMechanismEditablePoints = true;
    options.editorFriendlyMessages = true;
    options.editablePointLabel = QStringLiteral("玩家编辑候选点");
    return options;
}

bool LevelValidator::validateLevel(const Level &level,
                                   QString *errorMessage)
{
    return validateLevel(level, errorMessage, runtimeOptions());
}

bool LevelValidator::validateMapData(const QStringList &mapData,
                                     QString *errorMessage)
{
    return validateMapData(mapData, errorMessage, runtimeOptions());
}

bool LevelValidator::validateEditablePoints(const QStringList &mapData,
                                            const QVector<QPoint> &editablePoints,
                                            QString *errorMessage)
{
    return validateEditablePoints(mapData, editablePoints, errorMessage, runtimeOptions());
}

bool LevelValidator::validateLevel(const Level &level,
                                   QString *errorMessage,
                                   const Options &options)
{
    if (options.checkTargetReverseCount
        && (level.targetReverseCount < options.minTargetReverseCount
            || level.targetReverseCount > options.maxTargetReverseCount)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("目标反转次数必须在 %1 到 %2 之间，当前是 %3%4")
                                .arg(options.minTargetReverseCount)
                                .arg(options.maxTargetReverseCount)
                                .arg(level.targetReverseCount)
                                .arg(sentenceSuffix(options));
        }
        return false;
    }

    if (!validateMapData(level.mapData, errorMessage, options)) {
        return false;
    }

    if (!validateEditablePoints(level.mapData, level.editablePoints, errorMessage, options)) {
        return false;
    }

    return true;
}

bool LevelValidator::validateMapData(const QStringList &mapData,
                                     QString *errorMessage,
                                     const Options &options)
{
    const QString suffix = sentenceSuffix(options);

    if (mapData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图不能为空") + suffix;
        }
        return false;
    }

    const int expectedColumnCount = mapData[0].size();

    if (expectedColumnCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图第一行不能为空") + suffix;
        }
        return false;
    }

    if (options.minWidth > 0 && expectedColumnCount < options.minWidth) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图宽度必须在 %1 到 %2 之间，当前是 %3%4")
                                .arg(options.minWidth)
                                .arg(options.maxWidth)
                                .arg(expectedColumnCount)
                                .arg(suffix);
        }
        return false;
    }

    if (options.maxWidth > 0 && expectedColumnCount > options.maxWidth) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图宽度必须在 %1 到 %2 之间，当前是 %3%4")
                                .arg(options.minWidth)
                                .arg(options.maxWidth)
                                .arg(expectedColumnCount)
                                .arg(suffix);
        }
        return false;
    }

    if (options.minHeight > 0 && mapData.size() < options.minHeight) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图高度必须在 %1 到 %2 之间，当前是 %3%4")
                                .arg(options.minHeight)
                                .arg(options.maxHeight)
                                .arg(mapData.size())
                                .arg(suffix);
        }
        return false;
    }

    if (options.maxHeight > 0 && mapData.size() > options.maxHeight) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图高度必须在 %1 到 %2 之间，当前是 %3%4")
                                .arg(options.minHeight)
                                .arg(options.maxHeight)
                                .arg(mapData.size())
                                .arg(suffix);
        }
        return false;
    }

    int startCount = 0;
    int endCount = 0;

    for (int row = 0; row < mapData.size(); ++row) {
        const QString line = mapData[row];

        if (line.size() != expectedColumnCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("第 %1 行长度不一致，应该是 %2，实际是 %3%4")
                                    .arg(row + 1)
                                    .arg(expectedColumnCount)
                                    .arg(line.size())
                                    .arg(suffix);
            }
            return false;
        }

        for (int col = 0; col < line.size(); ++col) {
            const QChar tile = line[col];

            if (!TileDefs::isKnownTile(tile)) {
                if (errorMessage != nullptr) {
                    *errorMessage = QStringLiteral("第 %1 行第 %2 列出现非法字符：%3%4")
                                        .arg(row + 1)
                                        .arg(col + 1)
                                        .arg(tile)
                                        .arg(suffix);
                }
                return false;
            }

            if (TileDefs::isStart(tile)) {
                ++startCount;
            }

            if (TileDefs::isEnd(tile)) {
                ++endCount;
            }
        }
    }

    if (startCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = options.editorFriendlyMessages
                                ? QStringLiteral("地图缺少起点 2。请用“起点 S”工具放置一个起点。")
                                : QStringLiteral("地图缺少起点 2");
        }
        return false;
    }

    if (startCount > 1) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("地图只能有一个起点 2，但当前有 %1 个%2")
                                .arg(startCount)
                                .arg(suffix);
        }
        return false;
    }

    if (endCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = options.editorFriendlyMessages
                                ? QStringLiteral("地图至少需要一个终点 3。请用“终点 END”工具放置终点。")
                                : QStringLiteral("地图至少需要一个终点 3");
        }
        return false;
    }

    return true;
}

bool LevelValidator::validateEditablePoints(const QStringList &mapData,
                                            const QVector<QPoint> &editablePoints,
                                            QString *errorMessage,
                                            const Options &options)
{
    QSet<QString> usedKeys;
    const QString label = options.editablePointLabel.isEmpty()
                              ? QStringLiteral("候选编辑点")
                              : options.editablePointLabel;

    for (const QPoint &point : editablePoints) {
        const int col = point.x();
        const int row = point.y();

        if (row < 0 || row >= mapData.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1行号越界：row=%2 col=%3")
                                    .arg(label)
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        if (col < 0 || col >= mapData[row].size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1列号越界：row=%2 col=%3")
                                    .arg(label)
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        const QString key = pointKey(col, row);

        if (usedKeys.contains(key)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1重复：row=%2 col=%3")
                                    .arg(label)
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }

        usedKeys.insert(key);

        const QChar tile = mapData[row][col];

        if (rejectEditablePointTile(tile, options)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1不能放在 %2 上：row=%3 col=%4")
                                    .arg(label)
                                    .arg(TileDefs::nameOf(tile))
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }
    }

    return true;
}
