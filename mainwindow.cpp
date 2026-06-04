#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "gamescene.h"
#include "levelmanager.h"
#include "leveleditordialog.h"

#include <QDebug>
#include <QDialog>
#include <QFrame>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollArea>
#include <QTimer>
#include <QTransform>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , gameView(nullptr)
    , gameScene(nullptr)
    , levelLabel(nullptr)
    , timeLabel(nullptr)
    , gravityLabel(nullptr)
    , reverseLabel(nullptr)
    , deathLabel(nullptr)
    , stateLabel(nullptr)
    , viewZoomLabel(nullptr)
    , gameViewScale(1.0)
{
    ui->setupUi(this);

    setupMainMenu();
}

MainWindow::~MainWindow()
{
    clearGameScene();
    delete ui;
}

void MainWindow::clearGameScene()
{
    if (gameView != nullptr) {
        gameView->setScene(nullptr);
    }

    if (gameScene != nullptr) {
        delete gameScene;
        gameScene = nullptr;
    }

    gameView = nullptr;

    levelLabel = nullptr;
    timeLabel = nullptr;
    gravityLabel = nullptr;
    reverseLabel = nullptr;
    deathLabel = nullptr;
    stateLabel = nullptr;
    viewZoomLabel = nullptr;
    gameViewScale = 1.0;
}

