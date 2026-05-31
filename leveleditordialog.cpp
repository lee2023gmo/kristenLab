#include "leveleditordialog.h"
#include "tiledefs.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSizePolicy>
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
    , saveFolderComboBox(nullptr)
    , currentToolPreviewLabel(nullptr)
    , zoomInfoLabel(nullptr)
    , currentTile(TileDefs::Empty)
    , mapCellSize(42)
    , isPainting(false)
    , isErasing(false)
    , isBulkUpdating(false)
    , mapPlaceholderLabel(nullptr)
    , mapTable(nullptr)
    , mapPreviewEdit(nullptr)
    , generateButton(nullptr)
    , borderButton(nullptr)
    , clearButton(nullptr)
    , zoomOutButton(nullptr)
    , zoomInButton(nullptr)
    , resetZoomButton(nullptr)
    , importButton(nullptr)
    , validateButton(nullptr)
    , saveButton(nullptr)
    , closeButton(nullptr)
{
    setupUi();
    setupConnections();
}

bool LevelEditorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == mapTable->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            if (mouseEvent->button() == Qt::LeftButton) {
                isPainting = true;
                isErasing = false;
                paintCellAtViewportPosition(mouseEvent->pos(), currentTile);
                return true;
            }

            if (mouseEvent->button() == Qt::RightButton) {
                isPainting = false;
                isErasing = true;
                paintCellAtViewportPosition(mouseEvent->pos(), TileDefs::Empty);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            if (isPainting) {
                paintCellAtViewportPosition(mouseEvent->pos(), currentTile);
                return true;
            }

            if (isErasing) {
                paintCellAtViewportPosition(mouseEvent->pos(), TileDefs::Empty);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            isPainting = false;
            isErasing = false;
        }
        else if (event->type() == QEvent::Leave) {
            isPainting = false;
            isErasing = false;
        }
    }

    return QDialog::eventFilter(watched, event);
}

