#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "gamescene.h"
#include "levelmanager.h"
#include "level.h"
#include "leveleditordialog.h"
#include "progressmanager.h"

#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QList>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollArea>
#include <QTimer>
#include <QTransform>
#include "audioplayer.h"

namespace {

// 游戏视图专用背景层：背景固定在视口中，不随关卡缩放或滚动。
// GameScene 仍只负责关卡、碰撞和机关，因此这项改造不会改变游戏逻辑。
class BackgroundGraphicsView final : public QGraphicsView
{
public:
    explicit BackgroundGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr)
        : QGraphicsView(scene, parent)
        , backgroundPixmap(":/images/resources/images/game_interface_background.png")
    {
        // 固定背景在缩放和滚动时需要整块刷新，避免局部更新留下残影。
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setAttribute(Qt::WA_OpaquePaintEvent, false);

        if (viewport() != nullptr) {
            viewport()->setAutoFillBackground(false);
            viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
        }
    }

    // 返回关卡在“自适应窗口”模式下可使用的最大显示区域。
    // 地图、终端背板和窗口使用同一套动态边距，避免大小比例脱节。
    QRect mapSafeRect() const
    {
        if (viewport() == nullptr) {
            return QRect();
        }

        const QRect viewportRect(QPoint(0, 0), viewport()->size());
        const int outerX = qBound(22, qRound(viewportRect.width() * 0.035), 58);
        const int outerY = qBound(18, qRound(viewportRect.height() * 0.035), 36);
        const int frameSide = qBound(30, qRound(viewportRect.width() * 0.035), 52);
        const int frameTop = qBound(48, qRound(viewportRect.height() * 0.080), 68);
        const int frameBottom = qBound(26, qRound(viewportRect.height() * 0.045), 42);

        return viewportRect.adjusted(
            outerX + frameSide,
            outerY + frameTop,
            -(outerX + frameSide),
            -(outerY + frameBottom)
            );
    }

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override
    {
        Q_UNUSED(rect);

        if (painter == nullptr || viewport() == nullptr) {
            return;
        }

        painter->save();
        painter->resetTransform();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter->setRenderHint(QPainter::Antialiasing, true);

        const QRect viewportRect(QPoint(0, 0), viewport()->size());
        painter->fillRect(viewportRect, QColor("#07101a"));

        if (!backgroundPixmap.isNull()) {
            // 按 KeepAspectRatioByExpanding 的方式铺满视口，窗口变化时不会拉伸变形。
            const double scaleX = viewportRect.width() / double(backgroundPixmap.width());
            const double scaleY = viewportRect.height() / double(backgroundPixmap.height());
            const double scale = qMax(scaleX, scaleY);
            const QSize drawSize(
                qRound(backgroundPixmap.width() * scale),
                qRound(backgroundPixmap.height() * scale)
                );
            const QRect targetRect(
                viewportRect.center().x() - drawSize.width() / 2,
                viewportRect.center().y() - drawSize.height() / 2,
                drawSize.width(),
                drawSize.height()
                );
            painter->drawPixmap(targetRect, backgroundPixmap);
        }

        // 压暗复杂背景，保证角色、机关和网格始终清晰。
        painter->fillRect(viewportRect, QColor(2, 7, 14, 42));

        // 悬浮终端背板按当前地图在视口中的真实大小自动包裹，避免与地图比例脱节。
        const QRect boardRect = terminalRect(viewportRect);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 4, 9, 190));
        painter->drawRoundedRect(boardRect.adjusted(-20, -18, 20, 22), 22, 22);

        painter->setBrush(QColor(24, 31, 38, 248));
        painter->setPen(QPen(QColor(109, 126, 139, 235), 8));
        painter->drawRoundedRect(boardRect, 14, 14);

        painter->setBrush(QColor(5, 12, 22, 245));
        painter->setPen(QPen(QColor(30, 49, 66, 245), 3));
        painter->drawRoundedRect(boardRect.adjusted(18, 34, -18, -18), 8, 8);

        // 顶部细条与四角装饰让终端更接近参考图的工业科技风格。
        const QRect headerRect(
            boardRect.left() + 34,
            boardRect.top() + 18,
            boardRect.width() - 68,
            12
            );
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(111, 133, 143, 190));
        painter->drawRoundedRect(headerRect, 4, 4);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(94, 153, 199, 205), 3));
        const int bracket = 34;
        painter->drawLine(boardRect.left() + 12, boardRect.top() + bracket,
                          boardRect.left() + 12, boardRect.top() + 12);
        painter->drawLine(boardRect.left() + 12, boardRect.top() + 12,
                          boardRect.left() + bracket, boardRect.top() + 12);
        painter->drawLine(boardRect.right() - bracket, boardRect.top() + 12,
                          boardRect.right() - 12, boardRect.top() + 12);
        painter->drawLine(boardRect.right() - 12, boardRect.top() + 12,
                          boardRect.right() - 12, boardRect.top() + bracket);
        painter->drawLine(boardRect.left() + 12, boardRect.bottom() - bracket,
                          boardRect.left() + 12, boardRect.bottom() - 12);
        painter->drawLine(boardRect.left() + 12, boardRect.bottom() - 12,
                          boardRect.left() + bracket, boardRect.bottom() - 12);
        painter->drawLine(boardRect.right() - bracket, boardRect.bottom() - 12,
                          boardRect.right() - 12, boardRect.bottom() - 12);
        painter->drawLine(boardRect.right() - 12, boardRect.bottom() - 12,
                          boardRect.right() - 12, boardRect.bottom() - bracket);

        painter->restore();
    }

    void drawForeground(QPainter *painter, const QRectF &rect) override
    {
        Q_UNUSED(rect);

        if (painter == nullptr || scene() == nullptr) {
            return;
        }

        const QRectF levelRect = scene()->sceneRect();
        if (!levelRect.isValid() || levelRect.isEmpty()) {
            return;
        }

        // 边框跟随真实关卡缩放和滚动，背景本身则保持固定。
        const QRect mapRect = mapFromScene(levelRect).boundingRect().adjusted(-12, -12, 12, 12);

        painter->save();
        painter->resetTransform();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(Qt::NoBrush);

        painter->setPen(QPen(QColor(0, 5, 11, 205), 14));
        painter->drawRoundedRect(mapRect.adjusted(-3, -3, 3, 3), 10, 10);
        painter->setPen(QPen(QColor(104, 139, 169, 235), 6));
        painter->drawRoundedRect(mapRect, 8, 8);
        painter->setPen(QPen(QColor(35, 65, 91, 245), 2));
        painter->drawRoundedRect(mapRect.adjusted(4, 4, -4, -4), 6, 6);

        painter->restore();
    }

