#include "leveleditordialog.h"
#include "tiledefs.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

LevelEditorDialog::LevelEditorDialog(QWidget *parent)
    : QDialog(parent)
    , nameEdit(nullptr)
    , widthSpinBox(nullptr)
    , heightSpinBox(nullptr)
    , targetReverseSpinBox(nullptr)
    , tileComboBox(nullptr)
    , currentTile(TileDefs::Empty)
    , mapPlaceholderLabel(nullptr)
    , mapTable(nullptr)
    , generateButton(nullptr)
    , validateButton(nullptr)
    , saveButton(nullptr)
    , closeButton(nullptr)
{
    setupUi();
    setupConnections();
}

void LevelEditorDialog::setupUi()
{
    setWindowTitle("KristenLab - 关卡设计师");
    resize(900, 720);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("关卡设计师", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #80f7ff;"
        );

    QLabel *hintLabel = new QLabel(
        "阶段 25：选择地图元素后点击表格格子绘制。双击左键可以擦除为空地。",
        this
        );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet(
        "font-size: 13px;"
        "color: #cbd5e1;"
        );

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(hintLabel);

    QFrame *infoFrame = new QFrame(this);
    infoFrame->setObjectName("infoFrame");

    QFormLayout *formLayout = new QFormLayout(infoFrame);
    formLayout->setContentsMargins(18, 18, 18, 18);
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    nameEdit = new QLineEdit(infoFrame);
    nameEdit->setPlaceholderText("例如：我的设计关卡");

    widthSpinBox = new QSpinBox(infoFrame);
    widthSpinBox->setRange(5, 30);
    widthSpinBox->setValue(12);
    widthSpinBox->setSuffix(" 列");

    heightSpinBox = new QSpinBox(infoFrame);
    heightSpinBox->setRange(5, 20);
    heightSpinBox->setValue(8);
    heightSpinBox->setSuffix(" 行");

    targetReverseSpinBox = new QSpinBox(infoFrame);
    targetReverseSpinBox->setRange(0, 99);
    targetReverseSpinBox->setValue(6);
    targetReverseSpinBox->setSuffix(" 次");

    tileComboBox = new QComboBox(infoFrame);
    tileComboBox->addItem("空地 0", QString(TileDefs::Empty));
    tileComboBox->addItem("墙体 1", QString(TileDefs::Wall));
    tileComboBox->addItem("起点 S", QString(TileDefs::Start));
    tileComboBox->addItem("终点 END", QString(TileDefs::End));
    tileComboBox->addItem("死亡区 X", QString(TileDefs::Death));
    tileComboBox->addItem("弹射块 B", QString(TileDefs::Bounce));
    tileComboBox->addItem("缓冲区 SLOW", QString(TileDefs::Slow));
    tileComboBox->addItem("传送带 →", QString(TileDefs::Conveyor));
    tileComboBox->addItem("数据碎片 *", QString(TileDefs::Data));
    tileComboBox->setCurrentIndex(0);

    formLayout->addRow("关卡名：", nameEdit);
    formLayout->addRow("地图宽度：", widthSpinBox);
    formLayout->addRow("地图高度：", heightSpinBox);
    formLayout->addRow("目标反转次数：", targetReverseSpinBox);
    formLayout->addRow("当前绘制元素：", tileComboBox);

    mainLayout->addWidget(infoFrame);

    QFrame *mapFrame = new QFrame(this);
    mapFrame->setObjectName("mapFrame");
    mapFrame->setMinimumHeight(360);

    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(16, 16, 16, 16);
    mapLayout->setSpacing(10);

    mapPlaceholderLabel = new QLabel(
        "地图编辑区域\n\n"
        "请输入宽度和高度，然后点击“生成地图”。\n"
        "生成后，选择上方元素，左键点击格子绘制，双击左键擦除为空地。",
        mapFrame
        );
    mapPlaceholderLabel->setAlignment(Qt::AlignCenter);
    mapPlaceholderLabel->setWordWrap(true);
    mapPlaceholderLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #94a3b8;"
        );

    mapTable = new QTableWidget(mapFrame);
    mapTable->setVisible(false);
    mapTable->setShowGrid(true);
    mapTable->setAlternatingRowColors(false);
    mapTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mapTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    mapTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mapTable->horizontalHeader()->setVisible(false);
    mapTable->verticalHeader()->setVisible(false);
    mapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->horizontalHeader()->setDefaultSectionSize(42);
    mapTable->verticalHeader()->setDefaultSectionSize(42);

    mapLayout->addWidget(mapPlaceholderLabel);
    mapLayout->addWidget(mapTable, 1);

    mainLayout->addWidget(mapFrame, 1);

    QFrame *buttonFrame = new QFrame(this);
    buttonFrame->setObjectName("buttonFrame");

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonFrame);
    buttonLayout->setContentsMargins(14, 12, 14, 12);
    buttonLayout->setSpacing(10);

    generateButton = new QPushButton("生成地图", buttonFrame);
    validateButton = new QPushButton("校验地图", buttonFrame);
    saveButton = new QPushButton("保存关卡", buttonFrame);
    closeButton = new QPushButton("关闭", buttonFrame);

    generateButton->setMinimumHeight(36);
    validateButton->setMinimumHeight(36);
    saveButton->setMinimumHeight(36);
    closeButton->setMinimumHeight(36);

    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(validateButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addWidget(buttonFrame);

    setStyleSheet(
        "QDialog {"
        "background-color: #151821;"
        "color: white;"
        "font-family: Microsoft YaHei;"
        "}"
        "QFrame#infoFrame, QFrame#mapFrame, QFrame#buttonFrame {"
        "background-color: #202638;"
        "border: 1px solid #33415c;"
        "border-radius: 10px;"
        "}"
        "QLabel {"
        "color: white;"
        "font-size: 14px;"
        "}"
        "QLineEdit, QSpinBox, QComboBox {"
        "background-color: #10131f;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "padding: 6px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus {"
        "border: 1px solid #80f7ff;"
        "}"
        "QPushButton {"
        "background-color: #2d3348;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 8px;"
        "padding: 8px 16px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: #3a86ff;"
        "}"
        "QTableWidget {"
        "background-color: #10131f;"
        "color: white;"
        "gridline-color: #33415c;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "font-size: 13px;"
        "selection-background-color: #3a86ff;"
        "selection-color: white;"
        "}"
        "QTableWidget::item {"
        "padding: 2px;"
        "}"
        );
}

void LevelEditorDialog::setupConnections()
{
    connect(generateButton, &QPushButton::clicked,
            this, &LevelEditorDialog::generateMapTable);

    connect(tileComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        currentTile = currentTileFromCombo();
    });

    connect(mapTable, &QTableWidget::cellClicked, this, [this](int row, int col) {
        setCellTile(row, col, currentTile);
    });

    // 阶段 25 修正：右键在部分系统/触控板上不稳定，
    // 改为“双击左键”擦除当前格子为空地。
    connect(mapTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        setCellTile(row, col, TileDefs::Empty);
    });

    connect(validateButton, &QPushButton::clicked, this, [this]() {
        showStageTip("校验地图");
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        showStageTip("保存关卡");
    });

    connect(closeButton, &QPushButton::clicked, this, &LevelEditorDialog::reject);
}