void LevelEditorDialog::setupUi()
{
    setWindowTitle("KristenLab - 关卡设计师");
    resize(1180, 760);
    setMinimumSize(980, 620);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    QLabel *titleLabel = new QLabel("关卡设计师", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 26px;"
        "font-weight: bold;"
        "color: #80f7ff;"
        );

    QLabel *hintLabel = new QLabel(
        "左侧设置关卡和工具，右侧编辑地图。左键拖动连续绘制，右键拖动擦除为空地。",
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

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(14);
    mainLayout->addLayout(contentLayout, 1);

    QFrame *infoFrame = new QFrame(this);
    infoFrame->setObjectName("infoFrame");
    infoFrame->setFixedWidth(340);

    QVBoxLayout *sideLayout = new QVBoxLayout(infoFrame);
    sideLayout->setContentsMargins(14, 14, 14, 14);
    sideLayout->setSpacing(12);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    nameEdit = new QLineEdit(infoFrame);
    nameEdit->setPlaceholderText("例如：我的设计关卡");

    widthSpinBox = new QSpinBox(infoFrame);
    widthSpinBox->setRange(5, 150);
    widthSpinBox->setValue(12);
    widthSpinBox->setSuffix(" 列");

    heightSpinBox = new QSpinBox(infoFrame);
    heightSpinBox->setRange(5, 150);
    heightSpinBox->setValue(8);
    heightSpinBox->setSuffix(" 行");

    targetReverseSpinBox = new QSpinBox(infoFrame);
    targetReverseSpinBox->setRange(0, 999);
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
    tileComboBox->addItem("蹦床 T", QString(TileDefs::Trampoline));
    tileComboBox->setCurrentIndex(0);

    saveFolderComboBox = new QComboBox(infoFrame);
    saveFolderComboBox->addItem("自定义关卡文件夹 custom_levels", "custom_levels");
    saveFolderComboBox->addItem("内置关卡文件夹 levels", "levels");
    saveFolderComboBox->setCurrentIndex(0);

    currentToolPreviewLabel = new QLabel(infoFrame);
    currentToolPreviewLabel->setAlignment(Qt::AlignCenter);
    currentToolPreviewLabel->setMinimumHeight(34);

    zoomInfoLabel = new QLabel(infoFrame);
    zoomInfoLabel->setAlignment(Qt::AlignCenter);
    zoomInfoLabel->setMinimumHeight(30);
    zoomInfoLabel->setStyleSheet(
        "background-color: #10131f;"
        "color: #cbd5e1;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "padding: 4px;"
        );

    formLayout->addRow("关卡名：", nameEdit);
    formLayout->addRow("宽度：", widthSpinBox);
    formLayout->addRow("高度：", heightSpinBox);
    formLayout->addRow("目标：", targetReverseSpinBox);
    formLayout->addRow("元素：", tileComboBox);
    formLayout->addRow("预览：", currentToolPreviewLabel);
    formLayout->addRow("缩放：", zoomInfoLabel);
    formLayout->addRow("保存：", saveFolderComboBox);

    sideLayout->addLayout(formLayout);

    QFrame *buttonFrame = new QFrame(infoFrame);
    buttonFrame->setObjectName("buttonFrame");

    QGridLayout *buttonLayout = new QGridLayout(buttonFrame);
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    buttonLayout->setHorizontalSpacing(8);
    buttonLayout->setVerticalSpacing(8);

    generateButton = new QPushButton("生成地图", buttonFrame);
    borderButton = new QPushButton("边框墙", buttonFrame);
    clearButton = new QPushButton("清空", buttonFrame);
    zoomOutButton = new QPushButton("缩小地图", buttonFrame);
    zoomInButton = new QPushButton("放大地图", buttonFrame);
    resetZoomButton = new QPushButton("还原缩放", buttonFrame);
    importButton = new QPushButton("导入 JSON", buttonFrame);
    validateButton = new QPushButton("校验地图", buttonFrame);
    saveButton = new QPushButton("保存关卡", buttonFrame);
    closeButton = new QPushButton("关闭", buttonFrame);

    QList<QPushButton *> buttons = {
        generateButton,
        borderButton,
        clearButton,
        zoomOutButton,
        zoomInButton,
        resetZoomButton,
        importButton,
        validateButton,
        saveButton,
        closeButton
    };

    for (QPushButton *button : buttons) {
        button->setMinimumHeight(34);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    buttonLayout->addWidget(generateButton, 0, 0);
    buttonLayout->addWidget(importButton, 0, 1);
    buttonLayout->addWidget(borderButton, 1, 0);
    buttonLayout->addWidget(clearButton, 1, 1);
    buttonLayout->addWidget(zoomOutButton, 2, 0);
    buttonLayout->addWidget(zoomInButton, 2, 1);
    buttonLayout->addWidget(resetZoomButton, 3, 0, 1, 2);
    buttonLayout->addWidget(validateButton, 4, 0);
    buttonLayout->addWidget(saveButton, 4, 1);
    buttonLayout->addWidget(closeButton, 5, 0, 1, 2);

    sideLayout->addWidget(buttonFrame);

    QLabel *sideHintLabel = new QLabel(
        "操作提示：\n"
        "左键拖动：绘制当前元素\n"
        "右键拖动：擦除为空地\n"
        "双击格子：擦除为空地\n"
        "缩小地图：适合 150×150 大图\n横向/纵向滚动条：浏览边角\n"
        "还原缩放：恢复默认格子大小\n"
        "保存后可回到关卡选择查看",
        infoFrame
        );
    sideHintLabel->setWordWrap(true);
    sideHintLabel->setStyleSheet(
        "font-size: 12px;"
        "color: #94a3b8;"
        "line-height: 150%;"
        );

    sideLayout->addWidget(sideHintLabel);
    sideLayout->addStretch();

    QFrame *mapFrame = new QFrame(this);
    mapFrame->setObjectName("mapFrame");
    mapFrame->setMinimumSize(560, 420);
    mapFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
    mapLayout->setContentsMargins(14, 14, 14, 14);
    mapLayout->setSpacing(10);

    mapPlaceholderLabel = new QLabel(
        "地图编辑区域\n\n"
        "点击左侧“生成地图”新建，也可以“导入 JSON”继续编辑已有关卡。",
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
    mapTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mapTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    mapTable->horizontalHeader()->setVisible(false);
    mapTable->verticalHeader()->setVisible(false);
    mapTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    mapTable->horizontalHeader()->setDefaultSectionSize(42);
    mapTable->verticalHeader()->setDefaultSectionSize(42);
    mapTable->horizontalHeader()->setMinimumSectionSize(4);
    mapTable->verticalHeader()->setMinimumSectionSize(4);
    mapTable->viewport()->installEventFilter(this);

    mapPreviewEdit = new QPlainTextEdit(mapFrame);
    mapPreviewEdit->setReadOnly(true);
    mapPreviewEdit->setFixedHeight(84);
    mapPreviewEdit->setPlaceholderText("地图字符串预览会显示在这里。");
    mapPreviewEdit->setVisible(false);

    mapLayout->addWidget(mapPlaceholderLabel);
    mapLayout->addWidget(mapTable, 1);
    mapLayout->addWidget(mapPreviewEdit);

    contentLayout->addWidget(infoFrame);
    contentLayout->addWidget(mapFrame, 1);

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
        "padding: 8px 10px;"
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
        "QPlainTextEdit {"
        "background-color: #10131f;"
        "color: #cbd5e1;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "font-family: Consolas;"
        "font-size: 12px;"
        "}"
        );
}

void LevelEditorDialog::setupConnections()
{
    connect(generateButton, &QPushButton::clicked,
            this, &LevelEditorDialog::generateMapTable);

    connect(borderButton, &QPushButton::clicked,
            this, &LevelEditorDialog::addBorderWalls);

    connect(clearButton, &QPushButton::clicked,
            this, &LevelEditorDialog::clearMapToEmpty);

    connect(zoomOutButton, &QPushButton::clicked,
            this, &LevelEditorDialog::zoomOutMap);

    connect(zoomInButton, &QPushButton::clicked,
            this, &LevelEditorDialog::zoomInMap);

    connect(resetZoomButton, &QPushButton::clicked,
            this, &LevelEditorDialog::resetMapZoom);

    connect(importButton, &QPushButton::clicked,
            this, &LevelEditorDialog::importLevelFromJson);

    connect(tileComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        currentTile = currentTileFromCombo();
        updateCurrentToolPreview();
    });

    connect(mapTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        setCellTile(row, col, TileDefs::Empty);
    });

    connect(validateButton, &QPushButton::clicked,
            this, &LevelEditorDialog::validateMapByButton);

    connect(saveButton, &QPushButton::clicked,
            this, &LevelEditorDialog::saveCurrentLevel);

    connect(closeButton, &QPushButton::clicked, this, &LevelEditorDialog::reject);

    updateCurrentToolPreview();
    updateZoomInfo();
}