private:
    QRect terminalRect(const QRect &viewportRect) const
    {
        const QRect safeRect = viewportRect.adjusted(18, 16, -18, -16);
        const int sidePadding = qBound(22, qRound(viewportRect.width() * 0.022), 34);
        const int topPadding = qBound(46, qRound(viewportRect.height() * 0.070), 62);
        const int bottomPadding = qBound(22, qRound(viewportRect.height() * 0.035), 34);

        QRect desiredRect;

        if (scene() != nullptr) {
            const QRectF levelSceneRect = scene()->sceneRect();
            if (levelSceneRect.isValid() && !levelSceneRect.isEmpty()) {
                const QRect mapRect = mapFromScene(levelSceneRect).boundingRect();
                desiredRect = mapRect.adjusted(
                    -sidePadding,
                    -topPadding,
                    sidePadding,
                    bottomPadding
                    );
            }
        }

        if (!desiredRect.isValid() || desiredRect.isEmpty()) {
            const QSize fallbackSize(
                qMin(640, qMax(280, safeRect.width())),
                qMin(400, qMax(200, safeRect.height()))
                );
            desiredRect = QRect(QPoint(0, 0), fallbackSize);
            desiredRect.moveCenter(viewportRect.center());
        }

        // 手动放大或滚动时，背板跟随地图，但始终限制在游戏视口内。
        const int width = qMin(desiredRect.width(), qMax(1, safeRect.width()));
        const int height = qMin(desiredRect.height(), qMax(1, safeRect.height()));
        const int maxLeft = qMax(safeRect.left(), safeRect.right() - width + 1);
        const int maxTop = qMax(safeRect.top(), safeRect.bottom() - height + 1);
        const int left = qBound(safeRect.left(), desiredRect.left(), maxLeft);
        const int top = qBound(safeRect.top(), desiredRect.top(), maxTop);

        return QRect(left, top, width, height);
    }

    QPixmap backgroundPixmap;
};

} // namespace

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
    , inputIndicatorWidget(nullptr)
    , wKeyLabel(nullptr)
    , aKeyLabel(nullptr)
    , sKeyLabel(nullptr)
    , dKeyLabel(nullptr)
    , gameViewScale(1.0)
    , gameViewAutoFitPending(false)
    , gameViewAutoFitEnabled(true)
    , bgmPlayer(nullptr)
{
    ui->setupUi(this);

    setupBackgroundMusic();
    setupMainMenu();
}

