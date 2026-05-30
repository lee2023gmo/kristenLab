#include "leveleditordialog.h"
#include "tiledefs.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
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
    , currentTile(TileDefs::Empty)
    , mapPlaceholderLabel(nullptr)
    , mapTable(nullptr)
    , generateButton(nullptr)
    , borderButton(nullptr)
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
    resize(960, 760);

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
        "阶段 27：地图校验通过后，可以保存为 JSON 文件，并被关卡选择界面自动读取。",
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

    saveFolderComboBox = new QComboBox(infoFrame);
    saveFolderComboBox->addItem("自定义关卡文件夹 custom_levels", "custom_levels");
    saveFolderComboBox->addItem("内置关卡文件夹 levels", "levels");
    saveFolderComboBox->setCurrentIndex(0);

    formLayout->addRow("关卡名：", nameEdit);
    formLayout->addRow("地图宽度：", widthSpinBox);
    formLayout->addRow("地图高度：", heightSpinBox);
    formLayout->addRow("目标反转次数：", targetReverseSpinBox);
    formLayout->addRow("当前绘制元素：", tileComboBox);
    formLayout->addRow("保存位置：", saveFolderComboBox);

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
    borderButton = new QPushButton("自动加边框墙", buttonFrame);
    validateButton = new QPushButton("校验地图", buttonFrame);
    saveButton = new QPushButton("保存关卡", buttonFrame);
    closeButton = new QPushButton("关闭", buttonFrame);

    generateButton->setMinimumHeight(36);
    borderButton->setMinimumHeight(36);
    validateButton->setMinimumHeight(36);
    saveButton->setMinimumHeight(36);
    closeButton->setMinimumHeight(36);

    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(borderButton);
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
        "padding: 8px 14px;"
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

    connect(borderButton, &QPushButton::clicked,
            this, &LevelEditorDialog::addBorderWalls);

    connect(tileComboBox, &QComboBox::currentIndexChanged, this, [this]() {
        currentTile = currentTileFromCombo();
    });

    connect(mapTable, &QTableWidget::cellClicked, this, [this](int row, int col) {
        setCellTile(row, col, currentTile);
    });

    connect(mapTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        setCellTile(row, col, TileDefs::Empty);
    });

    connect(validateButton, &QPushButton::clicked,
            this, &LevelEditorDialog::validateMapByButton);

    connect(saveButton, &QPushButton::clicked,
            this, &LevelEditorDialog::saveCurrentLevel);

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

bool LevelEditorDialog::validateCurrentMap(QString *errorMessage) const
{
    QStringList mapData = buildMapDataFromTable();

    if (mapData.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = "请先点击“生成地图”，再进行校验。";
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
            "请先点击“生成地图”，再添加边框墙。"
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

    for (int col = 0; col < columnCount; ++col) {
        setCellTile(0, col, TileDefs::Wall);
        setCellTile(rowCount - 1, col, TileDefs::Wall);
    }

    for (int row = 0; row < rowCount; ++row) {
        setCellTile(row, 0, TileDefs::Wall);
        setCellTile(row, columnCount - 1, TileDefs::Wall);
    }

    QMessageBox::information(
        this,
        "边框墙已添加",
        "已将第一行、最后一行、第一列、最后一列全部设置为墙体 1。"
        );
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

    QMessageBox::information(
        this,
        "保存成功",
        QString("保存成功！\n\n文件已保存到项目根目录下的 %1 文件夹。\n\n完整路径：\n%2\n\n请回到关卡选择界面查看新关卡。")
            .arg(folderName)
            .arg(filePath)
        );
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

    QMessageBox::information(this, "阶段 27 提示", message);
}
