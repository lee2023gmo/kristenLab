#ifndef LEVELEDITORDIALOG_H
#define LEVELEDITORDIALOG_H

#include <QChar>
#include <QDialog>
#include <QString>
#include <QStringList>

class QPoint;

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class LevelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LevelEditorDialog(QWidget *parent = nullptr);

signals:
    void requestOpenLevelSelect();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QLineEdit *nameEdit;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *targetReverseSpinBox;

    QComboBox *tileComboBox;
    QComboBox *saveFolderComboBox;
    QLabel *currentToolPreviewLabel;
    QChar currentTile;

    bool isPainting;
    bool isErasing;

    QLabel *mapPlaceholderLabel;
    QTableWidget *mapTable;
    QPlainTextEdit *mapPreviewEdit;

    QPushButton *generateButton;
    QPushButton *borderButton;
<<<<<<< HEAD
    QPushButton *clearButton;
=======
>>>>>>> 9b45a65 (Need to check)
    QPushButton *importButton;
    QPushButton *validateButton;
    QPushButton *saveButton;
    QPushButton *closeButton;

    void setupUi();
    void setupConnections();

    // 阶段 24：根据宽度和高度生成表格式地图
    void generateMapTable();

    // 阶段 25：关卡元素绘制工具
    void setCellTile(int row, int col, QChar tile);
    QChar cellTile(int row, int col) const;
    void updateCellStyle(int row, int col);
    void clearOldStartTile();

    QChar currentTileFromCombo() const;
    QString tileDisplayText(QChar tile) const;
    QString tileToolTip(QChar tile) const;
    QColor tileBackgroundColor(QChar tile) const;
    QColor tileTextColor(QChar tile) const;

    // 阶段 26：设计师模式地图校验
    QStringList buildMapDataFromTable() const;
    bool validateMapData(const QStringList &mapData, QString *errorMessage) const;
    bool validateCurrentMap(QString *errorMessage) const;
    void validateMapByButton();
    void addBorderWalls();
    void clearMapToEmpty();
    void updateCurrentToolPreview();
    void updateMapPreview();
    void adjustEditorSizeToMap();
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
                           QString *errorMessage) const;
    void loadMapDataToTable(const QStringList &mapData);

    void showStageTip(const QString &actionName);
};

#endif // LEVELEDITORDIALOG_H