void LevelEditorDialog::generateMapTable()
{
    const int columnCount = widthSpinBox->value();
    const int rowCount = heightSpinBox->value();

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);
    isBulkUpdating = true;

    mapTable->clear();
    mapTable->setRowCount(rowCount);
    mapTable->setColumnCount(columnCount);

    for (int col = 0; col < columnCount; ++col) {
        mapTable->setColumnWidth(col, mapCellSize);
    }

    for (int row = 0; row < rowCount; ++row) {
        mapTable->setRowHeight(row, mapCellSize);

        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            mapTable->setItem(row, col, item);

            setCellTile(row, col, TileDefs::Empty);
        }
    }

    isBulkUpdating = false;
    mapTable->setUpdatesEnabled(true);

    mapPlaceholderLabel->setVisible(false);
    mapTable->setVisible(true);
    mapPreviewEdit->setVisible(true);

    mapTable->setCurrentCell(0, 0);
    autoFitMapZoom();
    updateMapPreview();
    adjustEditorSizeToMap();
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

    // 批量生成 / 清空 / 导入时不需要每次都扫描旧起点。
    // 用户手动画起点时仍然保证只有一个起点。
    if (!isBulkUpdating && TileDefs::isStart(tile)) {
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

    // 大地图慢的主要原因是：每改一个格子就 buildMapDataFromTable() 重建整张地图预览。
    // 批量操作时只在最后统一刷新一次。
    if (!isBulkUpdating) {
        updateMapPreview();
    }
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

    // 地图缩得很小时，字体也要一起缩小，否则 0/1 会挤出格子，看起来像没有缩放成功。
    int fontSize = qBound(4, mapCellSize / 2, 11);

    if (TileDefs::isSlow(tile) || TileDefs::isEnd(tile)) {
        fontSize = qBound(4, mapCellSize / 3, 8);
    }

    item->setTextAlignment(Qt::AlignCenter);
    font.setPointSize(fontSize);
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

    if (TileDefs::isTrampoline(tile)) {
        return "T";
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

    if (TileDefs::isTrampoline(tile)) {
        return QColor("#d63384");
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

QStringList LevelEditorDialog::buildMapDataFromTable() const
{
    QStringList mapData;

    if (mapTable == nullptr || !mapTable->isVisible()) {
        return mapData;
    }

    for (int row = 0; row < mapTable->rowCount(); ++row) {
        QString line;

        for (int col = 0; col < mapTable->columnCount(); ++col) {
            line.append(cellTile(row, col));
        }

        mapData.append(line);
    }

    return mapData;
}


bool LevelEditorDialog::validateMapData(const QStringList &mapData, QString *errorMessage) const
{
    if (mapData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图不能为空。";
        }
        return false;
    }

    int expectedColumnCount = mapData[0].size();

    if (expectedColumnCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图第一行不能为空。";
        }
        return false;
    }

    if (expectedColumnCount < 5 || expectedColumnCount > 150) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("地图宽度必须在 5 到 150 之间，当前是 %1。").arg(expectedColumnCount);
        }
        return false;
    }

    if (mapData.size() < 5 || mapData.size() > 150) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("地图高度必须在 5 到 150 之间，当前是 %1。").arg(mapData.size());
        }
        return false;
    }

    int startCount = 0;
    int endCount = 0;

    for (int row = 0; row < mapData.size(); ++row) {
        QString line = mapData[row];

        if (line.size() != expectedColumnCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("第 %1 行长度不一致，应该是 %2，实际是 %3。")
                                    .arg(row + 1)
                                    .arg(expectedColumnCount)
                                    .arg(line.size());
            }
            return false;
        }

        for (int col = 0; col < line.size(); ++col) {
            QChar tile = line[col];

            if (!TileDefs::isKnownTile(tile)) {
                if (errorMessage != nullptr) {
                    *errorMessage = QString("第 %1 行第 %2 列出现非法字符：%3。")
                                        .arg(row + 1)
                                        .arg(col + 1)
                                        .arg(tile);
                }
                return false;
            }

            if (TileDefs::isStart(tile)) {
                startCount++;
            }

            if (TileDefs::isEnd(tile)) {
                endCount++;
            }
        }
    }

    if (startCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图缺少起点 2。请用“起点 S”工具放置一个起点。";
        }
        return false;
    }

    if (startCount > 1) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("地图只能有一个起点 2，但当前有 %1 个。").arg(startCount);
        }
        return false;
    }

    if (endCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图至少需要一个终点 3。请用“终点 END”工具放置终点。";
        }
        return false;
    }

    return true;
}

