#ifndef LEVELVALIDATOR_H
#define LEVELVALIDATOR_H

#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>

#include "level.h"

// 关卡合法性校验的统一入口。
//
// 拆分目的：
// 1. LevelManager 和 LevelEditorDialog 不再各自维护一套重复校验逻辑。
// 2. 后续如果地图尺寸、目标反转次数、候选点规则变化，只需要优先改这里。
// 3. 通过 Options 保留“运行时兼容旧关卡”和“编辑器严格限制”两种模式。
class LevelValidator
{
public:
    struct Options
    {
        // 0 表示不限制。编辑器使用 5~150，运行时默认继续兼容旧关卡。
        int minWidth = 0;
        int maxWidth = 0;
        int minHeight = 0;
        int maxHeight = 0;

        // 目标反转次数限制。编辑器使用 0~999，运行时默认继续兼容旧关卡。
        int minTargetReverseCount = 0;
        int maxTargetReverseCount = 0;
        bool checkTargetReverseCount = false;

        // 旧关卡兼容：运行时允许候选点落在旧机关 5/7 上；编辑器保存/导入时更严格。
        bool rejectLegacyMechanismEditablePoints = false;

        // 文案差异：编辑器弹窗更友好，运行时日志更简洁。
        bool editorFriendlyMessages = false;
        QString editablePointLabel = QStringLiteral("候选编辑点");
    };

    static Options runtimeOptions();
    static Options editorOptions();

    static bool validateLevel(const Level &level,
                              QString *errorMessage = nullptr);
    static bool validateLevel(const Level &level,
                              QString *errorMessage,
                              const Options &options);

    static bool validateMapData(const QStringList &mapData,
                                QString *errorMessage = nullptr);
    static bool validateMapData(const QStringList &mapData,
                                QString *errorMessage,
                                const Options &options);

    static bool validateEditablePoints(const QStringList &mapData,
                                       const QVector<QPoint> &editablePoints,
                                       QString *errorMessage = nullptr);
    static bool validateEditablePoints(const QStringList &mapData,
                                       const QVector<QPoint> &editablePoints,
                                       QString *errorMessage,
                                       const Options &options);
};

#endif // LEVELVALIDATOR_H