MainWindow::~MainWindow()
{
    clearGameScene();
    delete ui;
}

void MainWindow::clearGameScene()
{
    if (gameView != nullptr && gameView->viewport() != nullptr) {
        gameView->viewport()->removeEventFilter(this);
    }

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
    inputIndicatorWidget = nullptr;
    wKeyLabel = nullptr;
    aKeyLabel = nullptr;
    sKeyLabel = nullptr;
    dKeyLabel = nullptr;
    gameViewScale = 1.0;
    gameViewAutoFitPending = false;
    gameViewAutoFitEnabled = true;
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
    central->setObjectName("gamePage");
    central->setStyleSheet(
        "#gamePage {"
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "stop:0 #111827, stop:1 #070b12);"
        "color: white;"
        "font-family: Microsoft YaHei;"
        "}"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    QFrame *statusFrame = new QFrame(central);
    statusFrame->setObjectName("statusFrame");
    statusFrame->setStyleSheet(
        "#statusFrame {"
        "background-color: rgba(17, 27, 47, 235);"
        "border: 1px solid rgba(88, 127, 188, 175);"
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

    gameView = new BackgroundGraphicsView(gameScene, central);
    gameView->setRenderHint(QPainter::Antialiasing);

    // 通过键盘切换关卡或其他方式改变场景尺寸时，也重新进入自动适配模式。
    connect(gameScene, &QGraphicsScene::sceneRectChanged, this, [this](const QRectF &) {
        gameViewAutoFitEnabled = true;
        scheduleAutoFitGameViewZoom();
    });

    // 自适应状态下隐藏不需要的滚动条；手动放大或大地图超出视口时自动出现。
    gameView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    gameView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    gameView->setAlignment(Qt::AlignCenter);

    gameView->setFocusPolicy(Qt::StrongFocus);
    gameView->setFocus();
    gameScene->setFocus();

    gameView->setStyleSheet(
        "QGraphicsView {"
        "background: transparent;"
        "border: 2px solid rgba(73, 126, 220, 215);"
        "border-radius: 8px;"
        "}"
        "QScrollBar:horizontal, QScrollBar:vertical {"
        "background: rgba(7, 13, 23, 225);"
        "border: 1px solid rgba(68, 96, 143, 190);"
        "}"
        "QScrollBar::handle:horizontal, QScrollBar::handle:vertical {"
        "background: rgba(96, 124, 169, 225);"
        "border-radius: 4px;"
        "}"
        );

    mainLayout->addWidget(gameView, 1);

    QFrame *buttonFrame = new QFrame(central);
    buttonFrame->setObjectName("buttonFrame");
    buttonFrame->setStyleSheet(
        "#buttonFrame {"
        "background-color: rgba(17, 27, 47, 235);"
        "border: 1px solid rgba(88, 127, 188, 175);"
        "border-radius: 8px;"
        "}"
        "QPushButton {"
        "background-color: rgba(39, 53, 82, 230);"
        "color: white;"
        "border: 1px solid rgba(105, 137, 187, 175);"
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
    createInputIndicator();

    connect(gameScene, &GameScene::inputDirectionChanged, this, [this](const QString &activeKey) {
        updateInputIndicator(activeKey);
    });

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

    scheduleAutoFitGameViewZoom();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (gameView != nullptr
        && watched == gameView->viewport()
        && event->type() == QEvent::Resize) {
        repositionInputIndicator();

        // 最大化、还原或拖动窗口尺寸时，自动重新适配地图和终端背板。
        if (gameViewAutoFitEnabled) {
            scheduleAutoFitGameViewZoom();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::scheduleAutoFitGameViewZoom()
{
    // Resize 事件可能连续触发；只在下一轮事件循环中执行一次适配，减少抖动。
    if (gameViewAutoFitPending) {
        return;
    }

    gameViewAutoFitPending = true;
    QTimer::singleShot(0, this, [this]() {
        // 先释放挂起标记；如果缩放导致滚动条状态变化并再次触发 Resize，
        // 下一轮事件循环还可以补做一次精确适配。
        gameViewAutoFitPending = false;

        if (gameView != nullptr && gameScene != nullptr && gameViewAutoFitEnabled) {
            autoFitGameViewZoom();
            repositionInputIndicator();
        }
    });
}

void MainWindow::createInputIndicator()
{
    if (gameView == nullptr || gameView->viewport() == nullptr) {
        return;
    }

    inputIndicatorWidget = new QFrame(gameView->viewport());
    inputIndicatorWidget->setObjectName("inputIndicatorWidget");
    inputIndicatorWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    inputIndicatorWidget->setStyleSheet(
        "#inputIndicatorWidget {"
        "background-color: rgba(21, 24, 33, 210);"
        "border: 1px solid rgba(128, 247, 255, 120);"
        "border-radius: 10px;"
        "}"
        "QLabel#indicatorTitle {"
        "color: #c9f9ff;"
        "font-size: 12px;"
        "font-weight: bold;"
        "}"
        );

    QVBoxLayout *layout = new QVBoxLayout(inputIndicatorWidget);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);

    QLabel *titleLabel = new QLabel("WASD", inputIndicatorWidget);
    titleLabel->setObjectName("indicatorTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto createKeyLabel = [this]() {
        QLabel *label = new QLabel(inputIndicatorWidget);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedSize(34, 34);
        return label;
    };

    wKeyLabel = createKeyLabel();
    aKeyLabel = createKeyLabel();
    sKeyLabel = createKeyLabel();
    dKeyLabel = createKeyLabel();

    wKeyLabel->setText("W");
    aKeyLabel->setText("A");
    sKeyLabel->setText("S");
    dKeyLabel->setText("D");

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(4);
    gridLayout->setVerticalSpacing(4);
    gridLayout->addWidget(wKeyLabel, 0, 1);
    gridLayout->addWidget(aKeyLabel, 1, 0);
    gridLayout->addWidget(sKeyLabel, 1, 1);
    gridLayout->addWidget(dKeyLabel, 1, 2);

    layout->addLayout(gridLayout);

    gameView->viewport()->installEventFilter(this);

    updateInputIndicator(QString());
    inputIndicatorWidget->adjustSize();
    repositionInputIndicator();
    inputIndicatorWidget->show();
}

void MainWindow::repositionInputIndicator()
{
    if (gameView == nullptr || gameView->viewport() == nullptr || inputIndicatorWidget == nullptr) {
        return;
    }

    inputIndicatorWidget->adjustSize();

    const int margin = 16;
    const QSize indicatorSize = inputIndicatorWidget->sizeHint();
    const int x = qMax(0, gameView->viewport()->width() - indicatorSize.width() - margin);
    inputIndicatorWidget->resize(indicatorSize);
    inputIndicatorWidget->move(x, margin);
    inputIndicatorWidget->raise();
}

void MainWindow::updateInputIndicator(const QString &activeKey)
{
    auto applyKeyStyle = [&activeKey](QLabel *label) {
        if (label == nullptr) {
            return;
        }

        const bool isActive = (label->text() == activeKey);
        label->setStyleSheet(
            isActive
                ? "background-color: #3a86ff;"
                  "color: white;"
                  "border: 1px solid #80f7ff;"
                  "border-radius: 6px;"
                  "font-size: 16px;"
                  "font-weight: bold;"
                : "background-color: rgba(45, 51, 72, 220);"
                  "color: #dce6f2;"
                  "border: 1px solid #4a5568;"
                  "border-radius: 6px;"
                  "font-size: 16px;"
                  "font-weight: bold;"
            );
    };

    applyKeyStyle(wKeyLabel);
    applyKeyStyle(aKeyLabel);
    applyKeyStyle(sKeyLabel);
    applyKeyStyle(dKeyLabel);
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

    if (gameView->viewport() != nullptr) {
        gameView->viewport()->update();
    }

    if (viewZoomLabel != nullptr) {
        viewZoomLabel->setText(QString("视图：%1%").arg(qRound(gameViewScale * 100)));
    }
}

void MainWindow::zoomGameViewIn()
{
    gameViewAutoFitEnabled = false;
    setGameViewScale(gameViewScale * 1.25);
}

void MainWindow::zoomGameViewOut()
{
    gameViewAutoFitEnabled = false;
    setGameViewScale(gameViewScale / 1.25);
}

void MainWindow::resetGameViewZoom()
{
    gameViewAutoFitEnabled = false;
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

    // 地图和终端背板共用同一套动态安全区域，因此会随窗口成套缩放。
    QRect safeRect = QRect(QPoint(0, 0), viewportSize).adjusted(72, 84, -72, -62);
    if (BackgroundGraphicsView *backgroundView = dynamic_cast<BackgroundGraphicsView *>(gameView)) {
        safeRect = backgroundView->mapSafeRect();
    }

    const double availableWidth = qMax(1, safeRect.width());
    const double availableHeight = qMax(1, safeRect.height());

    const double scaleX = availableWidth / sceneRect.width();
    const double scaleY = availableHeight / sceneRect.height();
    double fitScale = qMin(scaleX, scaleY);

    // 小地图允许适度放大；最大 200%，避免像素素材被过度放大。
    fitScale = qBound(0.05, fitScale, 2.0);
    gameViewAutoFitEnabled = true;
    setGameViewScale(fitScale);

    gameView->centerOn(sceneRect.center());

    if (gameView->viewport() != nullptr) {
        gameView->viewport()->update();
    }
}

void MainWindow::setupGameWindowForTestLevel(const Level &level)
{
    setupGameWindow(1);

    if (gameScene != nullptr) {
        gameScene->loadTemporaryLevelForTest(level);
        gameScene->refreshStatus();
        gameViewAutoFitEnabled = true;
        scheduleAutoFitGameViewZoom();
    }
}

void MainWindow::showLevelEditorDialog()
{
    LevelEditorDialog dialog(this);

    connect(&dialog, &LevelEditorDialog::requestOpenLevelSelect, this, [this, &dialog]() {
        dialog.accept();
        showLevelSelectDialog();
    });

    connect(&dialog, &LevelEditorDialog::requestTestLevel, this, [this, &dialog](const Level &level) {
        dialog.accept();
        setupGameWindowForTestLevel(level);
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

    ProgressManager progressManager;

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
        "关卡列表会根据 levels 和 custom_levels 文件夹自动生成；自定义关卡支持试玩、编辑、重命名和删除；通关后会保存星级、最佳用时、最少反转和最少死亡次数",
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
        auto normalizedPath = [](const QString &path) {
            if (path.trimmed().isEmpty()) {
                return QString();
            }

            return QDir::fromNativeSeparators(
                QDir::cleanPath(QFileInfo(path).absoluteFilePath())
                );
        };

        auto reopenLevelSelect = [this, &dialog]() {
            dialog.accept();
            QTimer::singleShot(0, this, [this]() {
                showLevelSelectDialog();
            });
        };

        auto openLevelByIndex = [this, &dialog](int levelNumber) {
            dialog.accept();

            setupGameWindow(levelNumber);

            if (gameView != nullptr) {
                gameView->setFocus();
            }

            if (gameScene != nullptr) {
                gameScene->setFocus();
            }
        };

        auto openCustomLevelEditor = [this, &dialog](const Level &level) {
            dialog.accept();

            LevelEditorDialog editor(this);
            editor.loadLevelForEditing(level, level.sourceFilePath);

            connect(&editor, &LevelEditorDialog::requestOpenLevelSelect, this, [this, &editor]() {
                editor.accept();
                showLevelSelectDialog();
            });

            connect(&editor, &LevelEditorDialog::requestTestLevel, this, [this, &editor](const Level &testLevel) {
                editor.accept();
                setupGameWindowForTestLevel(testLevel);
            });

            editor.exec();
        };

        auto renameCustomLevel = [&, this](const Level &level) {
            bool ok = false;
            QString newName = QInputDialog::getText(
                &dialog,
                QStringLiteral("重命名自定义关卡"),
                QStringLiteral("请输入新的关卡名称："),
                QLineEdit::Normal,
                level.name,
                &ok
                ).trimmed();

            if (!ok || newName.isEmpty() || newName == level.name) {
                return;
            }

            Level renamedLevel = level;
            renamedLevel.name = newName;
            renamedLevel.isCustomLevel = true;

            const QString oldPath = normalizedPath(level.sourceFilePath);
            const QString newPath = normalizedPath(previewManager.customLevelFilePathForName(newName));

            if (newPath.isEmpty()) {
                QMessageBox::warning(&dialog, QStringLiteral("重命名失败"), QStringLiteral("无法计算新的保存路径。"));
                return;
            }

            if (QFileInfo::exists(newPath) && oldPath != newPath) {
                QMessageBox::StandardButton overwrite = QMessageBox::question(
                    &dialog,
                    QStringLiteral("文件已存在"),
                    QStringLiteral("目标文件已经存在：\n%1\n\n是否覆盖它？").arg(newPath),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No
                    );

                if (overwrite != QMessageBox::Yes) {
                    return;
                }
            }

            QString errorMessage;
            if (!previewManager.saveLevelToFile(renamedLevel, newPath, &errorMessage)) {
                QMessageBox::warning(
                    &dialog,
                    QStringLiteral("重命名失败"),
                    QStringLiteral("保存新的关卡文件失败：\n%1").arg(errorMessage)
                    );
                return;
            }

            if (!oldPath.isEmpty() && oldPath != newPath && QFileInfo::exists(oldPath)) {
                QFile oldFile(oldPath);
                if (!oldFile.remove()) {
                    QMessageBox::warning(
                        &dialog,
                        QStringLiteral("旧文件清理失败"),
                        QStringLiteral("新名称已经保存，但旧文件删除失败：\n%1\n\n原因：%2")
                            .arg(oldPath)
                            .arg(oldFile.errorString())
                        );
                }
            }

            QMessageBox::information(
                &dialog,
                QStringLiteral("重命名成功"),
                QStringLiteral("自定义关卡已重命名为：%1").arg(newName)
                );

            reopenLevelSelect();
        };

        auto deleteCustomLevel = [&, this](const Level &level) {
            QMessageBox::StandardButton confirm = QMessageBox::question(
                &dialog,
                QStringLiteral("删除自定义关卡"),
                QStringLiteral("确定要删除这个自定义关卡吗？\n\n%1\n\n该操作会删除 custom_levels 中对应的 JSON 文件，不能从游戏内撤销。").arg(level.name),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
                );

            if (confirm != QMessageBox::Yes) {
                return;
            }

            QString errorMessage;
            if (!previewManager.deleteCustomLevelFile(level, &errorMessage)) {
                QMessageBox::warning(
                    &dialog,
                    QStringLiteral("删除失败"),
                    QStringLiteral("无法删除自定义关卡：\n%1").arg(errorMessage)
                    );
                return;
            }

            QMessageBox::information(
                &dialog,
                QStringLiteral("删除成功"),
                QStringLiteral("自定义关卡已删除：%1").arg(level.name)
                );

            reopenLevelSelect();
        };

        auto addLevelButton = [&](int index) {
            Level level = previewManager.levelAt(index);
            const bool unlocked = level.isCustomLevel || progressManager.isBuiltInLevelUnlocked(index);

            QString buttonText = previewManager.levelSelectTextAt(index);
            buttonText += QStringLiteral("\n");
            buttonText += progressManager.progressTextForLevel(level, index);

            int levelNumber = index + 1;

            if (level.isCustomLevel) {
                QFrame *rowFrame = new QFrame(scrollWidget);
                rowFrame->setObjectName("customLevelRow");
                rowFrame->setProperty("customLevel", true);

                QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
                rowLayout->setContentsMargins(8, 8, 8, 8);
                rowLayout->setSpacing(8);

                QPushButton *playButton = new QPushButton(buttonText, rowFrame);
                playButton->setMinimumHeight(58);
                playButton->setFocusPolicy(Qt::NoFocus);
                playButton->setProperty("customLevel", true);
                playButton->setProperty("customPlayButton", true);

                QPushButton *editButton = new QPushButton(QStringLiteral("编辑"), rowFrame);
                QPushButton *renameButton = new QPushButton(QStringLiteral("重命名"), rowFrame);
                QPushButton *deleteButton = new QPushButton(QStringLiteral("删除"), rowFrame);

                QList<QPushButton *> actionButtons;
                actionButtons << editButton << renameButton << deleteButton;
                for (QPushButton *button : actionButtons) {
                    button->setMinimumHeight(42);
                    button->setMinimumWidth(72);
                    button->setFocusPolicy(Qt::NoFocus);
                    button->setProperty("customAction", true);
                }
                deleteButton->setProperty("dangerAction", true);

                connect(playButton, &QPushButton::clicked, this, [openLevelByIndex, levelNumber]() {
                    openLevelByIndex(levelNumber);
                });

                connect(editButton, &QPushButton::clicked, this, [openCustomLevelEditor, level]() {
                    openCustomLevelEditor(level);
                });

                connect(renameButton, &QPushButton::clicked, this, [renameCustomLevel, level]() {
                    renameCustomLevel(level);
                });

                connect(deleteButton, &QPushButton::clicked, this, [deleteCustomLevel, level]() {
                    deleteCustomLevel(level);
                });

                rowLayout->addWidget(playButton, 1);
                rowLayout->addWidget(editButton);
                rowLayout->addWidget(renameButton);
                rowLayout->addWidget(deleteButton);

                levelLayout->addWidget(rowFrame);
                return;
            }

            QPushButton *levelButton = new QPushButton(buttonText, scrollWidget);
            levelButton->setMinimumHeight(58);
            levelButton->setFocusPolicy(Qt::NoFocus);
            levelButton->setProperty("customLevel", level.isCustomLevel);
            levelButton->setProperty("lockedLevel", !unlocked);
            levelButton->setEnabled(unlocked);

            if (!unlocked) {
                levelButton->setToolTip(QStringLiteral("请先通关前一关来解锁当前关卡"));
            }

            connect(levelButton, &QPushButton::clicked, this, [openLevelByIndex, levelNumber]() {
                openLevelByIndex(levelNumber);
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
        "QPushButton[lockedLevel=\"true\"] {"
        "background-color: #1f2433;"
        "border: 1px solid #343b50;"
        "color: #7d869c;"
        "}"
        "QFrame#customLevelRow {"
        "background-color: rgba(38, 70, 83, 0.35);"
        "border: 1px solid rgba(42, 157, 143, 0.55);"
        "border-radius: 10px;"
        "}"
        "QPushButton[customAction=\"true\"] {"
        "background-color: #334155;"
        "border: 1px solid #64748b;"
        "text-align: center;"
        "font-size: 13px;"
        "padding: 6px;"
        "}"
        "QPushButton[customAction=\"true\"]:hover {"
        "background-color: #3a86ff;"
        "}"
        "QPushButton[dangerAction=\"true\"] {"
        "background-color: #4a1d2b;"
        "border: 1px solid #b91c1c;"
        "}"
        "QPushButton[dangerAction=\"true\"]:hover {"
        "background-color: #dc2626;"
        "}"
        );

    dialog.exec();
}

void MainWindow::showHelpDialog()
{
    QMessageBox::information(
        this,
        "操作说明",
        "W / A / S / D：上 / 左 / 下 / 右移动，切换对应重力方向\n"
        "方向键：保留同样的上下左右操作\n"
        "右上角 WASD 指示器：高亮显示当前触发的方向键，便于路演展示\n"
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

void MainWindow::setupBackgroundMusic()
{
    bgmPlayer = new AudioPlayer();

    QString musicPath = findBackgroundMusicPath();

    if (!musicPath.isEmpty()) {
        if (bgmPlayer->play(musicPath, true)) {
            bgmPlayer->setVolume(0.5f);
            QMessageBox::information(this, "背景音乐",
                QString("已找到并开始播放：\n%1").arg(musicPath));
        } else {
            QMessageBox::warning(this, "背景音乐",
                QString("找到音乐文件但播放失败：\n%1\n\n%2")
                    .arg(musicPath)
                    .arg(bgmPlayer->lastError()));
        }
    } else {
        QMessageBox::information(this, "背景音乐",
            "未找到背景音乐文件。\n\n请把 .mp3 文件放到以下位置之一：\n"
            "1. 程序同级目录的 music/ 文件夹内\n"
            "2. 程序同级目录下并命名为 bgm.mp3");
    }
}

QString MainWindow::findBackgroundMusicPath() const
{
    QDir exeDir(QCoreApplication::applicationDirPath());

    // 1. 优先查找 exe 同级目录下的 music/ 子文件夹中的音频文件。
    QDir musicDir(exeDir.filePath("music"));
    if (musicDir.exists()) {
        QStringList filters;
        filters << "*.mp3" << "*.wav" << "*.ogg" << "*.flac" << "*.m4a";
        QFileInfoList files = musicDir.entryInfoList(filters, QDir::Files);
        if (!files.isEmpty()) {
            return files.first().absoluteFilePath();
        }
    }

    // 2. 查找 exe 同级目录下的 bgm.mp3。
    QString directPath = exeDir.filePath("bgm.mp3");
    if (QFile::exists(directPath)) {
        return directPath;
    }

    // 3. 开发模式：从项目根目录下的 music/ 文件夹查找。
    QDir projectDir(exeDir);
    QString currentFolderName = projectDir.dirName();
    if (currentFolderName.startsWith("Desktop_", Qt::CaseInsensitive)
        || currentFolderName.contains("Qt", Qt::CaseInsensitive)
        || currentFolderName.contains("Debug", Qt::CaseInsensitive)
        || currentFolderName.contains("Release", Qt::CaseInsensitive)) {
        projectDir.cdUp();
    }
    if (projectDir.dirName().compare("build", Qt::CaseInsensitive) == 0) {
        projectDir.cdUp();
    }

    QDir projectMusicDir(projectDir.filePath("music"));
    if (projectMusicDir.exists()) {
        QStringList filters;
        filters << "*.mp3" << "*.wav" << "*.ogg" << "*.flac" << "*.m4a";
        QFileInfoList files = projectMusicDir.entryInfoList(filters, QDir::Files);
        if (!files.isEmpty()) {
            return files.first().absoluteFilePath();
        }
    }

    return QString();
}