bool LevelEditorDialog::validateCurrentMap(QString *errorMessage) const
{
    QStringList mapData = buildMapDataFromTable();

    if (mapData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "请先点击“生成地图”或“导入 JSON”，再进行校验。";
        }
        return false;
    }

    return validateMapData(mapData, errorMessage);
}

void LevelEditorDialog::validateMapByButton()
{
    QString errorMessage;

    if (!validateCurrentMap(&errorMessage)) {
        QMessageBox::warning(
            this,
            "地图校验失败",
            "当前地图不合法。\n\n错误原因：\n" + errorMessage
            );
        return;
    }

    QStringList mapData = buildMapDataFromTable();

    QMessageBox::information(
        this,
        "地图校验通过",
        QString("地图校验通过！\n\n地图大小：%1 行 × %2 列\n可以保存为 JSON 文件。")
            .arg(mapData.size())
            .arg(mapData.isEmpty() ? 0 : mapData[0].size())
        );
}

void LevelEditorDialog::addBorderWalls()
{
    if (mapTable == nullptr || !mapTable->isVisible()) {
        QMessageBox::warning(
            this,
            "无法添加边框墙",
            "请先点击“生成地图”或“导入 JSON”，再添加边框墙。"
            );
        return;
    }

    int rowCount = mapTable->rowCount();
    int columnCount = mapTable->columnCount();

    if (rowCount <= 0 || columnCount <= 0) {
        QMessageBox::warning(
            this,
            "无法添加边框墙",
            "当前地图为空。"
            );
        return;
    }

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);
    isBulkUpdating = true;

    for (int col = 0; col < columnCount; ++col) {
        setCellTile(0, col, TileDefs::Wall);
        setCellTile(rowCount - 1, col, TileDefs::Wall);
    }

    for (int row = 0; row < rowCount; ++row) {
        setCellTile(row, 0, TileDefs::Wall);
        setCellTile(row, columnCount - 1, TileDefs::Wall);
    }

    isBulkUpdating = false;
    mapTable->setUpdatesEnabled(true);
    updateMapPreview();

    QMessageBox::information(
        this,
        "边框墙已添加",
        "已将第一行、最后一行、第一列、最后一列全部设置为墙体 1。"
        );
}

