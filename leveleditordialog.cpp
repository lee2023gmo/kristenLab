#include "leveleditordialog.h"

#include <QAbstractItemView>
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
    resize(860, 680);

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
        "阶段 24：输入宽度和高度，点击“生成地图”，自动生成可编辑的 QTableWidget 表格式地图。",
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

    formLayout->addRow("关卡名：", nameEdit);
    formLayout->addRow("地图宽度：", widthSpinBox);
    formLayout->addRow("地图高度：", heightSpinBox);
    formLayout->addRow("目标反转次数：", targetReverseSpinBox);

    mainLayout->addWidget(infoFrame);

    QFrame *mapFrame = new QFrame(this);
    mapFrame->setObjectName("mapFrame");
    mapFrame->setMinimumHeight(340);

    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(16, 16, 16, 16);
    mapLayout->setSpacing(10);

    mapPlaceholderLabel = new QLabel(
        "地图编辑区域\n\n"
        "请输入宽度和高度，然后点击“生成地图”。\n"
        "生成后，每个格子默认显示 0，表示空地。",
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
    mapTable->setEditTriggers(QAbstractItemView::DoubleClicked
                              | QAbstractItemView::EditKeyPressed
                              | QAbstractItemView::SelectedClicked);

    mapTable->horizontalHeader()->setVisible(false);
    mapTable->verticalHeader()->setVisible(false);
    mapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->horizontalHeader()->setDefaultSectionSize(34);
    mapTable->verticalHeader()->setDefaultSectionSize(34);

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
        "QLineEdit, QSpinBox {"
        "background-color: #10131f;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "padding: 6px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus {"
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
        "font-size: 14px;"
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
        mapTable->setRowHeight(row, 34);

        for (int col = 0; col < columnCount; ++col) {
            mapTable->setColumnWidth(col, 34);

            QTableWidgetItem *item = new QTableWidgetItem("0");
            item->setTextAlignment(Qt::AlignCenter);
            item->setToolTip("0：空地");

            item->setFlags(Qt::ItemIsSelectable
                           | Qt::ItemIsEnabled
                           | Qt::ItemIsEditable);

            mapTable->setItem(row, col, item);
        }
    }

    mapPlaceholderLabel->setVisible(false);
    mapTable->setVisible(true);

    for (int col = 0; col < columnCount; ++col) {
        mapTable->setColumnWidth(col, 34);
    }

    for (int row = 0; row < rowCount; ++row) {
        mapTable->setRowHeight(row, 34);
    }

    mapTable->setCurrentCell(0, 0);
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
                          "目标反转次数：%5\n\n"
                          "你点击的是：%6\n"
                          "校验和保存功能会在后续阶段继续实现。"
                          )
                          .arg(levelName)
                          .arg(widthSpinBox->value())
                          .arg(heightSpinBox->value())
                          .arg(tableInfo)
                          .arg(targetReverseSpinBox->value())
                          .arg(actionName);

    QMessageBox::information(this, "阶段 24 提示", message);
}