void LevelEditorDialog::generateMapTable()
{
    const int columnCount = widthSpinBox->value();
    const int rowCount = heightSpinBox->value();

    mapTable->clear();
    mapTable->setRowCount(rowCount);
    mapTable->setColumnCount(columnCount);

    for (int row = 0; row < rowCount; ++row) {
        mapTable->setRowHeight(row, 42);

        for (int col = 0; col < columnCount; ++col) {
            mapTable->setColumnWidth(col, 42);

            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            mapTable->setItem(row, col, item);
            setCellTile(row, col, TileDefs::Empty);
        }
    }

    mapPlaceholderLabel->setVisible(false);
    mapTable->setVisible(true);

    for (int col = 0; col < columnCount; ++col) {
        mapTable->setColumnWidth(col, 42);
    }

    for (int row = 0; row < rowCount; ++row) {
        mapTable->setRowHeight(row, 42);
    }

    mapTable->setCurrentCell(0, 0);
}

void LevelEditorDialog::setCellTile(int row, int col, QChar tile)
{
    if (mapTable == nullptr) {
        return;
    }

    if (row < 0 || row >= mapTable->rowCount()) {
        return;
    }

    if (col < 0 || col >= mapTable->columnCount()) {
        return;
    }

    if (!TileDefs::isKnownTile(tile)) {
        tile = TileDefs::Empty;
    }

    if (TileDefs::isStart(tile)) {
        clearOldStartTile();
    }

    QTableWidgetItem *item = mapTable->item(row, col);

    if (item == nullptr) {
        item = new QTableWidgetItem();
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        mapTable->setItem(row, col, item);
    }

    item->setData(Qt::UserRole, QString(tile));
    item->setText(tileDisplayText(tile));
    item->setToolTip(tileToolTip(tile));

    updateCellStyle(row, col);
}

QChar LevelEditorDialog::cellTile(int row, int col) const
{
    if (mapTable == nullptr) {
        return TileDefs::Empty;
    }

    if (row < 0 || row >= mapTable->rowCount()) {
        return TileDefs::Empty;
    }

    if (col < 0 || col >= mapTable->columnCount()) {
        return TileDefs::Empty;
    }

    QTableWidgetItem *item = mapTable->item(row, col);

    if (item == nullptr) {
        return TileDefs::Empty;
    }

    QString tileText = item->data(Qt::UserRole).toString();

    if (tileText.isEmpty()) {
        return TileDefs::Empty;
    }

    return tileText.at(0);
}