void LevelEditorDialog::clearMapToEmpty()
{
    if (mapTable == nullptr || !mapTable->isVisible()) {
        QMessageBox::warning(
            this,
            "无法清空地图",
            "请先点击“生成地图”或“导入 JSON”，再清空地图。"
            );
        return;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "确认清空地图",
        QString("确定要把当前地图全部变成空地 0 吗？\\n\\n地图大小：%1 行 × %2 列")
            .arg(mapTable->rowCount())
            .arg(mapTable->columnCount()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (result != QMessageBox::Yes) {
        return;
    }

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);
    isBulkUpdating = true;

    for (int row = 0; row < mapTable->rowCount(); ++row) {
        for (int col = 0; col < mapTable->columnCount(); ++col) {
            setCellTile(row, col, TileDefs::Empty);
        }
    }

    isBulkUpdating = false;
    mapTable->setUpdatesEnabled(true);
    updateMapPreview();
}

void LevelEditorDialog::updateCurrentToolPreview()
{
    if (currentToolPreviewLabel == nullptr) {
        return;
    }

    QChar tile = currentTile;
    QString text = QString("当前工具：%1").arg(tileToolTip(tile));

    currentToolPreviewLabel->setText(text);
    currentToolPreviewLabel->setStyleSheet(
        QString("background-color: %1; color: %2; border: 2px solid #80f7ff; border-radius: 8px; font-weight: bold; padding: 6px;")
            .arg(tileBackgroundColor(tile).name())
            .arg(tileTextColor(tile).name())
        );
}

void LevelEditorDialog::updateMapPreview()
{
    if (mapPreviewEdit == nullptr) {
        return;
    }

    if (mapTable == nullptr || !mapTable->isVisible()) {
        mapPreviewEdit->clear();
        return;
    }

    const int rowCount = mapTable->rowCount();
    const int columnCount = mapTable->columnCount();
    const int cellCount = rowCount * columnCount;

    if (rowCount <= 0 || columnCount <= 0) {
        mapPreviewEdit->clear();
        return;
    }

    // 大地图不再实时显示完整字符串。
    // 例如 150×150 有 22500 个格子，拖动绘制时每次都拼接完整字符串会明显卡顿。
    // 保存 JSON 时仍然会完整读取整张地图。
    if (cellCount > 5000) {
        mapPreviewEdit->setPlainText(
            QString("大地图预览已简化，以提升编辑性能。\\n"
                    "地图大小：%1 行 × %2 列，共 %3 个格子。\\n"
                    "保存时仍会完整写入 JSON。\\n"
                    "需要检查边角时，请使用横向 / 纵向滚动条。")
                .arg(rowCount)
                .arg(columnCount)
                .arg(cellCount)
            );
        return;
    }

    QStringList mapData = buildMapDataFromTable();

    if (mapData.isEmpty()) {
        mapPreviewEdit->clear();
        return;
    }

    mapPreviewEdit->setPlainText(mapData.join("\\n"));
}

void LevelEditorDialog::adjustEditorSizeToMap()
{
    if (mapTable == nullptr || !mapTable->isVisible()) {
        return;
    }

    const int tableWidth = mapTable->columnCount() * mapCellSize + 40;
    const int tableHeight = mapTable->rowCount() * mapCellSize + 40;

    // 不再把 mapTable 的最小尺寸强行设成整张地图大小。
    // 否则大地图会把表框撑爆，横向滚动条也不好用。
    // 现在表格视口保持一个合理大小，超出的内容靠横向/纵向滚动条浏览。
    const int viewportMinWidth = qBound(520, tableWidth, 1080);
    const int viewportMinHeight = qBound(360, tableHeight, 620);

    mapTable->setMinimumWidth(viewportMinWidth);
    mapTable->setMinimumHeight(viewportMinHeight);

    const int sidePanelWidth = 340;
    const int targetWindowWidth = qMin(qMax(width(), sidePanelWidth + viewportMinWidth + 90), 1480);
    const int targetWindowHeight = qMin(qMax(height(), viewportMinHeight + 260), 920);

    resize(targetWindowWidth, targetWindowHeight);
}

void LevelEditorDialog::setMapCellSize(int cellSize)
{
    // 原来最小 20px 对 80×80、150×150 这种大地图还是太大。
    // 现在允许缩到 6px，用来总览大图；需要精细编辑时再放大。
    mapCellSize = qBound(6, cellSize, 60);

    if (mapTable != nullptr) {
        QSignalBlocker blocker(mapTable);
        mapTable->setUpdatesEnabled(false);

        for (int col = 0; col < mapTable->columnCount(); ++col) {
            mapTable->setColumnWidth(col, mapCellSize);
        }

        for (int row = 0; row < mapTable->rowCount(); ++row) {
            mapTable->setRowHeight(row, mapCellSize);
        }

        // 字体、END/SLOW 等文本也跟随格子大小重新适配。
        for (int row = 0; row < mapTable->rowCount(); ++row) {
            for (int col = 0; col < mapTable->columnCount(); ++col) {
                updateCellStyle(row, col);
            }
        }

        mapTable->setUpdatesEnabled(true);
    }

    updateZoomInfo();
    adjustEditorSizeToMap();
}

void LevelEditorDialog::updateZoomInfo()
{
    if (zoomInfoLabel == nullptr) {
        return;
    }

    int percent = qRound(mapCellSize * 100.0 / 42.0);

    zoomInfoLabel->setText(
        QString("%1 px / %2%")
            .arg(mapCellSize)
            .arg(percent)
        );
}

void LevelEditorDialog::zoomInMap()
{
    setMapCellSize(mapCellSize + 2);
}

void LevelEditorDialog::zoomOutMap()
{
    setMapCellSize(mapCellSize - 2);
}

void LevelEditorDialog::resetMapZoom()
{
    setMapCellSize(42);
}

void LevelEditorDialog::autoFitMapZoom()
{
    if (mapTable == nullptr) {
        return;
    }

    const int rows = mapTable->rowCount();
    const int cols = mapTable->columnCount();

    // 更激进的大地图自动缩放：
    // 150×150 需要能总览和滚动浏览，所以自动到 6px。
    // 用户仍然可以用“放大地图 / 缩小地图 / 还原缩放”手动调整。
    if (cols >= 140 || rows >= 140) {
        setMapCellSize(6);
    }
    else if (cols >= 120 || rows >= 120) {
        setMapCellSize(8);
    }
    else if (cols >= 80 || rows >= 80) {
        setMapCellSize(10);
    }
    else if (cols >= 60 || rows >= 60) {
        setMapCellSize(12);
    }
    else if (cols >= 45 || rows >= 45) {
        setMapCellSize(16);
    }
    else if (cols >= 35 || rows >= 30) {
        setMapCellSize(20);
    }
    else if (cols >= 28 || rows >= 18) {
        setMapCellSize(24);
    }
    else if (cols >= 22 || rows >= 14) {
        setMapCellSize(32);
    }
    else {
        setMapCellSize(42);
    }
}

void LevelEditorDialog::paintCellAtViewportPosition(const QPoint &position, QChar tile)
{
    if (mapTable == nullptr || !mapTable->isVisible()) {
        return;
    }

    QModelIndex index = mapTable->indexAt(position);

    if (!index.isValid()) {
        return;
    }

    setCellTile(index.row(), index.column(), tile);
}

QString LevelEditorDialog::selectedFolderName() const
{
    QString folderName = saveFolderComboBox->currentData().toString();

    if (folderName != "levels" && folderName != "custom_levels") {
        folderName = "custom_levels";
    }

    return folderName;
}

QString LevelEditorDialog::projectRootPath() const
{
    QDir dir(QCoreApplication::applicationDirPath());

    // Qt Creator 默认运行目录通常是：
    // 项目根目录/build/Desktop_Qt_xxx-Debug
    // 这里向上跳出 Desktop_Qt_xxx-Debug，再跳出 build，
    // 回到真正的项目根目录。
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

    return dir.absolutePath();
}

QString LevelEditorDialog::selectedLevelFolderPath() const
{
    QDir projectDir(projectRootPath());
    QString folderPath = projectDir.filePath(selectedFolderName());

    QDir().mkpath(folderPath);

    return folderPath;
}

QString LevelEditorDialog::safeFileName(const QString &name) const
{
    QString fileName = name.trimmed();

    if (fileName.isEmpty()) {
        fileName = QString("custom_level_%1")
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    }

    fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    fileName.replace(QRegularExpression("\\s+"), "_");

    if (!fileName.endsWith(".json", Qt::CaseInsensitive)) {
        fileName += ".json";
    }

    return fileName;
}

void LevelEditorDialog::saveCurrentLevel()
{
    QString errorMessage;

    if (!validateCurrentMap(&errorMessage)) {
        QMessageBox::warning(
            this,
            "保存失败",
            "当前地图不合法，不能保存。\n\n错误原因：\n" + errorMessage
            );
        return;
    }

    QString levelName = nameEdit->text().trimmed();

    if (levelName.isEmpty()) {
        levelName = QString("custom_level_%1")
                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    }

    QStringList mapData = buildMapDataFromTable();

    QJsonObject rootObject;
    rootObject.insert("name", levelName);
    rootObject.insert("targetReverseCount", targetReverseSpinBox->value());

    QJsonArray mapArray;

    for (const QString &line : mapData) {
        mapArray.append(line);
    }

    rootObject.insert("map", mapArray);

    QJsonDocument document(rootObject);

    QString folderPath = selectedLevelFolderPath();
    QString filePath = QDir(folderPath).filePath(safeFileName(levelName));

    if (QFileInfo::exists(filePath)) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            "文件已存在",
            QString("文件已经存在：\n%1\n\n是否覆盖？").arg(filePath),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (result != QMessageBox::Yes) {
            return;
        }
    }

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(
            this,
            "保存失败",
            QString("无法写入文件：\n%1").arg(filePath)
            );
        return;
    }

    file.write(document.toJson(QJsonDocument::Indented));
    file.close();

    QString folderName = selectedFolderName();

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "保存成功",
        QString("保存成功！\n\n文件已保存到项目根目录下的 %1 文件夹。\n\n完整路径：\n%2\n\n是否立即回到关卡选择界面查看新关卡？")
            .arg(folderName)
            .arg(filePath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
        );

    if (result == QMessageBox::Yes) {
        emit requestOpenLevelSelect();
    }
}


