#include <algorithm>
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
#include <QShowEvent>
#include <QScreen>
#include <QScrollArea>
#include <QShortcut>
#include <QKeySequence>
#include <QGuiApplication>
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

const QChar LevelEditorDialog::EditablePointTool = QChar('@');

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
    , editablePointInfoLabel(nullptr)
    , dragWindowHandleLabel(nullptr)
    , currentTile(TileDefs::Empty)
    , mapCellSize(42)
    , isPainting(false)
    , isErasing(false)
    , isBulkUpdating(false)
    , isEditablePointMode(false)
    , isDraggingWindow(false)
    , isRestoringSnapshot(false)
    , isEditingExistingCustomLevel(false)
    , editingSourceFilePath(QString())
    , dragWindowOffset(QPoint(0, 0))
    , mapPlaceholderLabel(nullptr)
    , mapTable(nullptr)
    , mapPreviewEdit(nullptr)
    , generateButton(nullptr)
    , undoButton(nullptr)
    , redoButton(nullptr)
    , borderButton(nullptr)
    , clearButton(nullptr)
    , zoomOutButton(nullptr)
    , zoomInButton(nullptr)
    , resetZoomButton(nullptr)
    , editablePointModeButton(nullptr)
    , clearEditablePointsButton(nullptr)
    , importButton(nullptr)
    , validateButton(nullptr)
    , testButton(nullptr)
    , saveButton(nullptr)
    , closeButton(nullptr)
{
    setupUi();
    setupConnections();
}

void LevelEditorDialog::loadLevelForEditing(const Level &level, const QString &sourceFilePath)
{
    if (level.mapData.isEmpty()) {
        return;
    }

    isEditingExistingCustomLevel = level.isCustomLevel;
    editingSourceFilePath = sourceFilePath.trimmed().isEmpty()
                                ? level.sourceFilePath
                                : sourceFilePath;

    nameEdit->setText(level.name);
    targetReverseSpinBox->setValue(level.targetReverseCount);
    saveFolderComboBox->setCurrentIndex(0);

    loadMapDataToTable(level.mapData);
    loadEditablePointsToTable(level.editablePoints);

    undoStack.clear();
    redoStack.clear();
    updateUndoRedoButtons();

    if (isEditingExistingCustomLevel) {
        setWindowTitle(QStringLiteral("关卡设计师 - 编辑自定义关卡：%1").arg(level.name));
    }
}

