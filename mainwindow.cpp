#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "gamescene.h"
#include "levelmanager.h"
#include "leveleditordialog.h"

#include <QDebug>
#include <QDialog>
#include <QFrame>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollArea>

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
    resize(1000, 700);

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
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
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
        "padding: 8px 20px;"
        "font-size: 15px;"
        "}"
        "QPushButton:hover {"
        "background-color: #3a86ff;"
        "}"
        );

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonFrame);
    buttonLayout->setContentsMargins(16, 8, 16, 8);

    QPushButton *editButton = new QPushButton("进入编辑模式", buttonFrame);
    QPushButton *bounceButton = new QPushButton("弹射块", buttonFrame);
    QPushButton *slowButton = new QPushButton("缓冲区", buttonFrame);
    QPushButton *conveyorButton = new QPushButton("传送带", buttonFrame);
    QPushButton *trampolineButton = new QPushButton("蹦床", buttonFrame);
    QPushButton *runButton = new QPushButton("开始运行", buttonFrame);
    QPushButton *saveButton = new QPushButton("保存地图", buttonFrame);

    QPushButton *previousButton = new QPushButton("上一关", buttonFrame);
    QPushButton *restartButton = new QPushButton("重开", buttonFrame);
    QPushButton *pauseButton = new QPushButton("暂停", buttonFrame);
    QPushButton *nextButton = new QPushButton("下一关", buttonFrame);
    QPushButton *menuButton = new QPushButton("返回主菜单", buttonFrame);


    previousButton->setFocusPolicy(Qt::NoFocus);
    restartButton->setFocusPolicy(Qt::NoFocus);
    pauseButton->setFocusPolicy(Qt::NoFocus);
    nextButton->setFocusPolicy(Qt::NoFocus);
    menuButton->setFocusPolicy(Qt::NoFocus);
    saveButton->setFocusPolicy(Qt::NoFocus);
    trampolineButton->setFocusPolicy(Qt::NoFocus);

    buttonLayout->addStretch();

    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(bounceButton);
    buttonLayout->addWidget(slowButton);
    buttonLayout->addWidget(conveyorButton);
    buttonLayout->addWidget(trampolineButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(bounceButton);
    buttonLayout->addWidget(slowButton);
    buttonLayout->addWidget(conveyorButton);
    buttonLayout->addWidget(trampolineButton);
    buttonLayout->addWidget(runButton);
    buttonLayout->addWidget(saveButton);

    buttonLayout->addSpacing(20);

    buttonLayout->addWidget(previousButton);
    buttonLayout->addWidget(restartButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addWidget(nextButton);
    buttonLayout->addWidget(menuButton);

    buttonLayout->addStretch();

    mainLayout->addWidget(buttonFrame);

    setCentralWidget(central);

    connect(previousButton, &QPushButton::clicked, this, [this]() {
        gameScene->previousLevel();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(restartButton, &QPushButton::clicked, this, [this]() {
        gameScene->restartLevel();
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

    connect(bounceButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectBounceBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(slowButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectSlowBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(conveyorButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectConveyorBlock();
        gameView->setFocus();
        gameScene->setFocus();
    });

    connect(trampolineButton, &QPushButton::clicked, this, [this]() {
        gameScene->selectTrampolineBlock();
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
    // 阶段 22：
    // 每次打开关卡选择时，都重新扫描 levels 和 custom_levels。
    // 这样新增 / 删除自定义地图后，界面会自动更新。
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
        "青色圆点：数据碎片\n\n"
        "目标：引导小球到达终点，并尽量减少反转次数、收集数据碎片。"
        );
}

