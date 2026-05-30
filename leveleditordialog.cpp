#include "leveleditordialog.h"

#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

LevelEditorDialog::LevelEditorDialog(QWidget *parent)
    : QDialog(parent)
    , nameEdit(nullptr)
    , widthSpinBox(nullptr)
    , heightSpinBox(nullptr)
    , targetReverseSpinBox(nullptr)
    , mapPlaceholderLabel(nullptr)
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
    resize(760, 620);

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
        "阶段 23：先完成设计师入口和窗口框架。后续阶段会继续实现表格生成、绘制地图、校验和保存。",
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
    mapFrame->setMinimumHeight(280);

    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(16, 16, 16, 16);

    mapPlaceholderLabel = new QLabel(
        "地图编辑区域\n\n阶段 24 会在这里生成 QTableWidget 表格式地图。\n"
        "阶段 25 会加入墙体、起点、终点、死亡区、机关和碎片绘制工具。",
        mapFrame
        );
    mapPlaceholderLabel->setAlignment(Qt::AlignCenter);
    mapPlaceholderLabel->setWordWrap(true);
    mapPlaceholderLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #94a3b8;"
        );

    mapLayout->addWidget(mapPlaceholderLabel);

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
        );
}

void LevelEditorDialog::setupConnections()
{
    connect(generateButton, &QPushButton::clicked, this, [this]() {
        showStageTip("生成地图");
    });

    connect(validateButton, &QPushButton::clicked, this, [this]() {
        showStageTip("校验地图");
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        showStageTip("保存关卡");
    });

    connect(closeButton, &QPushButton::clicked, this, &LevelEditorDialog::reject);
}

void LevelEditorDialog::showStageTip(const QString &actionName)
{
    QString levelName = nameEdit->text().trimmed();

    if (levelName.isEmpty()) {
        levelName = "未命名关卡";
    }

    QString message = QString(
                          "阶段 23 已完成：关卡设计师窗口框架已经可以打开。\n\n"
                          "当前设置：\n"
                          "关卡名：%1\n"
                          "地图大小：%2 列 × %3 行\n"
                          "目标反转次数：%4\n\n"
                          "你点击的是：%5\n"
                          "这个按钮的具体功能会在后续阶段继续实现。"
                          )
                          .arg(levelName)
                          .arg(widthSpinBox->value())
                          .arg(heightSpinBox->value())
                          .arg(targetReverseSpinBox->value())
                          .arg(actionName);

    QMessageBox::information(this, "阶段 23 提示", message);
}