void LevelEditorDialog::updateCellStyle(int row, int col)
{
    QTableWidgetItem *item = mapTable->item(row, col);

    if (item == nullptr) {
        return;
    }

    QChar tile = cellTile(row, col);

    item->setBackground(QBrush(tileBackgroundColor(tile)));
    item->setForeground(QBrush(tileTextColor(tile)));

    QFont font = item->font();
    font.setBold(!TileDefs::isEmpty(tile));
    font.setPointSize(TileDefs::isSlow(tile) || TileDefs::isEnd(tile) ? 8 : 11);
    item->setFont(font);
}

void LevelEditorDialog::clearOldStartTile()
{
    if (mapTable == nullptr) {
        return;
    }

    for (int row = 0; row < mapTable->rowCount(); ++row) {
        for (int col = 0; col < mapTable->columnCount(); ++col) {
            if (cellTile(row, col) == TileDefs::Start) {
                QTableWidgetItem *item = mapTable->item(row, col);

                if (item != nullptr) {
                    item->setData(Qt::UserRole, QString(TileDefs::Empty));
                    item->setText(tileDisplayText(TileDefs::Empty));
                    item->setToolTip(tileToolTip(TileDefs::Empty));
                    updateCellStyle(row, col);
                }
            }
        }
    }
}

QChar LevelEditorDialog::currentTileFromCombo() const
{
    QString data = tileComboBox->currentData().toString();

    if (data.isEmpty()) {
        return TileDefs::Empty;
    }

    return data.at(0);
}

QString LevelEditorDialog::tileDisplayText(QChar tile) const
{
    if (TileDefs::isEmpty(tile)) {
        return "0";
    }

    if (TileDefs::isWall(tile)) {
        return "1";
    }

    if (TileDefs::isStart(tile)) {
        return "S";
    }

    if (TileDefs::isEnd(tile)) {
        return "END";
    }

    if (TileDefs::isDeath(tile)) {
        return "X";
    }

    if (TileDefs::isBounce(tile)) {
        return "B";
    }

    if (TileDefs::isSlow(tile)) {
        return "SLOW";
    }

    if (TileDefs::isConveyor(tile)) {
        return "→";
    }

    if (TileDefs::isData(tile)) {
        return "*";
    }

    return "?";
}

QString LevelEditorDialog::tileToolTip(QChar tile) const
{
    return QString("%1：%2").arg(tile).arg(TileDefs::nameOf(tile));
}

QColor LevelEditorDialog::tileBackgroundColor(QChar tile) const
{
    if (TileDefs::isEmpty(tile)) {
        return QColor("#10131f");
    }

    if (TileDefs::isWall(tile)) {
        return QColor("#3b4252");
    }

    if (TileDefs::isStart(tile)) {
        return QColor("#8a5cff");
    }

    if (TileDefs::isEnd(tile)) {
        return QColor("#2ecc71");
    }

    if (TileDefs::isDeath(tile)) {
        return QColor("#b83232");
    }

    if (TileDefs::isBounce(tile)) {
        return QColor("#f39c12");
    }

    if (TileDefs::isSlow(tile)) {
        return QColor("#0984e3");
    }

    if (TileDefs::isConveyor(tile)) {
        return QColor("#b5a800");
    }

    if (TileDefs::isData(tile)) {
        return QColor("#00f5d4");
    }

    return QColor("#10131f");
}

QColor LevelEditorDialog::tileTextColor(QChar tile) const
{
    if (TileDefs::isEmpty(tile)) {
        return QColor("#94a3b8");
    }

    if (TileDefs::isData(tile)) {
        return QColor("#10131f");
    }

    return QColor("#ffffff");
}

void LevelEditorDialog::showStageTip(const QString &actionName)
{
    QString levelName = nameEdit->text().trimmed();

    if (levelName.isEmpty()) {
        levelName = "未命名关卡";
    }

    QString tableInfo = "尚未生成地图";

    if (mapTable != nullptr && mapTable->isVisible()) {
        tableInfo = QString("%1 行 × %2 列")
                        .arg(mapTable->rowCount())
                        .arg(mapTable->columnCount());
    }

    QString message = QString(
                          "当前设置：\n"
                          "关卡名：%1\n"
                          "输入地图大小：%2 列 × %3 行\n"
                          "当前表格大小：%4\n"
                          "目标反转次数：%5\n"
                          "当前绘制元素：%6\n\n"
                          "你点击的是：%7\n"
                          "校验和保存功能会在后续阶段继续实现。"
                          )
                          .arg(levelName)
                          .arg(widthSpinBox->value())
                          .arg(heightSpinBox->value())
                          .arg(tableInfo)
                          .arg(targetReverseSpinBox->value())
                          .arg(tileToolTip(currentTile))
                          .arg(actionName);

    QMessageBox::information(this, "阶段 25 提示", message);
}