bool LevelEditorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == dragWindowHandleLabel) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            if (mouseEvent->button() == Qt::LeftButton) {
                startWindowDrag(mouseEvent->globalPosition().toPoint());
                mouseEvent->accept();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            if (isDraggingWindow) {
                updateWindowDrag(mouseEvent->globalPosition().toPoint());
                mouseEvent->accept();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            stopWindowDrag();
            event->accept();
            return true;
        }
    }

    if (mapTable != nullptr && watched == mapTable->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            if (mouseEvent->button() == Qt::LeftButton) {
                pushUndoSnapshot();
                isPainting = true;
                isErasing = false;
                paintCellAtViewportPosition(mouseEvent->pos(), currentTile);
                return true;
            }

            if (mouseEvent->button() == Qt::RightButton) {
                pushUndoSnapshot();
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
    setWindowFlags(windowFlags()
                   | Qt::WindowSystemMenuHint
                   | Qt::WindowMinimizeButtonHint
                   | Qt::WindowMaximizeButtonHint
                   | Qt::WindowCloseButtonHint);
    resize(1180, 760);
    setMinimumSize(980, 620);
    setSizeGripEnabled(true);

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
        "左侧设置关卡和工具，右侧编辑地图。候选点 E 可像普通元素一样绘制；反弹块和传送带已移除。",
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

    QScrollArea *sideScrollArea = new QScrollArea(this);
    sideScrollArea->setWidgetResizable(true);
    sideScrollArea->setFrameShape(QFrame::NoFrame);
    sideScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sideScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sideScrollArea->setFixedWidth(372);
    sideScrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    QFrame *infoFrame = new QFrame();
    infoFrame->setObjectName("infoFrame");
    infoFrame->setMinimumWidth(340);
    infoFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

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
    tileComboBox->addItem("缓冲区 SLOW", QString(TileDefs::Slow));
    tileComboBox->addItem("激光门 L", QString(TileDefs::Laser));
    tileComboBox->addItem("钥匙 K", QString(TileDefs::Key));
    tileComboBox->addItem("门 A", QString(TileDefs::Door));
    tileComboBox->addItem("传送门 B", QString(TileDefs::Portal));
    tileComboBox->addItem("数据碎片 *", QString(TileDefs::Data));
    tileComboBox->addItem("候选点 E", QString(EditablePointTool));
    tileComboBox->addItem("蹦床 ↗", QString(TileDefs::TrampolineUpRight));
    tileComboBox->addItem("蹦床 ↖", QString(TileDefs::TrampolineUpLeft));
    tileComboBox->addItem("蹦床 ↘", QString(TileDefs::TrampolineDownRight));
    tileComboBox->addItem("蹦床 ↙", QString(TileDefs::TrampolineDownLeft));
    tileComboBox->addItem("蹦床 →", QString(TileDefs::TrampolineRight));
    tileComboBox->addItem("蹦床 ←", QString(TileDefs::TrampolineLeft));
    tileComboBox->setCurrentIndex(0);

    saveFolderComboBox = new QComboBox(infoFrame);
    saveFolderComboBox->addItem("自定义关卡文件夹 custom_levels", "custom_levels");
    saveFolderComboBox->addItem("内置关卡文件夹 levels", "levels");
    saveFolderComboBox->setCurrentIndex(0);

    const int editorControlHeight = 38;
    nameEdit->setMinimumHeight(editorControlHeight);
    widthSpinBox->setMinimumHeight(editorControlHeight);
    heightSpinBox->setMinimumHeight(editorControlHeight);
    targetReverseSpinBox->setMinimumHeight(editorControlHeight);
    tileComboBox->setMinimumHeight(editorControlHeight);
    saveFolderComboBox->setMinimumHeight(editorControlHeight);

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

    editablePointInfoLabel = new QLabel(infoFrame);
    editablePointInfoLabel->setAlignment(Qt::AlignCenter);
    editablePointInfoLabel->setMinimumHeight(30);
    editablePointInfoLabel->setStyleSheet(
        "background-color: #10131f;"
        "color: #facc15;"
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
    formLayout->addRow("候选点：", editablePointInfoLabel);
    formLayout->addRow("保存：", saveFolderComboBox);

    sideLayout->addLayout(formLayout);

    QFrame *buttonFrame = new QFrame(infoFrame);
    buttonFrame->setObjectName("buttonFrame");

    QGridLayout *buttonLayout = new QGridLayout(buttonFrame);
    buttonLayout->setContentsMargins(10, 10, 10, 10);
    buttonLayout->setHorizontalSpacing(8);
    buttonLayout->setVerticalSpacing(8);

    generateButton = new QPushButton("生成地图", buttonFrame);
    undoButton = new QPushButton("撤销", buttonFrame);
    redoButton = new QPushButton("重做", buttonFrame);
    borderButton = new QPushButton("边框墙", buttonFrame);
    clearButton = new QPushButton("清空", buttonFrame);
    zoomOutButton = new QPushButton("缩小地图", buttonFrame);
    zoomInButton = new QPushButton("放大地图", buttonFrame);
    resetZoomButton = new QPushButton("还原缩放", buttonFrame);
    editablePointModeButton = new QPushButton("选择候选点工具", buttonFrame);
    clearEditablePointsButton = new QPushButton("清候选点", buttonFrame);
    importButton = new QPushButton("导入 JSON", buttonFrame);
    validateButton = new QPushButton("校验地图", buttonFrame);
    testButton = new QPushButton("测试当前关卡", buttonFrame);
    saveButton = new QPushButton("保存关卡", buttonFrame);
    closeButton = new QPushButton("关闭", buttonFrame);

    QList<QPushButton *> buttons = {
        generateButton,
        undoButton,
        redoButton,
        borderButton,
        clearButton,
        zoomOutButton,
        zoomInButton,
        resetZoomButton,
        editablePointModeButton,
        clearEditablePointsButton,
        importButton,
        validateButton,
        testButton,
        saveButton,
        closeButton
    };

    for (QPushButton *button : buttons) {
        button->setMinimumHeight(38);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    buttonLayout->addWidget(undoButton, 0, 0);
    buttonLayout->addWidget(redoButton, 0, 1);
    buttonLayout->addWidget(generateButton, 1, 0);
    buttonLayout->addWidget(importButton, 1, 1);
    buttonLayout->addWidget(borderButton, 2, 0);
    buttonLayout->addWidget(clearButton, 2, 1);
    buttonLayout->addWidget(zoomOutButton, 3, 0);
    buttonLayout->addWidget(zoomInButton, 3, 1);
    buttonLayout->addWidget(resetZoomButton, 4, 0, 1, 2);
    buttonLayout->addWidget(editablePointModeButton, 5, 0, 1, 2);
    buttonLayout->addWidget(clearEditablePointsButton, 6, 0, 1, 2);
    buttonLayout->addWidget(validateButton, 7, 0);
    buttonLayout->addWidget(testButton, 7, 1);
    buttonLayout->addWidget(saveButton, 8, 0, 1, 2);
    buttonLayout->addWidget(closeButton, 9, 0, 1, 2);

    sideLayout->addWidget(buttonFrame);

    QLabel *sideHintLabel = new QLabel(
        "操作提示：\n"
        "左键拖动：绘制当前元素\n"
        "右键拖动：擦除为空地\n"
        "Ctrl+Z：撤销，Ctrl+Y：重做\n"
        "测试当前关卡：不保存，直接进入游戏试玩\n"
        "传送门 B：按从上到下、从左到右两两配对\n"
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

    sideScrollArea->setWidget(infoFrame);
    contentLayout->addWidget(sideScrollArea);
    contentLayout->addWidget(mapFrame, 1);

    dragWindowHandleLabel = new QLabel("⇕ 标题栏被屏幕挡住时，可按住这里或窗口底部空白处拖动窗口", this);
    dragWindowHandleLabel->setAlignment(Qt::AlignCenter);
    dragWindowHandleLabel->setMinimumHeight(24);
    dragWindowHandleLabel->setCursor(Qt::SizeAllCursor);
    dragWindowHandleLabel->installEventFilter(this);
    dragWindowHandleLabel->setStyleSheet(
        "background-color: #10131f;"
        "color: #94a3b8;"
        "border: 1px dashed #4a5568;"
        "border-radius: 6px;"
        "font-size: 12px;"
        "padding: 3px;"
        );
    mainLayout->addWidget(dragWindowHandleLabel);

    setStyleSheet(
        "QDialog {"
        "background-color: #151821;"
        "color: white;"
        "font-family: Microsoft YaHei;"
        "}"
        "QScrollArea {"
        "background: transparent;"
        "border: none;"
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

    connect(undoButton, &QPushButton::clicked,
            this, &LevelEditorDialog::undoEdit);

    connect(redoButton, &QPushButton::clicked,
            this, &LevelEditorDialog::redoEdit);

    QShortcut *undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated,
            this, &LevelEditorDialog::undoEdit);

    QShortcut *redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redoShortcut, &QShortcut::activated,
            this, &LevelEditorDialog::redoEdit);

    QShortcut *redoShortcutAlt = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Z")), this);
    connect(redoShortcutAlt, &QShortcut::activated,
            this, &LevelEditorDialog::redoEdit);

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

    connect(editablePointModeButton, &QPushButton::clicked, this, [this]() {
        selectEditablePointTool();
    });

    connect(clearEditablePointsButton, &QPushButton::clicked,
            this, &LevelEditorDialog::clearEditablePoints);

    connect(importButton, &QPushButton::clicked,
            this, &LevelEditorDialog::importLevelFromJson);

    connect(tileComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        currentTile = currentTileFromCombo();

        // 现在“候选点”已经作为元素工具存在。
        // 选择普通元素时自动关闭旧的候选点模式，避免误操作。
        if (!isEditablePointTool(currentTile) && isEditablePointMode) {
            setEditablePointMode(false);
        }

        updateCurrentToolPreview();
    });

    connect(mapTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (isEditablePointMode || isEditablePointTool(currentTile)) {
            removeEditablePoint(row, col);
        } else {
            setCellTile(row, col, TileDefs::Empty);
        }
    });

    connect(validateButton, &QPushButton::clicked,
            this, &LevelEditorDialog::validateMapByButton);

    connect(testButton, &QPushButton::clicked,
            this, &LevelEditorDialog::testCurrentLevel);

    connect(saveButton, &QPushButton::clicked,
            this, &LevelEditorDialog::saveCurrentLevel);

    connect(closeButton, &QPushButton::clicked, this, &LevelEditorDialog::reject);

    updateCurrentToolPreview();
    updateZoomInfo();
    updateEditablePointInfo();
    updateUndoRedoButtons();
}


LevelEditorDialog::EditorSnapshot LevelEditorDialog::createEditorSnapshot() const
{
    EditorSnapshot snapshot;

    snapshot.tableVisible = (mapTable != nullptr && mapTable->isVisible());
    snapshot.levelName = (nameEdit != nullptr) ? nameEdit->text() : QString();
    snapshot.width = (widthSpinBox != nullptr) ? widthSpinBox->value() : 12;
    snapshot.height = (heightSpinBox != nullptr) ? heightSpinBox->value() : 8;
    snapshot.targetReverseCount = (targetReverseSpinBox != nullptr) ? targetReverseSpinBox->value() : 6;
    snapshot.mapCellSize = mapCellSize;
    snapshot.tileComboIndex = (tileComboBox != nullptr) ? tileComboBox->currentIndex() : 0;
    snapshot.saveFolderIndex = (saveFolderComboBox != nullptr) ? saveFolderComboBox->currentIndex() : 0;

    if (snapshot.tableVisible) {
        snapshot.mapData = buildMapDataFromTable();
        snapshot.editablePoints = buildEditablePointsFromTable();
    }

    return snapshot;
}

void LevelEditorDialog::restoreEditorSnapshot(const EditorSnapshot &snapshot)
{
    isRestoringSnapshot = true;

    nameEdit->setText(snapshot.levelName);
    widthSpinBox->setValue(snapshot.width);
    heightSpinBox->setValue(snapshot.height);
    targetReverseSpinBox->setValue(snapshot.targetReverseCount);

    if (tileComboBox != nullptr && snapshot.tileComboIndex >= 0 && snapshot.tileComboIndex < tileComboBox->count()) {
        tileComboBox->setCurrentIndex(snapshot.tileComboIndex);
        currentTile = currentTileFromCombo();
    }

    if (saveFolderComboBox != nullptr && snapshot.saveFolderIndex >= 0 && snapshot.saveFolderIndex < saveFolderComboBox->count()) {
        saveFolderComboBox->setCurrentIndex(snapshot.saveFolderIndex);
    }

    if (!snapshot.tableVisible || snapshot.mapData.isEmpty()) {
        QSignalBlocker blocker(mapTable);
        mapTable->clear();
        mapTable->setRowCount(0);
        mapTable->setColumnCount(0);
        editablePointKeys.clear();
        mapPlaceholderLabel->setVisible(true);
        mapTable->setVisible(false);
        mapPreviewEdit->clear();
        mapPreviewEdit->setVisible(false);
        updateEditablePointInfo();
        updateMapPreview();
        updateCurrentToolPreview();
        updateZoomInfo();
        adjustEditorSizeToMap();
        isRestoringSnapshot = false;
        return;
    }

    loadMapDataToTable(snapshot.mapData);
    loadEditablePointsToTable(snapshot.editablePoints);
    setMapCellSize(snapshot.mapCellSize);
    updateMapPreview();
    updateEditablePointInfo();
    updateCurrentToolPreview();
    adjustEditorSizeToMap();

    isRestoringSnapshot = false;
}

void LevelEditorDialog::pushUndoSnapshot()
{
    if (isRestoringSnapshot) {
        return;
    }

    undoStack.append(createEditorSnapshot());

    constexpr int MaxUndoSteps = 80;
    while (undoStack.size() > MaxUndoSteps) {
        undoStack.removeFirst();
    }

    redoStack.clear();
    updateUndoRedoButtons();
}

void LevelEditorDialog::undoEdit()
{
    if (undoStack.isEmpty()) {
        return;
    }

    EditorSnapshot current = createEditorSnapshot();
    EditorSnapshot previous = undoStack.takeLast();
    redoStack.append(current);
    restoreEditorSnapshot(previous);
    updateUndoRedoButtons();
}

void LevelEditorDialog::redoEdit()
{
    if (redoStack.isEmpty()) {
        return;
    }

    EditorSnapshot current = createEditorSnapshot();
    EditorSnapshot next = redoStack.takeLast();
    undoStack.append(current);
    restoreEditorSnapshot(next);
    updateUndoRedoButtons();
}

void LevelEditorDialog::updateUndoRedoButtons()
{
    if (undoButton != nullptr) {
        undoButton->setEnabled(!undoStack.isEmpty());
    }

    if (redoButton != nullptr) {
        redoButton->setEnabled(!redoStack.isEmpty());
    }
}

void LevelEditorDialog::generateMapTable()
{
    const int columnCount = widthSpinBox->value();
    const int rowCount = heightSpinBox->value();

    pushUndoSnapshot();

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);
    isBulkUpdating = true;

    editablePointKeys.clear();

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
    updateEditablePointInfo();
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

    QString editableError;
    if (editablePointKeys.contains(editablePointKey(row, col))
        && !canBeEditablePoint(row, col, &editableError)) {
        editablePointKeys.remove(editablePointKey(row, col));
        updateEditablePointInfo();
    }

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
    bool editablePoint = isEditablePoint(row, col);

    QColor background = tileBackgroundColor(tile);
    QColor foreground = tileTextColor(tile);

    if (editablePoint) {
        background = QColor("#6b4f00");
        foreground = QColor("#fff7b0");
    }

    item->setBackground(QBrush(background));
    item->setForeground(QBrush(foreground));

    QString displayText = tileDisplayText(tile);

    if (editablePoint) {
        if (TileDefs::isEmpty(tile)) {
            displayText = "E";
        } else {
            displayText = displayText + "*";
        }
    }

    item->setText(displayText);

    QString toolTip = tileToolTip(tile);

    if (editablePoint) {
        toolTip += "；玩家编辑模式可改";
    }

    item->setToolTip(toolTip);

    QFont font = item->font();
    font.setBold(editablePoint || !TileDefs::isEmpty(tile));

    int fontSize = qBound(4, mapCellSize / 2, 11);

    if (TileDefs::isSlow(tile) || TileDefs::isEnd(tile) || TileDefs::isDoor(tile) || TileDefs::isPortal(tile)) {
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

bool LevelEditorDialog::isEditablePointTool(QChar tile) const
{
    return tile == EditablePointTool;
}

int LevelEditorDialog::editablePointToolComboIndex() const
{
    if (tileComboBox == nullptr) {
        return -1;
    }

    for (int i = 0; i < tileComboBox->count(); ++i) {
        QString data = tileComboBox->itemData(i).toString();

        if (!data.isEmpty() && data.at(0) == EditablePointTool) {
            return i;
        }
    }

    return -1;
}

void LevelEditorDialog::selectEditablePointTool()
{
    int index = editablePointToolComboIndex();

    if (index >= 0) {
        tileComboBox->setCurrentIndex(index);
    }

    currentTile = EditablePointTool;
    isEditablePointMode = false;
    updateCurrentToolPreview();
    updateEditablePointInfo();
}

QString LevelEditorDialog::tileDisplayText(QChar tile) const
{
    if (isEditablePointTool(tile)) {
        return "E";
    }

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

    if (TileDefs::isLaser(tile)) {
        return "L";
    }

    if (TileDefs::isKey(tile)) {
        return "K";
    }

    if (TileDefs::isDoor(tile)) {
        return "A";
    }

    if (TileDefs::isPortal(tile)) {
        return "B";
    }

    if (TileDefs::isData(tile)) {
        return "*";
    }

    if (TileDefs::isTrampoline(tile)) {
        return TileDefs::trampolineArrow(tile);
    }

    return "?";
}

QString LevelEditorDialog::tileToolTip(QChar tile) const
{
    if (isEditablePointTool(tile)) {
        return "E：玩家编辑候选点";
    }

    return QString("%1：%2").arg(tile).arg(TileDefs::nameOf(tile));
}

QColor LevelEditorDialog::tileBackgroundColor(QChar tile) const
{
    if (isEditablePointTool(tile)) {
        return QColor("#6b4f00");
    }

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

    if (TileDefs::isLaser(tile)) {
        return QColor("#ff1744");
    }

    if (TileDefs::isKey(tile)) {
        return QColor("#facc15");
    }

    if (TileDefs::isDoor(tile)) {
        return QColor("#92400e");
    }

    if (TileDefs::isPortal(tile)) {
        return QColor("#6d28d9");
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
    if (isEditablePointTool(tile)) {
        return QColor("#fff7b0");
    }

    if (TileDefs::isEmpty(tile)) {
        return QColor("#94a3b8");
    }

    if (TileDefs::isData(tile) || TileDefs::isKey(tile)) {
        return QColor("#10131f");
    }

    if (TileDefs::isLaser(tile) || TileDefs::isPortal(tile)) {
        return QColor("#ffffff");
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
    int keyCount = 0;
    int doorCount = 0;
    int portalCount = 0;

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

            if (TileDefs::isKey(tile)) {
                keyCount++;
            }

            if (TileDefs::isDoor(tile)) {
                doorCount++;
            }

            if (TileDefs::isPortal(tile)) {
                portalCount++;
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

    if (doorCount > 0 && keyCount == 0) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图中存在门 A，但没有钥匙 K。请至少放置一把钥匙，避免门永远无法打开。";
        }
        return false;
    }

    if (portalCount == 1) {
        if (errorMessage != nullptr) {
            *errorMessage = "地图中只有 1 个传送门 B。传送门必须成对出现。";
        }
        return false;
    }

    if (portalCount % 2 != 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("传送门 B 的数量必须是偶数，当前有 %1 个。传送门会按从上到下、从左到右的顺序两两配对。").arg(portalCount);
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

    if (!validateMapData(mapData, errorMessage)) {
        return false;
    }

    QVector<QPoint> editablePoints = buildEditablePointsFromTable();

    if (!validateEditablePoints(mapData, editablePoints, errorMessage)) {
        return false;
    }

    return true;
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
        QString("地图校验通过！\n\n地图大小：%1 行 × %2 列\n玩家编辑候选点：%3 个\n可以保存为 JSON 文件。")
            .arg(mapData.size())
            .arg(mapData.isEmpty() ? 0 : mapData[0].size())
            .arg(editablePointKeys.size())
        );
}

void LevelEditorDialog::testCurrentLevel()
{
    QString errorMessage;

    if (!validateCurrentMap(&errorMessage)) {
        QMessageBox::warning(
            this,
            "测试失败",
            "当前地图不合法，不能进入测试。\n\n错误原因：\n" + errorMessage
            );
        return;
    }

    QString levelName = nameEdit->text().trimmed();
    if (levelName.isEmpty()) {
        levelName = "设计器临时测试关卡";
    }

    Level level(
        levelName,
        buildMapDataFromTable(),
        targetReverseSpinBox->value(),
        buildEditablePointsFromTable(),
        true
        );

    emit requestTestLevel(level);
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

    pushUndoSnapshot();

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

    pushUndoSnapshot();

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

    if (isEditablePointMode || isEditablePointTool(currentTile)) {
        currentToolPreviewLabel->setText("当前工具：候选点 E");
        currentToolPreviewLabel->setStyleSheet(
            "background-color: #6b4f00;"
            "color: #fff7b0;"
            "border: 2px solid #facc15;"
            "border-radius: 8px;"
            "font-weight: bold;"
            "padding: 6px;"
            );
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
    QRect availableGeometry;

    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        availableGeometry = screen->availableGeometry();
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        availableGeometry = screen->availableGeometry();
    }

    const int sidePanelWidth = 372;
    int maxWindowWidth = 1480;
    int maxWindowHeight = 900;

    if (availableGeometry.isValid()) {
        maxWindowWidth = qMax(980, availableGeometry.width() - 80);
        maxWindowHeight = qMax(620, availableGeometry.height() - 80);
    }

    if (mapTable == nullptr || !mapTable->isVisible()) {
        resize(qMin(1180, maxWindowWidth), qMin(760, maxWindowHeight));
        moveDialogInsideScreen();
        return;
    }

    const int tableWidth = mapTable->columnCount() * mapCellSize + 40;
    const int tableHeight = mapTable->rowCount() * mapCellSize + 40;

    // 右侧地图区域根据地图大小增长，但窗口只增长到屏幕能放下的大小。
    // 超出部分使用表格自带横向 / 纵向滚动条浏览。
    const int viewportMinWidth = qBound(520, tableWidth, qMax(520, maxWindowWidth - sidePanelWidth - 110));
    const int viewportMinHeight = qBound(320, tableHeight, qMax(320, maxWindowHeight - 300));

    mapTable->setMinimumWidth(viewportMinWidth);
    mapTable->setMinimumHeight(viewportMinHeight);

    const int targetWindowWidth = qBound(980, sidePanelWidth + viewportMinWidth + 90, maxWindowWidth);
    const int targetWindowHeight = qBound(620, viewportMinHeight + 290, maxWindowHeight);

    resize(targetWindowWidth, targetWindowHeight);
    moveDialogInsideScreen();
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

QString LevelEditorDialog::editablePointKey(int row, int col) const
{
    return QString("%1,%2").arg(col).arg(row);
}

bool LevelEditorDialog::isEditablePoint(int row, int col) const
{
    return editablePointKeys.contains(editablePointKey(row, col));
}

bool LevelEditorDialog::canBeEditablePoint(int row, int col, QString *errorMessage) const
{
    if (mapTable == nullptr || !mapTable->isVisible()) {
        if (errorMessage != nullptr) {
            *errorMessage = "请先生成或导入地图。";
        }
        return false;
    }

    if (row < 0 || row >= mapTable->rowCount()
        || col < 0 || col >= mapTable->columnCount()) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("候选点越界：row=%1 col=%2").arg(row).arg(col);
        }
        return false;
    }

    QChar tile = cellTile(row, col);

    if (TileDefs::isWall(tile)
        || TileDefs::isStart(tile)
        || TileDefs::isEnd(tile)
        || TileDefs::isDeath(tile)
        || TileDefs::isData(tile)
        || TileDefs::isKey(tile)
        || TileDefs::isDoor(tile)
        || TileDefs::isPortal(tile)
        || TileDefs::isBounce(tile)
        || TileDefs::isConveyor(tile)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("候选点不能放在 %1 上。").arg(TileDefs::nameOf(tile));
        }
        return false;
    }

    return true;
}

void LevelEditorDialog::addEditablePoint(int row, int col)
{
    QString errorMessage;

    if (!canBeEditablePoint(row, col, &errorMessage)) {
        return;
    }

    QString key = editablePointKey(row, col);

    if (!editablePointKeys.contains(key)) {
        editablePointKeys.insert(key);
        updateCellStyle(row, col);
        updateEditablePointInfo();
    }
}

void LevelEditorDialog::removeEditablePoint(int row, int col)
{
    QString key = editablePointKey(row, col);

    if (editablePointKeys.remove(key) > 0) {
        updateCellStyle(row, col);
        updateEditablePointInfo();
    }
}

void LevelEditorDialog::clearEditablePoints()
{
    if (editablePointKeys.isEmpty()) {
        QMessageBox::information(
            this,
            "没有候选点",
            "当前地图还没有玩家编辑候选点。"
            );
        return;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "清空候选点",
        QString("确定要清空全部 %1 个玩家编辑候选点吗？\n\n这不会改变地图元素，只会清除玩家编辑模式可编辑的位置。")
            .arg(editablePointKeys.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (result != QMessageBox::Yes) {
        return;
    }

    pushUndoSnapshot();

    QVector<QPoint> oldPoints = buildEditablePointsFromTable();
    editablePointKeys.clear();

    for (const QPoint &point : oldPoints) {
        if (mapTable != nullptr
            && point.y() >= 0 && point.y() < mapTable->rowCount()
            && point.x() >= 0 && point.x() < mapTable->columnCount()) {
            updateCellStyle(point.y(), point.x());
        }
    }

    updateEditablePointInfo();
}

void LevelEditorDialog::setEditablePointMode(bool enabled)
{
    isEditablePointMode = enabled;

    if (editablePointModeButton != nullptr) {
        editablePointModeButton->setText(
            isEditablePointMode ? "候选点工具：开" : "选择候选点工具"
            );
        editablePointModeButton->setStyleSheet(
            isEditablePointMode
                ? "background-color: #6b4f00; color: #fff7b0; border: 1px solid #facc15; border-radius: 8px; padding: 8px 10px; font-size: 14px;"
                : ""
            );
    }

    if (enabled) {
        int index = editablePointToolComboIndex();

        if (index >= 0) {
            tileComboBox->setCurrentIndex(index);
        }

        currentTile = EditablePointTool;
    }

    updateCurrentToolPreview();
    updateEditablePointInfo();
}

void LevelEditorDialog::updateEditablePointInfo()
{
    if (editablePointInfoLabel == nullptr) {
        return;
    }

    editablePointInfoLabel->setText(
        QString("%1 个可编辑位置").arg(editablePointKeys.size())
        );
}

void LevelEditorDialog::refreshAllCellStyles()
{
    if (mapTable == nullptr) {
        return;
    }

    QSignalBlocker blocker(mapTable);
    mapTable->setUpdatesEnabled(false);

    for (int row = 0; row < mapTable->rowCount(); ++row) {
        for (int col = 0; col < mapTable->columnCount(); ++col) {
            updateCellStyle(row, col);
        }
    }

    mapTable->setUpdatesEnabled(true);
}

QVector<QPoint> LevelEditorDialog::buildEditablePointsFromTable() const
{
    QVector<QPoint> points;

    for (const QString &key : editablePointKeys) {
        QStringList parts = key.split(',');

        if (parts.size() != 2) {
            continue;
        }

        bool colOk = false;
        bool rowOk = false;
        int col = parts[0].toInt(&colOk);
        int row = parts[1].toInt(&rowOk);

        if (colOk && rowOk) {
            points.append(QPoint(col, row));
        }
    }

    std::sort(points.begin(), points.end(), [](const QPoint &a, const QPoint &b) {
        if (a.y() == b.y()) {
            return a.x() < b.x();
        }

        return a.y() < b.y();
    });

    return points;
}

bool LevelEditorDialog::validateEditablePoints(const QStringList &mapData,
                                               const QVector<QPoint> &editablePoints,
                                               QString *errorMessage) const
{
    QSet<QString> usedKeys;

    for (const QPoint &point : editablePoints) {
        int col = point.x();
        int row = point.y();

        if (row < 0 || row >= mapData.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("玩家编辑候选点行号越界：row=%1 col=%2").arg(row).arg(col);
            }
            return false;
        }

        if (col < 0 || col >= mapData[row].size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("玩家编辑候选点列号越界：row=%1 col=%2").arg(row).arg(col);
            }
            return false;
        }

        QString key = QString("%1,%2").arg(col).arg(row);

        if (usedKeys.contains(key)) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("玩家编辑候选点重复：row=%1 col=%2").arg(row).arg(col);
            }
            return false;
        }

        usedKeys.insert(key);

        QChar tile = mapData[row][col];

        if (TileDefs::isWall(tile)
            || TileDefs::isStart(tile)
            || TileDefs::isEnd(tile)
            || TileDefs::isDeath(tile)
            || TileDefs::isData(tile)
            || TileDefs::isKey(tile)
            || TileDefs::isDoor(tile)
            || TileDefs::isPortal(tile)
            || TileDefs::isBounce(tile)
            || TileDefs::isConveyor(tile)) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("玩家编辑候选点不能放在 %1 上：row=%2 col=%3")
                                    .arg(TileDefs::nameOf(tile))
                                    .arg(row)
                                    .arg(col);
            }
            return false;
        }
    }

    return true;
}