void LevelEditorDialog::importLevelFromJson()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "导入关卡 JSON",
        projectRootPath(),
        "JSON 文件 (*.json);;所有文件 (*.*)"
        );

    if (filePath.isEmpty()) {
        return;
    }

    QString name;
    int targetReverseCount = 0;
    QStringList mapData;
    QString errorMessage;

    if (!loadLevelJsonFile(filePath, &name, &targetReverseCount, &mapData, &errorMessage)) {
        QMessageBox::warning(
            this,
            "导入失败",
            QString("无法导入该 JSON 文件。\n\n文件：\n%1\n\n错误原因：\n%2")
                .arg(filePath)
                .arg(errorMessage)
            );
        return;
    }

    nameEdit->setText(name);
    targetReverseSpinBox->setValue(targetReverseCount);
    loadMapDataToTable(mapData);

    // 导入后默认另存到 custom_levels，避免误覆盖内置关卡。
    saveFolderComboBox->setCurrentIndex(0);

    QMessageBox::information(
        this,
        "导入成功",
        QString("已成功导入关卡：%1\n\n来源文件：\n%2\n\n你可以继续编辑，修改后建议保存到 custom_levels。")
            .arg(name)
            .arg(filePath)
        );
}

bool LevelEditorDialog::loadLevelJsonFile(const QString &filePath,
                                          QString *name,
                                          int *targetReverseCount,
                                          QStringList *mapData,
                                          QString *errorMessage) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = "无法打开文件。";
        }
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = "JSON 解析失败：" + parseError.errorString();
        }
        return false;
    }

    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = "JSON 根节点必须是对象。";
        }
        return false;
    }

    QJsonObject object = document.object();

    QString loadedName = object.value("name").toString();

    if (loadedName.isEmpty()) {
        loadedName = QFileInfo(filePath).baseName();
    }

    int loadedTargetReverseCount = object.value("targetReverseCount").toInt(0);

    QJsonValue mapValue = object.value("map");

    if (!mapValue.isArray()) {
        if (errorMessage != nullptr) {
            *errorMessage = "JSON 缺少 map 数组。";
        }
        return false;
    }

    QJsonArray mapArray = mapValue.toArray();

    if (mapArray.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "map 数组不能为空。";
        }
        return false;
    }

    QStringList loadedMapData;

    for (int i = 0; i < mapArray.size(); ++i) {
        QJsonValue rowValue = mapArray.at(i);

        if (!rowValue.isString()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("map 第 %1 行不是字符串。").arg(i + 1);
            }
            return false;
        }

        loadedMapData.append(rowValue.toString());
    }

    QString validateError;

    if (!validateMapData(loadedMapData, &validateError)) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图数据不合法：" + validateError;
        }
        return false;
    }

    if (name != nullptr) {
        *name = loadedName;
    }

    if (targetReverseCount != nullptr) {
        *targetReverseCount = loadedTargetReverseCount;
    }

    if (mapData != nullptr) {
        *mapData = loadedMapData;
    }

    return true;
}

