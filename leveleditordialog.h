#ifndef LEVELEDITORDIALOG_H
#define LEVELEDITORDIALOG_H

#include <QChar>
#include <QDialog>
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class LevelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LevelEditorDialog(QWidget *parent = nullptr);

private:
    QLineEdit *nameEdit;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *targetReverseSpinBox;

    QComboBox *tileComboBox;
    QChar currentTile;

    QLabel *mapPlaceholderLabel;
    QTableWidget *mapTable;

    QPushButton *generateButton;
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

    void showStageTip(const QString &actionName);
};

#endif // LEVELEDITORDIALOG_H