void LevelEditorDialog::loadEditablePointsToTable(const QVector<QPoint> &editablePoints)
{
    editablePointKeys.clear();

    if (mapTable == nullptr) {
        return;
    }

    for (const QPoint &point : editablePoints) {
        if (point.y() < 0 || point.y() >= mapTable->rowCount()
            || point.x() < 0 || point.x() >= mapTable->columnCount()) {
            continue;
        }

        editablePointKeys.insert(editablePointKey(point.y(), point.x()));
    }

    refreshAllCellStyles();
    updateEditablePointInfo();
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

    const int row = index.row();
    const int col = index.column();

    // 候选点现在是一个真正的绘制工具：
    // 左键 / 拖动：添加候选点
    // 右键 / 拖动：删除候选点
    //
    // 旧版“候选点模式”也保留兼容。
    if (isEditablePointMode || isEditablePointTool(currentTile) || isEditablePointTool(tile)) {
        if (isErasing) {
            removeEditablePoint(row, col);
        } else {
            addEditablePoint(row, col);
        }

        return;
    }

    if (isErasing) {
        setCellTile(row, col, TileDefs::Empty);
        return;
    }

    setCellTile(row, col, tile);
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
    QVector<QPoint> editablePoints = buildEditablePointsFromTable();

    QJsonObject rootObject;
    rootObject.insert("name", levelName);
    rootObject.insert("targetReverseCount", targetReverseSpinBox->value());

    QJsonArray mapArray;

    for (const QString &line : mapData) {
        mapArray.append(line);
    }

    rootObject.insert("map", mapArray);

    QJsonArray editablePointsArray;

    for (const QPoint &point : editablePoints) {
        QJsonObject pointObject;
        pointObject.insert("row", point.y());
        pointObject.insert("col", point.x());
        editablePointsArray.append(pointObject);
    }

    rootObject.insert("editablePoints", editablePointsArray);

    QJsonDocument document(rootObject);

    QString folderPath = selectedLevelFolderPath();
    QString filePath = QDir(folderPath).filePath(safeFileName(levelName));
    QString folderName = selectedFolderName();

    QString oldEditingPath;
    if (!editingSourceFilePath.trimmed().isEmpty()) {
        oldEditingPath = QDir::fromNativeSeparators(
            QDir::cleanPath(QFileInfo(editingSourceFilePath).absoluteFilePath())
            );
    }

    QString newSavePath = QDir::fromNativeSeparators(
        QDir::cleanPath(QFileInfo(filePath).absoluteFilePath())
        );

    bool overwriteSameCustomFile = false;
    bool shouldRemoveOldCustomFile = false;

    if (isEditingExistingCustomLevel
        && folderName == QStringLiteral("custom_levels")
        && !oldEditingPath.trimmed().isEmpty()) {
        overwriteSameCustomFile = (oldEditingPath == newSavePath);
        shouldRemoveOldCustomFile = !overwriteSameCustomFile;
    }

    if (QFileInfo::exists(filePath) && !overwriteSameCustomFile) {
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

    if (shouldRemoveOldCustomFile && QFileInfo::exists(oldEditingPath)) {
        QFile oldFile(oldEditingPath);
        if (!oldFile.remove()) {
            QMessageBox::warning(
                this,
                "旧文件清理失败",
                QString("新关卡文件已经保存，但旧文件删除失败：\n%1\n\n原因：%2")
                    .arg(oldEditingPath)
                    .arg(oldFile.errorString())
                );
        }
    }

    if (folderName == QStringLiteral("custom_levels")) {
        isEditingExistingCustomLevel = true;
        editingSourceFilePath = filePath;
        setWindowTitle(QStringLiteral("关卡设计师 - 编辑自定义关卡：%1").arg(levelName));
    }

    const QString saveActionText = overwriteSameCustomFile
                                       ? QStringLiteral("自定义关卡已更新")
                                       : QStringLiteral("保存成功");

    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        saveActionText,
        QString("%1！\n\n文件已保存到项目根目录下的 %2 文件夹。\n\n玩家编辑候选点：%3 个\n\n完整路径：\n%4\n\n是否立即回到关卡选择界面查看关卡？")
            .arg(saveActionText)
            .arg(folderName)
            .arg(editablePoints.size())
            .arg(filePath),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
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
    QVector<QPoint> editablePoints;
    QString errorMessage;

    if (!loadLevelJsonFile(filePath, &name, &targetReverseCount, &mapData, &editablePoints, &errorMessage)) {
        QMessageBox::warning(
            this,
            "导入失败",
            QString("无法导入该 JSON 文件。\n\n文件：\n%1\n\n错误原因：\n%2")
                .arg(filePath)
                .arg(errorMessage)
            );
        return;
    }

    pushUndoSnapshot();

    nameEdit->setText(name);
    targetReverseSpinBox->setValue(targetReverseCount);
    loadMapDataToTable(mapData);
    loadEditablePointsToTable(editablePoints);

    // 导入后默认另存到 custom_levels，避免误覆盖已有自定义关卡或内置关卡。
    isEditingExistingCustomLevel = false;
    editingSourceFilePath.clear();
    setWindowTitle(QStringLiteral("关卡设计师"));
    saveFolderComboBox->setCurrentIndex(0);

    QMessageBox::information(
        this,
        "导入成功",
        QString("已成功导入关卡：%1\n\n玩家编辑候选点：%2 个\n\n来源文件：\n%3\n\n你可以继续编辑，修改后建议保存到 custom_levels。")
            .arg(name)
            .arg(editablePoints.size())
            .arg(filePath)
        );
}

bool LevelEditorDialog::loadLevelJsonFile(const QString &filePath,
                                          QString *name,
                                          int *targetReverseCount,
                                          QStringList *mapData,
                                          QVector<QPoint> *editablePoints,
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

    QVector<QPoint> loadedEditablePoints;

    QJsonValue editablePointsValue = object.value("editablePoints");

    if (editablePointsValue.isArray()) {
        QJsonArray editablePointsArray = editablePointsValue.toArray();

        for (int i = 0; i < editablePointsArray.size(); ++i) {
            QJsonValue pointValue = editablePointsArray.at(i);

            if (pointValue.isObject()) {
                QJsonObject pointObject = pointValue.toObject();

                if (!pointObject.contains("row") || !pointObject.contains("col")) {
                    if (errorMessage != nullptr) {
                        *errorMessage = QString("editablePoints 第 %1 项缺少 row 或 col。").arg(i + 1);
                    }
                    return false;
                }

                loadedEditablePoints.append(
                    QPoint(
                        pointObject.value("col").toInt(-1),
                        pointObject.value("row").toInt(-1)
                        )
                    );
            }
            else if (pointValue.isArray()) {
                QJsonArray pointArray = pointValue.toArray();

                if (pointArray.size() != 2) {
                    if (errorMessage != nullptr) {
                        *errorMessage = QString("editablePoints 第 %1 项数组长度必须是 2。").arg(i + 1);
                    }
                    return false;
                }

                loadedEditablePoints.append(
                    QPoint(
                        pointArray.at(0).toInt(-1),
                        pointArray.at(1).toInt(-1)
                        )
                    );
            }
            else {
                if (errorMessage != nullptr) {
                    *errorMessage = QString("editablePoints 第 %1 项格式不合法。").arg(i + 1);
                }
                return false;
            }
        }

        if (!validateEditablePoints(loadedMapData, loadedEditablePoints, &validateError)) {
            if (errorMessage != nullptr) {
                *errorMessage = "玩家编辑候选点不合法：" + validateError;
            }
            return false;
        }
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

    if (editablePoints != nullptr) {
        *editablePoints = loadedEditablePoints;
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
    editablePointKeys.clear();

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
    updateEditablePointInfo();
    adjustEditorSizeToMap();
}

bool LevelEditorDialog::isBottomDragArea(const QPoint &position) const
{
    return position.y() >= height() - 36;
}

void LevelEditorDialog::startWindowDrag(const QPoint &globalPosition)
{
    isDraggingWindow = true;
    dragWindowOffset = globalPosition - frameGeometry().topLeft();
}

void LevelEditorDialog::updateWindowDrag(const QPoint &globalPosition)
{
    if (!isDraggingWindow) {
        return;
    }

    move(globalPosition - dragWindowOffset);
    moveDialogInsideScreen();
}

void LevelEditorDialog::stopWindowDrag()
{
    isDraggingWindow = false;
}

void LevelEditorDialog::fitDialogToAvailableScreen()
{
    QRect availableGeometry;

    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        availableGeometry = screen->availableGeometry();
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        availableGeometry = screen->availableGeometry();
    }

    if (!availableGeometry.isValid()) {
        return;
    }

    // 注意：这里的“适应窗口”不是铺满整个屏幕。
    // 只在窗口太大时缩小到屏幕可见范围内，并把它居中。
    const int maxWidth = qMax(980, availableGeometry.width() - 80);
    const int maxHeight = qMax(620, availableGeometry.height() - 80);

    int newWidth = qMin(width(), maxWidth);
    int newHeight = qMin(height(), maxHeight);

    resize(newWidth, newHeight);

    QPoint centeredPosition(
        availableGeometry.left() + (availableGeometry.width() - width()) / 2,
        availableGeometry.top() + (availableGeometry.height() - height()) / 2
        );

    move(centeredPosition);
}

void LevelEditorDialog::moveDialogInsideScreen()
{
    QRect availableGeometry;

    if (QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
        availableGeometry = screen->availableGeometry();
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        availableGeometry = screen->availableGeometry();
    }

    if (!availableGeometry.isValid()) {
        return;
    }

    QRect frame = frameGeometry();

    if (frame.height() > availableGeometry.height() - 20) {
        resize(width(), qMax(560, availableGeometry.height() - 40));
        frame = frameGeometry();
    }

    int minX = availableGeometry.left();
    int maxX = availableGeometry.right() - frame.width() + 1;
    int minY = availableGeometry.top();
    int maxY = availableGeometry.bottom() - frame.height() + 1;

    if (maxX < minX) {
        maxX = minX;
    }

    if (maxY < minY) {
        maxY = minY;
    }

    int newX = qBound(minX, frame.x(), maxX);
    int newY = qBound(minY, frame.y(), maxY);

    if (newX != frame.x() || newY != frame.y()) {
        move(newX, newY);
    }
}

void LevelEditorDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isBottomDragArea(event->position().toPoint())) {
        startWindowDrag(event->globalPosition().toPoint());
        event->accept();
        return;
    }

    QDialog::mousePressEvent(event);
}

void LevelEditorDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (isDraggingWindow) {
        updateWindowDrag(event->globalPosition().toPoint());
        event->accept();
        return;
    }

    QDialog::mouseMoveEvent(event);
}

void LevelEditorDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if (isDraggingWindow) {
        stopWindowDrag();
        event->accept();
        return;
    }

    QDialog::mouseReleaseEvent(event);
}

void LevelEditorDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    fitDialogToAvailableScreen();
    moveDialogInsideScreen();
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
