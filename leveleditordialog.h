#ifndef LEVELEDITORDIALOG_H
#define LEVELEDITORDIALOG_H

#include <QChar>
#include <QDialog>
#include <QPoint>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "level.h"

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QLineEdit;
class QMouseEvent;
class QShowEvent;
class QPushButton;
class QSpinBox;
class QTableWidget;

class LevelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LevelEditorDialog(QWidget *parent = nullptr);
    void loadLevelForEditing(const Level &level, const QString &sourceFilePath = QString());

signals:
    void requestOpenLevelSelect();
    void requestTestLevel(const Level &level);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    static const QChar EditablePointTool;

    struct EditorSnapshot
    {
        bool tableVisible = false;
        QString levelName;
        int width = 12;
        int height = 8;
        int targetReverseCount = 6;
        int mapCellSize = 42;
        int tileComboIndex = 0;
        int saveFolderIndex = 0;
        QStringList mapData;
        QVector<QPoint> editablePoints;
    };

    QLineEdit *nameEdit;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *targetReverseSpinBox;

    QComboBox *tileComboBox;
    QComboBox *saveFolderComboBox;
    QLabel *currentToolPreviewLabel;
    QLabel *zoomInfoLabel;
    QLabel *editablePointInfoLabel;
    QLabel *dragWindowHandleLabel;
    QChar currentTile;
    int mapCellSize;

    bool isPainting;
    bool isErasing;
    bool isBulkUpdating;
    bool isEditablePointMode;
    bool isDraggingWindow;
    bool isRestoringSnapshot;
    bool isEditingExistingCustomLevel;
    QString editingSourceFilePath;
    QPoint dragWindowOffset;
    QSet<QString> editablePointKeys;
    QVector<EditorSnapshot> undoStack;
    QVector<EditorSnapshot> redoStack;

    QLabel *mapPlaceholderLabel;
    QTableWidget *mapTable;
    QPlainTextEdit *mapPreviewEdit;

    QPushButton *generateButton;
    QPushButton *undoButton;
    QPushButton *redoButton;
    QPushButton *borderButton;
    QPushButton *clearButton;
    QPushButton *zoomOutButton;
    QPushButton *zoomInButton;
    QPushButton *resetZoomButton;
    QPushButton *editablePointModeButton;
    QPushButton *clearEditablePointsButton;
    QPushButton *importButton;
    QPushButton *validateButton;
    QPushButton *testButton;
    QPushButton *saveButton;
    QPushButton *closeButton;

    void setupUi();
    void setupConnections();

    // 关卡设计师撤销 / 重做
    EditorSnapshot createEditorSnapshot() const;
    void restoreEditorSnapshot(const EditorSnapshot &snapshot);
    void pushUndoSnapshot();
    void undoEdit();
    void redoEdit();
    void updateUndoRedoButtons();

    // 阶段 24：根据宽度和高度生成表格式地图
    void generateMapTable();

    // 阶段 25：关卡元素绘制工具
    void setCellTile(int row, int col, QChar tile);
    QChar cellTile(int row, int col) const;
    void updateCellStyle(int row, int col);
    void clearOldStartTile();

    QChar currentTileFromCombo() const;
    bool isEditablePointTool(QChar tile) const;
    int editablePointToolComboIndex() const;
    void selectEditablePointTool();

    QString tileDisplayText(QChar tile) const;
    QString tileToolTip(QChar tile) const;
    QColor tileBackgroundColor(QChar tile) const;
    QColor tileTextColor(QChar tile) const;

    // 阶段 26：设计师模式地图校验
    QStringList buildMapDataFromTable() const;
    bool validateMapData(const QStringList &mapData, QString *errorMessage) const;
    bool validateCurrentMap(QString *errorMessage) const;
    void validateMapByButton();
    void testCurrentLevel();
    void addBorderWalls();
    void clearMapToEmpty();
    void updateCurrentToolPreview();
    void updateMapPreview();
    void adjustEditorSizeToMap();
    void setMapCellSize(int cellSize);
    void updateZoomInfo();
    void zoomInMap();
    void zoomOutMap();
    void resetMapZoom();
    void autoFitMapZoom();

    // 玩家编辑模式候选点设计
    QString editablePointKey(int row, int col) const;
    bool isEditablePoint(int row, int col) const;
    bool canBeEditablePoint(int row, int col, QString *errorMessage = nullptr) const;
    void addEditablePoint(int row, int col);
    void removeEditablePoint(int row, int col);
    void clearEditablePoints();
    void setEditablePointMode(bool enabled);
    void updateEditablePointInfo();
    void refreshAllCellStyles();
    QVector<QPoint> buildEditablePointsFromTable() const;
    bool validateEditablePoints(const QStringList &mapData,
                                const QVector<QPoint> &editablePoints,
                                QString *errorMessage) const;
    void loadEditablePointsToTable(const QVector<QPoint> &editablePoints);

    void paintCellAtViewportPosition(const QPoint &position, QChar tile);

    // 阶段 27：保存为 JSON 文件
    QString projectRootPath() const;
    QString selectedLevelFolderPath() const;
    QString selectedFolderName() const;
    QString safeFileName(const QString &name) const;
    void saveCurrentLevel();

    // 阶段 28：导入已有 JSON 继续编辑
    void importLevelFromJson();
    bool loadLevelJsonFile(const QString &filePath,
                           QString *name,
                           int *targetReverseCount,
                           QStringList *mapData,
                           QVector<QPoint> *editablePoints,
                           QString *errorMessage) const;
    void loadMapDataToTable(const QStringList &mapData);

    void showStageTip(const QString &actionName);

    // 标题栏出屏时的窗口拖动辅助
    bool isBottomDragArea(const QPoint &position) const;
    void startWindowDrag(const QPoint &globalPosition);
    void updateWindowDrag(const QPoint &globalPosition);
    void stopWindowDrag();
    void moveDialogInsideScreen();
    void fitDialogToAvailableScreen();
};

#endif // LEVELEDITORDIALOG_H