void MainWindow::setupMainMenu()
{
    clearGameScene();

    setWindowTitle("KristenLab - 主菜单");
    resize(1000, 700);

    QWidget *central = new QWidget(this);
    central->setStyleSheet(
        "background-color: #151821;"
        "color: white;"
        "font-family: Microsoft YaHei;"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(80, 80, 80, 80);
    mainLayout->setSpacing(24);

    QLabel *titleLabel = new QLabel("KristenLab", central);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 48px;"
        "font-weight: bold;"
        "color: #80f7ff;"
        );

    QLabel *subTitleLabel = new QLabel("简化重力解谜小游戏", central);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    subTitleLabel->setStyleSheet(
        "font-size: 20px;"
        "color: #e6edf3;"
        );

    QPushButton *startButton = new QPushButton("开始游戏", central);
    QPushButton *levelButton = new QPushButton("关卡选择", central);
    QPushButton *designerButton = new QPushButton("关卡设计师", central);
    QPushButton *helpButton = new QPushButton("操作说明", central);
    QPushButton *exitButton = new QPushButton("退出游戏", central);

    QString buttonStyle =
        "QPushButton {"
        "background-color: #2d3348;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 10px;"
        "padding: 14px 40px;"
        "font-size: 20px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #3a86ff;"
        "}";

    startButton->setStyleSheet(buttonStyle);
    levelButton->setStyleSheet(buttonStyle);
    designerButton->setStyleSheet(buttonStyle);
    helpButton->setStyleSheet(buttonStyle);
    exitButton->setStyleSheet(buttonStyle);

    startButton->setFixedWidth(260);
    levelButton->setFixedWidth(260);
    designerButton->setFixedWidth(260);
    helpButton->setFixedWidth(260);
    exitButton->setFixedWidth(260);

    mainLayout->addStretch();
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subTitleLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(startButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(levelButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(designerButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(helpButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(exitButton, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    setCentralWidget(central);

    connect(startButton, &QPushButton::clicked, this, [this]() {
        setupGameWindow(1);
    });

    connect(levelButton, &QPushButton::clicked, this, [this]() {
        showLevelSelectDialog();
    });

    connect(designerButton, &QPushButton::clicked, this, [this]() {
        showLevelEditorDialog();
    });

    connect(helpButton, &QPushButton::clicked, this, [this]() {
        showHelpDialog();
    });

    connect(exitButton, &QPushButton::clicked, this, [this]() {
        close();
    });
}

void MainWindow::setupGameWindow(int startLevelNumber)
{
    clearGameScene();

    setWindowTitle("KristenLab - 游戏中");
    resize(1280, 820);

    QWidget *central = new QWidget(this);
    central->setStyleSheet(
        "background-color: #151821;"
        "color: white;"
        "font-family: Microsoft YaHei;"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    QFrame *statusFrame = new QFrame(central);
    statusFrame->setObjectName("statusFrame");
    statusFrame->setStyleSheet(
        "#statusFrame {"
        "background-color: #202638;"
        "border: 1px solid #33415c;"
        "border-radius: 8px;"
        "}"
        "QLabel {"
        "font-size: 15px;"
        "font-weight: bold;"
        "color: #e6edf3;"
        "}"
        );

    QHBoxLayout *statusLayout = new QHBoxLayout(statusFrame);
    statusLayout->setContentsMargins(16, 8, 16, 8);

    levelLabel = new QLabel("关卡：1/5", statusFrame);
    gravityLabel = new QLabel("重力：↓", statusFrame);
    timeLabel = new QLabel("时间：0.0s", statusFrame);
    reverseLabel = new QLabel("反转：0", statusFrame);
    deathLabel = new QLabel("死亡：0", statusFrame);
    stateLabel = new QLabel("状态：运行中", statusFrame);
    viewZoomLabel = new QLabel("视图：100%", statusFrame);

    statusLayout->addWidget(levelLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(gravityLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(timeLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(reverseLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(deathLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(stateLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(viewZoomLabel);

    mainLayout->addWidget(statusFrame);

    gameScene = new GameScene(this);

    connect(gameScene,
            &GameScene::statusChanged,
            this,
            [this](const QString &levelText,
                   const QString &gravityText,
                   const QString &timeText,
                   const QString &reverseText,
                   const QString &deathText,
                   const QString &stateText) {
                if (levelLabel != nullptr) {
                    levelLabel->setText(levelText);
                }

                if (gravityLabel != nullptr) {
                    gravityLabel->setText(gravityText);
                }

                if (timeLabel != nullptr) {
                    timeLabel->setText(timeText);
                }

                if (reverseLabel != nullptr) {
                    reverseLabel->setText(reverseText);
                }

                if (deathLabel != nullptr) {
                    deathLabel->setText(deathText);
                }

                if (stateLabel != nullptr) {
                    stateLabel->setText(stateText);
                }
            });

    gameScene->loadLevelByNumber(startLevelNumber);
    gameScene->refreshStatus();

    gameView = new QGraphicsView(gameScene, central);
    gameView->setRenderHint(QPainter::Antialiasing);

    // 大地图需要保留双向滚动条。
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    gameView->setAlignment(Qt::AlignCenter);

    gameView->setFocusPolicy(Qt::StrongFocus);
    gameView->setFocus();
    gameScene->setFocus();

    gameView->setStyleSheet(
        "QGraphicsView {"
        "background-color: #10131f;"
        "border: 2px solid #3a86ff;"
        "border-radius: 8px;"
        "}"
        "QScrollBar:horizontal, QScrollBar:vertical {"
        "background: #10131f;"
        "border: 1px solid #33415c;"
        "}"
        "QScrollBar::handle:horizontal, QScrollBar::handle:vertical {"
        "background: #4a5568;"
        "border-radius: 4px;"
        "}"
        );

    mainLayout->addWidget(gameView, 1);

    QFrame *buttonFrame = new QFrame(central);
    buttonFrame->setObjectName("buttonFrame");
    buttonFrame->setStyleSheet(
        "#buttonFrame {"
        "background-color: #202638;"
        "border: 1px solid #33415c;"
        "border-radius: 8px;"
        "}"
        "QPushButton {"
        "background-color: #2d3348;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 6px;"
        "padding: 7px 10px;"
        "font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "background-color: #3a86ff;"
        "}"
        );

    QGridLayout *buttonLayout = new QGridLayout(buttonFrame);
    buttonLayout->setContentsMargins(14, 8, 14, 8);
    buttonLayout->setHorizontalSpacing(8);
    buttonLayout->setVerticalSpacing(8);

    QPushButton *editButton = new QPushButton("进入编辑模式", buttonFrame);
    QPushButton *slowButton = new QPushButton("缓冲区", buttonFrame);
    QPushButton *laserButton = new QPushButton("激光门", buttonFrame);
    QPushButton *trampolineUpRightButton = new QPushButton("蹦床↗", buttonFrame);
    QPushButton *trampolineUpLeftButton = new QPushButton("蹦床↖", buttonFrame);
    QPushButton *trampolineDownRightButton = new QPushButton("蹦床↘", buttonFrame);
    QPushButton *trampolineDownLeftButton = new QPushButton("蹦床↙", buttonFrame);
    QPushButton *trampolineRightButton = new QPushButton("蹦床→", buttonFrame);
    QPushButton *trampolineLeftButton = new QPushButton("蹦床←", buttonFrame);
    QPushButton *runButton = new QPushButton("开始运行", buttonFrame);
    QPushButton *saveButton = new QPushButton("保存地图", buttonFrame);

    QPushButton *zoomOutButton = new QPushButton("缩小视图", buttonFrame);
    QPushButton *zoomInButton = new QPushButton("放大视图", buttonFrame);
    QPushButton *resetZoomButton = new QPushButton("还原视图", buttonFrame);
    QPushButton *fitZoomButton = new QPushButton("适应窗口", buttonFrame);

    QPushButton *previousButton = new QPushButton("上一关", buttonFrame);
    QPushButton *restartButton = new QPushButton("重开", buttonFrame);
    QPushButton *pauseButton = new QPushButton("暂停", buttonFrame);
    QPushButton *nextButton = new QPushButton("下一关", buttonFrame);
    QPushButton *menuButton = new QPushButton("返回主菜单", buttonFrame);

    QList<QPushButton *> buttons = {
        editButton,
        slowButton,
        laserButton,
        trampolineUpRightButton,
        trampolineUpLeftButton,
        trampolineDownRightButton,
        trampolineDownLeftButton,
        trampolineRightButton,
        trampolineLeftButton,
        runButton,
        saveButton,
        zoomOutButton,
        zoomInButton,
        resetZoomButton,
        fitZoomButton,
        previousButton,
        restartButton,
        pauseButton,
        nextButton,
        menuButton
    };

    for (QPushButton *button : buttons) {
        button->setFocusPolicy(Qt::NoFocus);
        button->setMinimumHeight(34);
    }

    buttonLayout->addWidget(editButton, 0, 0);
    buttonLayout->addWidget(slowButton, 0, 1);
    buttonLayout->addWidget(laserButton, 0, 2);
    buttonLayout->addWidget(trampolineUpRightButton, 0, 3);
    buttonLayout->addWidget(trampolineUpLeftButton, 0, 4);
    buttonLayout->addWidget(trampolineDownRightButton, 0, 5);
    buttonLayout->addWidget(trampolineDownLeftButton, 0, 6);
    buttonLayout->addWidget(trampolineRightButton, 0, 7);
    buttonLayout->addWidget(trampolineLeftButton, 0, 8);
    buttonLayout->addWidget(runButton, 0, 9);
    buttonLayout->addWidget(saveButton, 0, 10);

    buttonLayout->addWidget(zoomOutButton, 1, 0);
    buttonLayout->addWidget(zoomInButton, 1, 1);
    buttonLayout->addWidget(resetZoomButton, 1, 2);
    buttonLayout->addWidget(fitZoomButton, 1, 3);

    buttonLayout->addWidget(previousButton, 1, 4);
    buttonLayout->addWidget(restartButton, 1, 5);
    buttonLayout->addWidget(pauseButton, 1, 6);
    buttonLayout->addWidget(nextButton, 1, 7);
    buttonLayout->addWidget(menuButton, 1, 8, 1, 2);

    mainLayout->addWidget(buttonFrame);

    setCentralWidget(central);

    connect(previousButton, &QPushButton::clicked, this, [this]() {
        gameScene->previousLevel();
        autoFitGameViewZoom();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(restartButton, &QPushButton::clicked, this, [this]() {
        gameScene->restartLevel();
        autoFitGameViewZoom();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(pauseButton, &QPushButton::clicked, this, [this]() {
        gameScene->togglePause();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(nextButton, &QPushButton::clicked, this, [this]() {
        gameScene->nextLevel();
        autoFitGameViewZoom();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(menuButton, &QPushButton::clicked, this, [this]() {
        setupMainMenu();
    });

    connect(editButton, &QPushButton::clicked, this, [this]() {
        gameScene->enterEditMode();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(slowButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectSlowBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(laserButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectLaserBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineUpRightButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineUpRightBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineUpLeftButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineUpLeftBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineDownRightButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineDownRightBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineDownLeftButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineDownLeftBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineRightButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineRightBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineLeftButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineLeftBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(runButton, &QPushButton::clicked, this, [this]() {
        gameScene->startRunMode();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        if (gameScene != nullptr) {
            gameScene->saveCurrentEditedLevel();
        }

        if (gameView != nullptr) {
            gameView->setFocus();
        }

        if (gameScene != nullptr) {
            gameScene->setFocus();
        }
    });

    connect(zoomOutButton, &QPushButton::clicked, this, [this]() {
        zoomGameViewOut();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(zoomInButton, &QPushButton::clicked, this, [this]() {
        zoomGameViewIn();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(resetZoomButton, &QPushButton::clicked, this, [this]() {
        resetGameViewZoom();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(fitZoomButton, &QPushButton::clicked, this, [this]() {
        autoFitGameViewZoom();
        gameView->setFocus();
        gameScene->setFocus();
    });

    QTimer::singleShot(0, this, &MainWindow::autoFitGameViewZoom);
}

void MainWindow::setGameViewScale(double scale)
{
    gameViewScale = qBound(0.05, scale, 4.0);
    applyGameViewZoom();
}

void MainWindow::applyGameViewZoom()
{
    if (gameView == nullptr) {
        return;
    }

    QTransform transform;
    transform.scale(gameViewScale, gameViewScale);
    gameView->setTransform(transform);

    if (viewZoomLabel != nullptr) {
        viewZoomLabel->setText(QString("视图：%1%").arg(qRound(gameViewScale * 100)));
    }
}

void MainWindow::zoomGameViewIn()
{
    setGameViewScale(gameViewScale * 1.25);
}

void MainWindow::zoomGameViewOut()
{
    setGameViewScale(gameViewScale / 1.25);
}

void MainWindow::resetGameViewZoom()
{
    setGameViewScale(1.0);

    if (gameView != nullptr && gameScene != nullptr) {
        gameView->centerOn(gameScene->sceneRect().center());
    }
}

void MainWindow::autoFitGameViewZoom()
{
    if (gameView == nullptr || gameScene == nullptr) {
        return;
    }

    QRectF sceneRect = gameScene->sceneRect();

    if (sceneRect.width() <= 0 || sceneRect.height() <= 0) {
        return;
    }

    QSize viewportSize = gameView->viewport()->size();

    if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
        return;
    }

    const double availableWidth = qMax(1, viewportSize.width() - 24);
    const double availableHeight = qMax(1, viewportSize.height() - 24);

    double scaleX = availableWidth / sceneRect.width();
    double scaleY = availableHeight / sceneRect.height();

    double fitScale = qMin(scaleX, scaleY);

    // 小地图保持原尺寸，大地图自动适配视图。
    fitScale = qMin(fitScale, 1.0);
    setGameViewScale(fitScale);

    gameView->centerOn(sceneRect.center());
}

void MainWindow::showLevelEditorDialog()
{
    LevelEditorDialog dialog(this);

    connect(&dialog, &LevelEditorDialog::requestOpenLevelSelect, this, [this, &dialog]() {
        dialog.accept();
        showLevelSelectDialog();
    });

    dialog.exec();
}

void MainWindow::showLevelSelectDialog()
{
    // 打开关卡选择时重新扫描关卡目录。
    LevelManager previewManager;
    previewManager.loadDefaultLevels();

    QDialog dialog(this);
    dialog.setWindowTitle("关卡选择");
    dialog.resize(460, 560);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    QLabel *titleLabel = new QLabel("请选择关卡", &dialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #80f7ff;"
        );

    QLabel *hintLabel = new QLabel(
        "关卡列表会根据 levels 和 custom_levels 文件夹自动生成",
        &dialog
        );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet(
        "font-size: 12px;"
        "color: #cbd5e1;"
        );

    layout->addWidget(titleLabel);
    layout->addWidget(hintLabel);

    QScrollArea *scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollWidget = new QWidget(scrollArea);
    scrollWidget->setObjectName("levelScrollWidget");

    QVBoxLayout *levelLayout = new QVBoxLayout(scrollWidget);
    levelLayout->setContentsMargins(4, 4, 4, 4);
    levelLayout->setSpacing(8);

    auto addSectionTitle = [&](const QString &text) {
        QLabel *sectionLabel = new QLabel(text, scrollWidget);
        sectionLabel->setObjectName("sectionLabel");
        sectionLabel->setMinimumHeight(28);
        sectionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        levelLayout->addWidget(sectionLabel);
    };

    int totalCount = previewManager.levelCount();

    if (totalCount <= 0) {
        QLabel *emptyLabel = new QLabel(
            "没有可用关卡。\n请检查 levels 和 custom_levels 文件夹。",
            scrollWidget
            );
        emptyLabel->setWordWrap(true);
        emptyLabel->setAlignment(Qt::AlignCenter);

        levelLayout->addWidget(emptyLabel);
    } else {
        auto addLevelButton = [&](int index) {
            Level level = previewManager.levelAt(index);
            QString buttonText = previewManager.levelSelectTextAt(index);

            QPushButton *levelButton = new QPushButton(buttonText, scrollWidget);
            levelButton->setMinimumHeight(42);
            levelButton->setFocusPolicy(Qt::NoFocus);
            levelButton->setProperty("customLevel", level.isCustomLevel);

            int levelNumber = index + 1;

            connect(levelButton, &QPushButton::clicked, this, [this, levelNumber, &dialog]() {
                dialog.accept();

                setupGameWindow(levelNumber);

                if (gameView != nullptr) {
                    gameView->setFocus();
                }

                if (gameScene != nullptr) {
                    gameScene->setFocus();
                }
            });

            levelLayout->addWidget(levelButton);
        };

        bool hasBuiltInLevel = false;
        bool hasCustomLevel = false;

        for (int index = 0; index < totalCount; ++index) {
            Level level = previewManager.levelAt(index);

            if (level.isCustomLevel) {
                hasCustomLevel = true;
            } else {
                hasBuiltInLevel = true;
            }
        }

        if (hasBuiltInLevel) {
            addSectionTitle("内置关卡");

            for (int index = 0; index < totalCount; ++index) {
                Level level = previewManager.levelAt(index);

                if (level.isCustomLevel) {
                    continue;
                }

                addLevelButton(index);
            }
        }

        if (hasCustomLevel) {
            addSectionTitle("自定义关卡");

            for (int index = 0; index < totalCount; ++index) {
                Level level = previewManager.levelAt(index);

                if (!level.isCustomLevel) {
                    continue;
                }

                addLevelButton(index);
            }
        }
    }

    levelLayout->addStretch();

    scrollArea->setWidget(scrollWidget);
    layout->addWidget(scrollArea, 1);

    QPushButton *closeButton = new QPushButton("关闭", &dialog);
    closeButton->setMinimumHeight(38);
    closeButton->setFocusPolicy(Qt::NoFocus);

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    layout->addWidget(closeButton);

    dialog.setStyleSheet(
        "QDialog {"
        "background-color: #151821;"
        "color: white;"
        "font-family: Microsoft YaHei;"
        "}"
        "QScrollArea {"
        "background: transparent;"
        "border: none;"
        "}"
        "QWidget#levelScrollWidget {"
        "background: transparent;"
        "}"
        "QLabel {"
        "color: white;"
        "}"
        "QLabel#sectionLabel {"
        "font-size: 15px;"
        "font-weight: bold;"
        "color: #f1fa8c;"
        "margin-top: 8px;"
        "}"
        "QPushButton {"
        "background-color: #2d3348;"
        "color: white;"
        "border: 1px solid #4a5568;"
        "border-radius: 8px;"
        "padding: 8px;"
        "font-size: 14px;"
        "text-align: left;"
        "}"
        "QPushButton:hover {"
        "background-color: #3a86ff;"
        "}"
        "QPushButton[customLevel=\"true\"] {"
        "background-color: #264653;"
        "border: 1px solid #2a9d8f;"
        "color: #e0fffa;"
        "}"
        "QPushButton[customLevel=\"true\"]:hover {"
        "background-color: #2a9d8f;"
        "color: white;"
        "}"
        );

    dialog.exec();
}

void MainWindow::showHelpDialog()
{
    QMessageBox::information(
        this,
        "操作说明",
        "方向键：切换重力方向\n"
        "R：重新开始当前关\n"
        "Space：暂停 / 继续\n"
        "P：上一关\n"
        "N：下一关\n\n"
        "地图元素：\n"
        "绿色 END：终点\n"
        "红色方块：死亡区\n"
        "橙色 ↑：弹射块\n"
        "蓝色 S：缓冲区\n"
        "黄色 >>：传送带\n"
        "红色 L：激光门，周期亮灭；亮时会反弹，灭时可通过\n"
        "青色圆点：数据碎片\n\n"
        "目标：引导小球到达终点，并尽量减少反转次数、收集数据碎片。"
        );
}