void LevelEditorDialog::loadMapDataToTable(const QStringList &mapData)
{
    if (mapData.isEmpty()) {
        return;
    }

    const int rowCount = mapData.size();
    const int columnCount = mapData[0].size();

    widthSpinBox->setValue(columnCount);
    heightSpinBox->setValue(rowCount);

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);
    isBulkUpdating = true;

    mapTable->clear();
    mapTable->setRowCount(rowCount);
    mapTable->setColumnCount(columnCount);

    for (int col = 0; col < columnCount; ++col) {
        mapTable->setColumnWidth(col, mapCellSize);
    }

    for (int row = 0; row < rowCount; ++row) {
        mapTable->setRowHeight(row, mapCellSize);

        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            mapTable->setItem(row, col, item);

            setCellTile(row, col, mapData[row][col]);
        }
    }

    isBulkUpdating = false;
    mapTable->setUpdatesEnabled(true);

    mapPlaceholderLabel->setVisible(false);
    mapTable->setVisible(true);
    mapPreviewEdit->setVisible(true);

    mapTable->setCurrentCell(0, 0);
    autoFitMapZoom();
    updateMapPreview();
    adjustEditorSizeToMap();
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
                          "你点击的是：%7"
                          )
                          .arg(levelName)
                          .arg(widthSpinBox->value())
                          .arg(heightSpinBox->value())
                          .arg(tableInfo)
                          .arg(targetReverseSpinBox->value())
                          .arg(tileToolTip(currentTile))
                          .arg(actionName);

    QMessageBox::information(this, "阶段 29 提示", message);
}
