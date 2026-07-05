#include "gamescene.h"
#include "constants.h"
#include "tiledefs.h"
#include "progressmanager.h"

#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPen>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QPushButton>
#include <QGraphicsRectItem>
#include <QDateTime>
#include <QDir>
#include <QInputDialog>
#include <QLineEdit>
#include <QRegularExpression>
#include <QPainter>
#include <QMovie>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QFrame>

GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
    , isTemporaryTestLevel(false)
    , temporaryTestLevel()
    , currentLevelIndex(0)
    , timer(new QTimer(this))
    , gravityDirection(GravityDirection::Down)
    , velocity(0, BALL_SPEED)
    , moveSpeed(BALL_SPEED)
    , reverseCount(0)
    , deathCount(0)
    , collectedDataFragmentCount(0)
    , totalDataFragmentCount(0)
    , collectedKeyCount(0)
    , totalKeyCount(0)
    , isPaused(false)
    , gameEnded(false)
    , wasOnTrampoline(false)
    , isTrampolineLaunchMove(false)
    , elapsedMs(0)
    , statusText(nullptr)
    , ballMovie(nullptr)
    , currentBallMoviePath()
    , isEditMode(false)
    , selectedEditTile(TileDefs::Slow)
{
    levelManager.loadDefaultLevels();
    loadLevel(0);

    connect(timer, &QTimer::timeout, this, &GameScene::updateGame);
    timer->start(TIMER_INTERVAL);

    setFocus();

    qDebug() << "LevelManager enabled.";
}
void GameScene::loadLevel(int levelIndex)
{
    if (levelManager.levelCount() == 0) {
        qWarning() << "No valid levels available.";

        QMessageBox::critical(
            nullptr,
            "关卡加载失败",
            "没有可用的合法关卡，请检查 LevelManager 中的地图数据。"
            );

        return;
    }

    if (!levelManager.isValidLevelIndex(levelIndex)) {
        qDebug() << "Invalid level index:" << levelIndex;
        return;
    }

    if (!isLevelUnlocked(levelIndex)) {
        QMessageBox::information(
            nullptr,
            QStringLiteral("关卡未解锁"),
            QStringLiteral("请先通关前面的关卡，再挑战这一关。")
            );
        return;
    }

    currentLevelIndex = levelIndex;
    isTemporaryTestLevel = false;
    temporaryTestLevel = Level();

    Level currentLevel = levelManager.levelAt(currentLevelIndex);
    applyLevelData(currentLevel);

    qDebug() << "Loaded level:" << currentLevel.name;
}

bool GameScene::isLevelUnlocked(int levelIndex) const
{
    if (!levelManager.isValidLevelIndex(levelIndex)) {
        return false;
    }

    Level level = levelManager.levelAt(levelIndex);
    if (level.isCustomLevel) {
        return true;
    }

    ProgressManager progressManager;
    return progressManager.isBuiltInLevelUnlocked(levelIndex);
}

void GameScene::loadTemporaryLevelForTest(const Level &level)
{
    isTemporaryTestLevel = true;
    temporaryTestLevel = level;
    currentLevelIndex = -1;

    applyLevelData(temporaryTestLevel);

    qDebug() << "Loaded temporary designer test level:" << temporaryTestLevel.name;
}

void GameScene::applyLevelData(const Level &currentLevel)
{
    mapData = currentLevel.mapData;

    // 读取关卡配置的候选编辑点。
    candidateEditPoints = currentLevel.editablePoints;

    // 兼容旧关卡：没有候选点时自动生成演示点。
    if (candidateEditPoints.isEmpty()) {
        candidateEditPoints = fallbackCandidateEditPoints(4);
    }

    rebuildCandidateEditPointKeys();

    editMapData = mapData;
    playerPlacedMechanismKeys.clear();
    isEditMode = false;
    gravityDirection = GravityDirection::Down;
    resetVelocityByGravity();

    reverseCount = 0;
    deathCount = 0;

    collectedDataFragmentCount = 0;
    totalDataFragmentCount = countDataFragments();
    collectedKeyCount = 0;
    totalKeyCount = countKeys();
    teleportLockPortalKey.clear();
    rebuildPortalPairs();

    elapsedMs = 0;
    wasOnTrampoline = false;
    isTrampolineLaunchMove = false;

    isPaused = false;
    gameEnded = false;

    drawMap();

    timer->start(TIMER_INTERVAL);

    updateStatusText();
    setFocus();
}

void GameScene::drawMap()
{
    clear();

    // 清理旧动画。
    if (ballMovie != nullptr) {
        ballMovie->stop();
        delete ballMovie;
        ballMovie = nullptr;
    }

    // 清空数据碎片图形记录。
    dataFragmentItems.clear();
    keyItems.clear();
    doorItems.clear();
    doorLabelItems.clear();
    laserItems.clear();
    laserLabelItems.clear();

    ball.item = nullptr;
    statusText = nullptr;

    startGridPos = QPoint(-1, -1);
    endGridPos = QPoint(-1, -1);

    const int rows = mapData.size();
    const int cols = mapData[0].size();

    setSceneRect(0, 0, cols * TILE_SIZE, rows * TILE_SIZE);
    setBackgroundBrush(QBrush(QColor("#10131f")));

    // 加载静态资源。
    static QPixmap backgroundPixmap(":/images/resources/images/map_background.png");

    // 如果有完整背景图，先铺满整个场景作为最底层。
    if (!backgroundPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = addPixmap(backgroundPixmap.scaled(
            cols * TILE_SIZE, rows * TILE_SIZE,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));
        bgItem->setPos(0, 0);
        bgItem->setZValue(-1);
    }

    drawGridBackground(rows, cols);

    static QPixmap emptyPixmap(":/images/resources/images/empty.png");
    static QPixmap wallPixmap(":/images/resources/images/wall.png");
    static QPixmap endPixmap(":/images/resources/images/end.png");
    static QPixmap deathPixmap(":/images/resources/images/death.png");
    static QPixmap bouncePixmap(":/images/resources/images/bounce.png");
    static QPixmap slowPixmap(":/images/resources/images/slow.png");
    static QPixmap conveyorPixmap(":/images/resources/images/conveyor.png");
    static QPixmap trampolineUpRightPixmap(":/images/resources/images/trampoline_upright.png");
    static QPixmap trampolineUpLeftPixmap(":/images/resources/images/trampoline_upleft.png");
    static QPixmap trampolineDownRightPixmap(":/images/resources/images/trampoline_downright.png");
    static QPixmap trampolineDownLeftPixmap(":/images/resources/images/trampoline_downleft.png");
    static QPixmap trampolineRightPixmap(":/images/resources/images/trampoline_right.png");
    static QPixmap trampolineLeftPixmap(":/images/resources/images/trampoline_left.png");
    static QPixmap laserPixmap(":/images/resources/images/laser.png");
    static QPixmap laserInactivePixmap(":/images/resources/images/laser_inactive.png");
    static QPixmap keyPixmap(":/images/resources/images/key.png");
    static QPixmap doorPixmap(":/images/resources/images/door.png");
    static QPixmap portalPixmap(":/images/resources/images/portal.png");

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const QChar tile = mapData[row][col];

            const int x = col * TILE_SIZE;
            const int y = row * TILE_SIZE;

            if (TileDefs::isEmpty(tile)) {
                // 有完整背景图时，空地块不再需要单独绘制，避免覆盖底层背景。
                if (backgroundPixmap.isNull()) {
                    if (!emptyPixmap.isNull()) {
                        QGraphicsPixmapItem *p = addPixmap(emptyPixmap.scaled(
                            TILE_SIZE, TILE_SIZE,
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation));
                        p->setPos(x, y);
                        p->setZValue(0);
                    } else {
                        addRect(x, y, TILE_SIZE, TILE_SIZE,
                                QPen(QColor("#27304a")), QBrush(QColor("#10131f")));
                    }
                }
            }
            else if (TileDefs::isWall(tile)) {
                if (!wallPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(wallPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                } else {
                    addRect(x, y, TILE_SIZE, TILE_SIZE,
                            QPen(QColor("#596275")), QBrush(QColor("#3b4252")));
                }
            }
            else if (TileDefs::isStart(tile)) {
                startGridPos = QPoint(col, row);
                if (!emptyPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(emptyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                    p->setZValue(0);
                }
            }
            else if (TileDefs::isEnd(tile)) {
                endGridPos = QPoint(col, row);
                if (!endPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(endPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                } else {
                    const int inset = 6;
                    QRectF endRect(x + inset, y + inset, TILE_SIZE - 2 * inset, TILE_SIZE - 2 * inset);
                    addRect(endRect, QPen(QColor("#2ecc71")), QBrush(QColor("#2ecc71")));
                    QFont endFont("Arial", 8, QFont::Bold);
                    QGraphicsSimpleTextItem *text = addSimpleText("END", endFont);
                    text->setBrush(Qt::white);
                    QRectF textRect = text->boundingRect();
                    text->setPos(endRect.center().x() - textRect.width() / 2,
                                 endRect.center().y() - textRect.height() / 2 + 1);
                }
            }
            else if (TileDefs::isDeath(tile)) {
                if (!deathPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(deathPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                } else {
                    addRect(x + 4, y + 4, TILE_SIZE - 8, TILE_SIZE - 8,
                            QPen(QColor("#ff4d4d")), QBrush(QColor("#b83232")));
                }
            }
            else if (TileDefs::isBounce(tile)) {
                if (!bouncePixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(bouncePixmap);
                    p->setPos(x, y);
                } else {
                    QRectF rect(x + 6, y + 6, TILE_SIZE - 12, TILE_SIZE - 12);
                    addRect(rect, QPen(QColor("#f1c40f")), QBrush(QColor("#f39c12")));
                    addCenteredTextInRect("5", rect, Qt::white);
                }
            }
            else if (TileDefs::isSlow(tile)) {
                if (!slowPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(slowPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                } else {
                    QRectF rect(x + 6, y + 6, TILE_SIZE - 12, TILE_SIZE - 12);
                    addRect(rect, QPen(QColor("#74b9ff")), QBrush(QColor("#0984e3")));
                    addCenteredTextInRect("6", rect, Qt::white);
                }
            }
            else if (TileDefs::isConveyor(tile)) {
                if (!conveyorPixmap.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(conveyorPixmap);
                    p->setPos(x, y);
                } else {
                    QRectF rect(x + 6, y + 6, TILE_SIZE - 12, TILE_SIZE - 12);
                    addRect(rect, QPen(QColor("#55efc4")), QBrush(QColor("#00b894")));
                    addCenteredTextInRect("7", rect, Qt::white);
                }
            }
            else if (TileDefs::isLaser(tile)) {
                // 激光门底下仍然画空地，熄灭时看起来就像普通可通行格子。
                if (!emptyPixmap.isNull()) {
                    QGraphicsPixmapItem *background = addPixmap(emptyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    background->setPos(x, y);
                    background->setZValue(0);
                } else {
                    addRect(x, y, TILE_SIZE, TILE_SIZE,
                            QPen(QColor("#27304a")), QBrush(QColor("#10131f")));
                }

                QString key = gridKey(QPoint(col, row));

                if (!laserPixmap.isNull()) {
                    // 使用导入的图片绘制激光门。
                    QGraphicsPixmapItem *laserItem = addPixmap(laserPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    laserItem->setPos(x, y);
                    laserItem->setZValue(9);
                    laserItems.insert(key, laserItem);
                    // 图片模式下不再额外绘制文字标签。
                } else {
                    // 回退到代码绘制（无图片资源时）。
                    QRectF beamRect(
                        x + 4,
                        y + TILE_SIZE / 2.0 - 4,
                        TILE_SIZE - 8,
                        8
                        );

                    QGraphicsRectItem *laserBeam = addRect(
                        beamRect,
                        QPen(QColor("#ff4d6d"), 2),
                        QBrush(QColor("#ff1744"))
                        );
                    laserBeam->setZValue(9);

                    QFont laserFont("Arial", 8, QFont::Bold);
                    QGraphicsSimpleTextItem *laserText = addSimpleText("L", laserFont);
                    laserText->setBrush(QColor("#ffffff"));
                    laserText->setZValue(10);
                    QRectF textRect = laserText->boundingRect();
                    laserText->setPos(
                        x + TILE_SIZE / 2.0 - textRect.width() / 2.0,
                        y + TILE_SIZE / 2.0 - textRect.height() / 2.0
                        );

                    laserItems.insert(key, laserBeam);
                    laserLabelItems.insert(key, laserText);
                }
            }
            else if (TileDefs::isKey(tile)) {
                // 钥匙底下仍然画空地，角色进入该格后会自动拾取。
                if (!emptyPixmap.isNull()) {
                    QGraphicsPixmapItem *background = addPixmap(emptyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    background->setPos(x, y);
                    background->setZValue(0);
                } else {
                    addRect(x, y, TILE_SIZE, TILE_SIZE,
                            QPen(QColor("#27304a")), QBrush(QColor("#10131f")));
                }

                QString key = gridKey(QPoint(col, row));

                if (!keyPixmap.isNull()) {
                    QGraphicsPixmapItem *keyItem = addPixmap(keyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    keyItem->setPos(x, y);
                    keyItem->setZValue(7);
                    keyItems.insert(key, keyItem);
                } else {
                    QRectF keyBody(x + 12, y + 9, TILE_SIZE - 24, TILE_SIZE - 18);
                    QGraphicsEllipseItem *keyItem = addEllipse(
                        keyBody,
                        QPen(QColor("#f8fafc"), 2),
                        QBrush(QColor("#facc15"))
                        );
                    keyItem->setZValue(7);

                    QRectF keyHandle(x + TILE_SIZE / 2.0 - 2, y + TILE_SIZE / 2.0,
                                     4, TILE_SIZE / 2.0 - 8);
                    QGraphicsRectItem *handleItem = addRect(
                        keyHandle,
                        QPen(QColor("#f8fafc"), 1),
                        QBrush(QColor("#facc15"))
                        );
                    handleItem->setParentItem(keyItem);

                    QGraphicsSimpleTextItem *keyText = addSimpleText("K", QFont("Arial", 10, QFont::Bold));
                    keyText->setBrush(QColor("#111827"));
                    keyText->setZValue(8);
                    QRectF textRect = keyText->boundingRect();
                    keyText->setPos(
                        x + TILE_SIZE / 2.0 - textRect.width() / 2.0,
                        y + TILE_SIZE / 2.0 - textRect.height() / 2.0
                        );
                    keyText->setParentItem(keyItem);

                    keyItems.insert(key, keyItem);
                }
            }
            else if (TileDefs::isDoor(tile)) {
                // 门底下画空地。未拾取钥匙时门阻挡；拾取钥匙后门变淡并可通过。
                if (!emptyPixmap.isNull()) {
                    QGraphicsPixmapItem *background = addPixmap(emptyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    background->setPos(x, y);
                    background->setZValue(0);
                } else {
                    addRect(x, y, TILE_SIZE, TILE_SIZE,
                            QPen(QColor("#27304a")), QBrush(QColor("#10131f")));
                }

                QString key = gridKey(QPoint(col, row));

                if (!doorPixmap.isNull()) {
                    QGraphicsPixmapItem *doorItem = addPixmap(doorPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    doorItem->setPos(x, y);
                    doorItem->setZValue(8);
                    doorItems.insert(key, doorItem);
                    doorLabelItems.insert(key, nullptr);
                } else {
                    QRectF doorRect(x + 6, y + 4, TILE_SIZE - 12, TILE_SIZE - 8);
                    QGraphicsRectItem *doorItem = addRect(
                        doorRect,
                        QPen(QColor("#f59e0b"), 2),
                        QBrush(QColor("#92400e"))
                        );
                    doorItem->setZValue(8);

                    QGraphicsSimpleTextItem *doorText = addSimpleText("A", QFont("Arial", 11, QFont::Bold));
                    doorText->setBrush(QColor("#ffffff"));
                    doorText->setZValue(9);
                    QRectF textRect = doorText->boundingRect();
                    doorText->setPos(
                        x + TILE_SIZE / 2.0 - textRect.width() / 2.0,
                        y + TILE_SIZE / 2.0 - textRect.height() / 2.0
                        );

                    doorItems.insert(key, doorItem);
                    doorLabelItems.insert(key, doorText);
                }
            }
            else if (TileDefs::isPortal(tile)) {
                // 传送门底下画空地，角色进入后会传送到配对传送门中心。
                if (!emptyPixmap.isNull()) {
                    QGraphicsPixmapItem *background = addPixmap(emptyPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    background->setPos(x, y);
                    background->setZValue(0);
                } else {
                    addRect(x, y, TILE_SIZE, TILE_SIZE,
                            QPen(QColor("#27304a")), QBrush(QColor("#10131f")));
                }

                if (!portalPixmap.isNull()) {
                    QGraphicsPixmapItem *portalItem = addPixmap(portalPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    portalItem->setPos(x, y);
                    portalItem->setZValue(7);
                } else {
                    QRectF portalOuter(x + 5, y + 5, TILE_SIZE - 10, TILE_SIZE - 10);
                    QGraphicsEllipseItem *portalRing = addEllipse(
                        portalOuter,
                        QPen(QColor("#a78bfa"), 3),
                        QBrush(QColor(76, 29, 149, 170))
                        );
                    portalRing->setZValue(7);

                    QRectF portalInner(x + 12, y + 12, TILE_SIZE - 24, TILE_SIZE - 24);
                    QGraphicsEllipseItem *portalCore = addEllipse(
                        portalInner,
                        QPen(QColor("#ddd6fe"), 1),
                        QBrush(QColor("#7c3aed"))
                        );
                    portalCore->setZValue(8);

                    QGraphicsSimpleTextItem *portalText = addSimpleText("B", QFont("Arial", 10, QFont::Bold));
                    portalText->setBrush(QColor("#ffffff"));
                    portalText->setZValue(9);
                    QRectF textRect = portalText->boundingRect();
                    portalText->setPos(
                        x + TILE_SIZE / 2.0 - textRect.width() / 2.0,
                        y + TILE_SIZE / 2.0 - textRect.height() / 2.0
                        );
                }
            }
            else if (TileDefs::isTrampoline(tile)) {
                QPixmap pixmapToUse;

                if (TileDefs::isTrampolineUpRight(tile) && !trampolineUpRightPixmap.isNull()) {
                    pixmapToUse = trampolineUpRightPixmap;
                } else if (TileDefs::isTrampolineUpLeft(tile) && !trampolineUpLeftPixmap.isNull()) {
                    pixmapToUse = trampolineUpLeftPixmap;
                } else if (TileDefs::isTrampolineDownRight(tile) && !trampolineDownRightPixmap.isNull()) {
                    pixmapToUse = trampolineDownRightPixmap;
                } else if (TileDefs::isTrampolineDownLeft(tile) && !trampolineDownLeftPixmap.isNull()) {
                    pixmapToUse = trampolineDownLeftPixmap;
                } else if (TileDefs::isTrampolineRight(tile) && !trampolineRightPixmap.isNull()) {
                    pixmapToUse = trampolineRightPixmap;
                } else if (TileDefs::isTrampolineLeft(tile) && !trampolineLeftPixmap.isNull()) {
                    pixmapToUse = trampolineLeftPixmap;
                }

                if (!pixmapToUse.isNull()) {
                    QGraphicsPixmapItem *p = addPixmap(pixmapToUse.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    p->setPos(x, y);
                } else {
                    QRectF rect(
                        x + 5,
                        y + 8,
                        TILE_SIZE - 10,
                        TILE_SIZE - 16
                        );

                    addRect(
                        rect,
                        QPen(QColor("#ff79c6")),
                        QBrush(QColor("#d63384"))
                        );

                    addCenteredTextInRect(TileDefs::trampolineArrow(tile), rect, Qt::white);
                }
            }
            else if (TileDefs::isData(tile)) {
                static QPixmap dataPixmap(":/images/resources/images/data_fragment.png");

                if (!dataPixmap.isNull()) {
                    QGraphicsPixmapItem *fragmentItem = addPixmap(dataPixmap.scaled(
                        TILE_SIZE, TILE_SIZE,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation));
                    fragmentItem->setPos(x, y);
                    fragmentItem->setZValue(5);
                    QString key = gridKey(QPoint(col, row));
                    dataFragmentItems.insert(key, fragmentItem);
                } else {
                    QRectF rect(x + 8, y + 8, TILE_SIZE - 16, TILE_SIZE - 16);
                    QGraphicsEllipseItem *fragmentItem = addEllipse(
                        rect, QPen(QColor("#ffffff")), QBrush(QColor("#00f5d4")));
                    fragmentItem->setZValue(5);
                    QGraphicsSimpleTextItem *dataText = addSimpleText("8");
                    dataText->setBrush(Qt::black);
                    dataText->setZValue(6);
                    dataText->setParentItem(fragmentItem);
                    QRectF textRect = dataText->boundingRect();
                    dataText->setPos(rect.center().x() - textRect.width() / 2,
                                     rect.center().y() - textRect.height() / 2);
                    QString key = gridKey(QPoint(col, row));
                    dataFragmentItems.insert(key, fragmentItem);
                }
            }
        }
    }

    createBallAtStart();

    // 编辑模式下绘制候选点提示。
    drawCandidateEditPoints();

    createStatusText();
    updateLaserItems();
    updateDoorItems();

    qDebug() << "Map loaded.";
    qDebug() << "Start grid position:" << startGridPos;
    qDebug() << "Ball center position:" << ball.position;
}

void GameScene::drawGridBackground(int rows, int cols)
{
    QPen gridPen(QColor("#27304a"));
    gridPen.setWidth(1);

    for (int row = 0; row <= rows; ++row) {
        int y = row * TILE_SIZE;
        addLine(0, y, cols * TILE_SIZE, y, gridPen);
    }

    for (int col = 0; col <= cols; ++col) {
        int x = col * TILE_SIZE;
        addLine(x, 0, x, rows * TILE_SIZE, gridPen);
    }
}

void GameScene::addCenteredTextInRect(const QString &text, const QRectF &rect, const QColor &color)
{
    QFont font("Arial", 12, QFont::Bold);

    QGraphicsSimpleTextItem *textItem = addSimpleText(text, font);
    textItem->setBrush(color);
    textItem->setZValue(8);

    QRectF textRect = textItem->boundingRect();

    textItem->setPos(
        rect.center().x() - textRect.width() / 2,
        rect.center().y() - textRect.height() / 2
        );
}

QPointF GameScene::gridCenterToScenePos(const QPoint &gridPos) const
{
    return QPointF(
        gridPos.x() * TILE_SIZE + TILE_SIZE / 2.0,
        gridPos.y() * TILE_SIZE + TILE_SIZE / 2.0
        );
}

void GameScene::createBallAtStart()
{
    ball.position = gridCenterToScenePos(startGridPos);
    lastNonLaserBallPosition = ball.position;
    const int targetSize = ball.radius * 3;
    // 设置与角色贴图匹配的方形碰撞体。
    ball.collisionHalfSize = ball.radius * 3 / 2;

    // 清理旧动画。
    if (ballMovie != nullptr) {
        ballMovie->stop();
        delete ballMovie;
        ballMovie = nullptr;
    }
    currentBallMoviePath.clear();

    // 创建静态占位图。
    QPixmap pixmapToUse;
    static QPixmap ballPixmap(":/images/resources/images/ball.png");
    pixmapToUse = ballPixmap;

    if (pixmapToUse.isNull()) {
        // 资源缺失时绘制占位图。
        int d = ball.radius * 2;
        pixmapToUse = QPixmap(d, d);
        pixmapToUse.fill(Qt::transparent);
        QPainter painter(&pixmapToUse);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor("#80f7ff"), 2));
        painter.setBrush(QBrush(QColor("#8a5cff")));
        painter.drawEllipse(0, 0, d, d);
        painter.end();
    } else if (pixmapToUse.width() != targetSize || pixmapToUse.height() != targetSize) {
        pixmapToUse = pixmapToUse.scaled(
            targetSize, targetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    }

    ball.item = addPixmap(pixmapToUse);
    ball.item->setOffset(-pixmapToUse.width() / 2.0, -pixmapToUse.height() / 2.0);
    ball.item->setPos(ball.position);
    ball.item->setZValue(10);

    // 按重力和速度切换角色动画。
    updateBallMovie();
}

void GameScene::createStatusText()
{
    QFont font("Microsoft YaHei", 9, QFont::Bold);

    statusText = addSimpleText("", font);
    statusText->setBrush(QColor("#f1fa8c"));
    statusText->setPos(8, 8);
    statusText->setZValue(20);

    updateStatusText();
}

void GameScene::updateStatusText()
{
    QString stateText;

    if (isEditMode) {
        stateText = QString("候选点编辑：%1").arg(selectedEditTileName());
    }
    else if (gameEnded) {
        stateText = "已结束";
    }
    else if (isPaused) {
        stateText = "暂停中";
    }
    else {
        if (ball.item != nullptr && hasAnyWallContact()) {
            stateText = "贴墙可切换";
        } else {
            stateText = "空中不可切换";
        }
    }

    QString levelName = "未知关卡";
    int targetCount = 0;
    QString levelText;

    if (isTemporaryTestLevel) {
        levelName = "[测试] " + temporaryTestLevel.name;
        targetCount = temporaryTestLevel.targetReverseCount;
        levelText = QString("关卡：测试  %1").arg(levelName);
    }
    else if (levelManager.isValidLevelIndex(currentLevelIndex)) {
        Level currentLevel = levelManager.levelAt(currentLevelIndex);
        levelName = currentLevel.name;
        if (currentLevel.isCustomLevel) {
            levelName = "[自定义] " + levelName;
        }
        targetCount = currentLevel.targetReverseCount;
        levelText = QString("关卡：%1/%2  %3")
                        .arg(currentLevelIndex + 1)
                        .arg(levelManager.levelCount())
                        .arg(levelName);
    }
    else {
        levelText = QString("关卡：未知  %1").arg(levelName);
    }

    QString gravityText = QString("重力：%1")
                              .arg(gravityDirectionToString());

    QString timeText = QString("时间：%1s")
                           .arg(elapsedTimeText());

    QString reverseText = QString("反转：%1 / 目标：%2   碎片：%3/%4")
                              .arg(reverseCount)
                              .arg(targetCount)
                              .arg(collectedDataFragmentCount)
                              .arg(totalDataFragmentCount);

    if (totalKeyCount > 0) {
        reverseText += QString("   钥匙：%1/%2")
                           .arg(collectedKeyCount)
                           .arg(totalKeyCount);
    }

    QString deathText = QString("死亡：%1")
                            .arg(deathCount);

    QString fullStateText = QString("状态：%1")
                                .arg(stateText);

    if (statusText != nullptr) {
        statusText->setScale(1.0);
        statusText->setText(
            QString("%1   %2   %3   %4   %5   %6")
                .arg(levelText)
                .arg(gravityText)
                .arg(timeText)
                .arg(reverseText)
                .arg(deathText)
                .arg(fullStateText)
            );

        // 窄地图中状态文字也按地图宽度自动缩小，避免伸出终端边框。
        const double maximumWidth = qMax(1.0, sceneRect().width() - 16.0);
        const double textWidth = statusText->boundingRect().width();
        if (textWidth > maximumWidth) {
            statusText->setScale(maximumWidth / textWidth);
        }
    }

    emit statusChanged(
        levelText,
        gravityText,
        timeText,
        reverseText,
        deathText,
        fullStateText
        );
}

QString GameScene::gravityDirectionToString() const
{
    if (gravityDirection == GravityDirection::Up) {
        return "↑";
    }
    else if (gravityDirection == GravityDirection::Down) {
        return "↓";
    }
    else if (gravityDirection == GravityDirection::Left) {
        return "←";
    }
    else if (gravityDirection == GravityDirection::Right) {
        return "→";
    }

    return "?";
}

void GameScene::updateGame()
{

    if (ball.item == nullptr || isPaused || gameEnded || isEditMode) {
        return;
    }

    elapsedMs += TIMER_INTERVAL;
    updateLaserItems();

    // 激光重新亮起时，将激光格内的角色弹回最近安全位置。
    if (isActiveLaserAt(ball.position)) {
        QPointF blockedMovement = velocity;
        if (blockedMovement == QPointF(0, 0)) {
            blockedMovement = velocityForGravityDirection(gravityDirection);
        }

        if (lastNonLaserBallPosition != QPointF()
            && canBallMoveTo(lastNonLaserBallPosition)
            && !isActiveLaserAt(lastNonLaserBallPosition)) {
            ball.setPosition(lastNonLaserBallPosition);
        }

        repelFromLaserCollision(blockedMovement);
        updateBallMovie();
        updateStatusText();
        return;
    }

    moveBallOneStep();

    // 离开上下支撑面后，立即按当前重力坠落。
    applyGravityAfterLeavingWall();

    applyTileEffects();

    checkCurrentTile();

    if (!gameEnded) {
        rememberLastNonLaserPosition();
        updateBallMovie();
        updateStatusText();
    }
}
void GameScene::moveBallOneStep()
{
    QPointF newPosition = ball.position;

    const bool isDiagonalAirMove = (velocity.x() != 0 && velocity.y() != 0);
    const bool shouldStopOnWall = isDiagonalAirMove || isTrampolineLaunchMove;

    // 先处理水平位移；贴地或贴天花板时允许继续接触当前支撑面。
    if (velocity.x() != 0) {
        QPointF horizontalVelocity(velocity.x(), 0);
        QPointF tryXPosition(
            ball.position.x() + velocity.x(),
            ball.position.y()
            );

        if (canBallMoveToForVelocity(tryXPosition, horizontalVelocity)) {
            newPosition.setX(tryXPosition.x());
        } else {
            if (wouldCollideWithActiveLaser(tryXPosition)) {
                ball.setPosition(newPosition);
                repelFromLaserCollision(horizontalVelocity);
                return;
            }

            if (shouldStopOnWall) {
                // 空中弹射撞到侧墙时只停止水平分量，保留竖直坠落。
                if (velocity.y() != 0
                    && (gravityDirection == GravityDirection::Up
                        || gravityDirection == GravityDirection::Down)) {
                    velocity = velocityForGravityDirection(gravityDirection);
                    isTrampolineLaunchMove = true;

                    qDebug() << "Trampoline launch hit side wall; continue falling by gravity."
                             << "Gravity:" << gravityDirectionToString()
                             << "Velocity:" << velocity;
                } else {
                    velocity = QPointF(0, 0);
                    isTrampolineLaunchMove = false;
                    ball.setPosition(newPosition);
                    return;
                }
            }

            bool escapedFromLedge = false;

            if (gravityDirection == GravityDirection::Down
                || gravityDirection == GravityDirection::Up) {
                QPointF fallVelocity = velocityForGravityDirection(gravityDirection);

                if (tryEscapeCornerAndFall(ball.position, fallVelocity, &newPosition)) {
                    velocity = fallVelocity;
                    escapedFromLedge = true;
                }
            }

            if (!escapedFromLedge) {
                velocity.setX(0);
            }
        }
    }

    // 再处理竖直位移，上下墙始终严格阻挡。
    if (velocity.y() != 0) {
        QPointF verticalVelocity(0, velocity.y());
        QPointF tryYPosition(
            newPosition.x(),
            newPosition.y() + velocity.y()
            );

        if (canBallMoveToForVelocity(tryYPosition, verticalVelocity)) {
            newPosition.setY(tryYPosition.y());
        } else {
            if (wouldCollideWithActiveLaser(tryYPosition)) {
                ball.setPosition(newPosition);
                repelFromLaserCollision(verticalVelocity);
                return;
            }

            if (shouldStopOnWall) {
                // 撞到坠落方向的墙面后停止。
                velocity = QPointF(0, 0);
                isTrampolineLaunchMove = false;
                ball.setPosition(newPosition);
                return;
            }

            bool escapedFromCorner = false;

            if (gravityDirection == GravityDirection::Up) {
                if (!hasDirectSupportAbove()) {
                    QPointF fallVelocity = velocityForGravityDirection(GravityDirection::Up);

                    if (tryEscapeCornerAndFall(newPosition, fallVelocity, &newPosition)) {
                        velocity = fallVelocity;
                        escapedFromCorner = true;
                    }
                }
            }
            else if (gravityDirection == GravityDirection::Down) {
                if (!hasDirectSupportBelow()) {
                    QPointF fallVelocity = velocityForGravityDirection(GravityDirection::Down);

                    if (tryEscapeCornerAndFall(newPosition, fallVelocity, &newPosition)) {
                        velocity = fallVelocity;
                        escapedFromCorner = true;
                    }
                }
            }

            if (!escapedFromCorner) {
                velocity.setY(0);
            }
        }
    }

    ball.setPosition(newPosition);
}

QString GameScene::indicatorKeyForEvent(int key) const
{
    switch (key) {
    case Qt::Key_W:
    case Qt::Key_Up:
        return "W";

    case Qt::Key_A:
    case Qt::Key_Left:
        return "A";

    case Qt::Key_S:
    case Qt::Key_Down:
        return "S";

    case Qt::Key_D:
    case Qt::Key_Right:
        return "D";

    default:
        return QString();
    }
}

void GameScene::keyPressEvent(QKeyEvent *event)
{
    if (isEditMode) {
        event->accept();
        return;
    }

    const QString indicatorKey = indicatorKeyForEvent(event->key());

    switch (event->key()) {
    case Qt::Key_W:
    case Qt::Key_Up:
        setGravityDirection(GravityDirection::Up);
        break;

    case Qt::Key_S:
    case Qt::Key_Down:
        setGravityDirection(GravityDirection::Down);
        break;

    case Qt::Key_A:
    case Qt::Key_Left:
        setGravityDirection(GravityDirection::Left);
        break;

    case Qt::Key_D:
    case Qt::Key_Right:
        setGravityDirection(GravityDirection::Right);
        break;

    case Qt::Key_R:
        restartLevel();
        break;

    case Qt::Key_Space:
        togglePause();
        break;

    case Qt::Key_N:
        nextLevel();
        break;

    case Qt::Key_P:
        previousLevel();
        break;

    default:
        QGraphicsScene::keyPressEvent(event);
        return;
    }

    if (!indicatorKey.isEmpty()) {
        emit inputDirectionChanged(indicatorKey);
    }

    event->accept();
}

void GameScene::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat() && !indicatorKeyForEvent(event->key()).isEmpty()) {
        emit inputDirectionChanged("");
        event->accept();
        return;
    }

    QGraphicsScene::keyReleaseEvent(event);
}

void GameScene::setGravityDirection(GravityDirection newDirection)
{
    if (ball.item == nullptr || gameEnded || isPaused) {
        return;
    }

    const bool touchingAbove = isTouchingWallAbove();
    const bool touchingBelow = isTouchingWallBelow();

    // 只有上下墙算支撑面，左右墙只负责阻挡。
    if (!touchingAbove && !touchingBelow) {
        qDebug() << "Gravity change denied: ball has no upper/lower wall support.";
        updateStatusText();
        return;
    }

    GravityDirection nextGravityDirection = gravityDirection;
    QPointF nextVelocity(0, 0);
    bool allowed = false;

    // 贴着下方墙时，可左右滚动或向上离开。
    if (touchingBelow) {
        if (newDirection == GravityDirection::Left) {
            nextGravityDirection = GravityDirection::Down;
            nextVelocity = QPointF(-moveSpeed, 0);
            allowed = true;
        }
        else if (newDirection == GravityDirection::Right) {
            nextGravityDirection = GravityDirection::Down;
            nextVelocity = QPointF(moveSpeed, 0);
            allowed = true;
        }
        else if (newDirection == GravityDirection::Up) {
            nextGravityDirection = GravityDirection::Up;
            nextVelocity = QPointF(0, -moveSpeed);
            allowed = true;
        }
    }

    // 贴着上方墙时，可左右滚动或向下离开。
    if (!allowed && touchingAbove) {
        if (newDirection == GravityDirection::Left) {
            nextGravityDirection = GravityDirection::Up;
            nextVelocity = QPointF(-moveSpeed, 0);
            allowed = true;
        }
        else if (newDirection == GravityDirection::Right) {
            nextGravityDirection = GravityDirection::Up;
            nextVelocity = QPointF(moveSpeed, 0);
            allowed = true;
        }
        else if (newDirection == GravityDirection::Down) {
            nextGravityDirection = GravityDirection::Down;
            nextVelocity = QPointF(0, moveSpeed);
            allowed = true;
        }
    }

    if (!allowed) {
        qDebug() << "Gravity change denied: direction goes into the support wall.";
        updateStatusText();
        return;
    }

    // 新方向若立即撞墙则忽略；横向贴墙移动允许接触当前支撑面。
    if (!canBallMoveToForVelocity(ball.position + nextVelocity, nextVelocity)) {
        qDebug() << "Gravity change denied: next movement is blocked by wall.";
        updateStatusText();
        return;
    }

    bool changed = (gravityDirection != nextGravityDirection || velocity != nextVelocity);

    gravityDirection = nextGravityDirection;
    velocity = nextVelocity;
    isTrampolineLaunchMove = false;

    if (changed) {
        reverseCount++;
    }

    updateStatusText();
    updateBallMovie();

    qDebug() << "Control accepted. Gravity:" << gravityDirectionToString()
             << "Velocity:" << velocity
             << "Reverse count:" << reverseCount;
}

void GameScene::restartLevel()
{
    int oldDeathCount = deathCount;

    if (!editMapData.isEmpty()) {
        mapData = editMapData;

        isEditMode = false;
        isPaused = false;
        gameEnded = false;

        resetRuntimeStateForCurrentMap();

        deathCount = oldDeathCount;

        drawMap();

        timer->start(TIMER_INTERVAL);

        updateStatusText();
        setFocus();

        qDebug() << "Current edited level restarted.";
        return;
    }

    loadLevel(currentLevelIndex);

    deathCount = oldDeathCount;

    updateStatusText();

    qDebug() << "Current level restarted.";
}
void GameScene::nextLevel()
{
    if (isTemporaryTestLevel) {
        QMessageBox::information(
            nullptr,
            "提示",
            "当前是关卡设计师的临时测试关卡，没有下一关。"
            );
        setFocus();
        return;
    }

    if (!levelManager.isValidLevelIndex(currentLevelIndex + 1)) {
        QMessageBox::information(
            nullptr,
            "提示",
            "已经是最后一关了。"
            );

        setFocus();
        return;
    }

    loadLevel(currentLevelIndex + 1);
}

void GameScene::previousLevel()
{
    if (isTemporaryTestLevel) {
        QMessageBox::information(
            nullptr,
            "提示",
            "当前是关卡设计师的临时测试关卡，没有上一关。"
            );
        setFocus();
        return;
    }

    if (!levelManager.isValidLevelIndex(currentLevelIndex - 1)) {
        QMessageBox::information(
            nullptr,
            "提示",
            "已经是第一关了。"
            );

        setFocus();
        return;
    }

    loadLevel(currentLevelIndex - 1);
}

void GameScene::loadLevelByNumber(int levelNumber)
{
    int levelIndex = levelNumber - 1;

    if (!levelManager.isValidLevelIndex(levelIndex)) {
        qDebug() << "Invalid level number:" << levelNumber;
        return;
    }

    loadLevel(levelIndex);
}

int GameScene::currentLevelNumber() const
{
    if (isTemporaryTestLevel) {
        return 1;
    }

    return currentLevelIndex + 1;
}

int GameScene::totalLevelCount() const
{
    if (isTemporaryTestLevel) {
        return 1;
    }

    return levelManager.levelCount();
}

QString GameScene::levelNameByNumber(int levelNumber) const
{
    if (isTemporaryTestLevel && levelNumber == 1) {
        return temporaryTestLevel.name;
    }

    int levelIndex = levelNumber - 1;

    if (!levelManager.isValidLevelIndex(levelIndex)) {
        return QString("第 %1 关").arg(levelNumber);
    }

    Level level = levelManager.levelAt(levelIndex);

    return level.name;
}

void GameScene::togglePause()
{
    if (gameEnded) {
        return;
    }

    isPaused = !isPaused;

    if (isPaused) {
        timer->stop();
        qDebug() << "Game paused.";
    } else {
        timer->start(TIMER_INTERVAL);
        qDebug() << "Game resumed.";
    }

    updateStatusText();
    setFocus();
}

QChar GameScene::tileAtScenePos(const QPointF &scenePos) const
{
    if (scenePos.x() < 0 || scenePos.y() < 0) {
        return TileDefs::Wall;
    }

    int col = static_cast<int>(scenePos.x()) / TILE_SIZE;
    int row = static_cast<int>(scenePos.y()) / TILE_SIZE;

    if (row < 0 || row >= mapData.size()) {
        return TileDefs::Wall;
    }

    if (col < 0 || col >= mapData[row].size()) {
        return TileDefs::Wall;
    }

    return mapData[row][col];
}

bool GameScene::isWallAt(const QPointF &scenePos) const
{
    QChar tile = tileAtScenePos(scenePos);
    return TileDefs::isWall(tile) || (TileDefs::isDoor(tile) && !isDoorOpen());
}

bool GameScene::isDoorOpen() const
{
    // 当前版本采用“一把钥匙打开全部门”的规则。
    return collectedKeyCount > 0;
}

bool GameScene::isClosedDoorAt(const QPointF &scenePos) const
{
    return TileDefs::isDoor(tileAtScenePos(scenePos)) && !isDoorOpen();
}

bool GameScene::isPortalAt(const QPointF &scenePos) const
{
    return TileDefs::isPortal(tileAtScenePos(scenePos));
}

bool GameScene::isLaserActive() const
{
    const int cycleMs = LASER_ACTIVE_MS + LASER_INACTIVE_MS;

    if (cycleMs <= 0) {
        return false;
    }

    int phaseMs = elapsedMs % cycleMs;
    return phaseMs < LASER_ACTIVE_MS;
}

bool GameScene::isActiveLaserAt(const QPointF &scenePos) const
{
    return isLaserActive() && TileDefs::isLaser(tileAtScenePos(scenePos));
}

bool GameScene::isBlockingAt(const QPointF &scenePos) const
{
    return isWallAt(scenePos) || isActiveLaserAt(scenePos);
}

bool GameScene::canBallMoveTo(const QPointF &nextPosition) const
{
    return canBallMoveToWithSupportAllowance(nextPosition, false, false, false, false);
}

bool GameScene::canBallMoveToForVelocity(const QPointF &nextPosition,
                                         const QPointF &movement) const
{
    const bool horizontalMove = (movement.x() != 0 && movement.y() == 0);
    const bool verticalMove = (movement.y() != 0 && movement.x() == 0);

    // 横向贴支撑面移动时，忽略当前支撑面避免误判。
    const bool ignoreAbove =
        horizontalMove
        && gravityDirection == GravityDirection::Up
        && isTouchingWallAbove();

    const bool ignoreBelow =
        horizontalMove
        && gravityDirection == GravityDirection::Down
        && isTouchingWallBelow();

    // 纯竖直坠落贴到侧墙时，忽略对应侧边以避免卡墙。
    const bool ignoreLeft =
        verticalMove
        && (gravityDirection == GravityDirection::Up || gravityDirection == GravityDirection::Down)
        && isTouchingWallLeft();

    const bool ignoreRight =
        verticalMove
        && (gravityDirection == GravityDirection::Up || gravityDirection == GravityDirection::Down)
        && isTouchingWallRight();

    return canBallMoveToWithSupportAllowance(nextPosition,
                                             ignoreLeft,
                                             ignoreRight,
                                             ignoreAbove,
                                             ignoreBelow);
}

bool GameScene::canBallMoveToWithSupportAllowance(const QPointF &nextPosition,
                                                  bool ignoreLeft,
                                                  bool ignoreRight,
                                                  bool ignoreAbove,
                                                  bool ignoreBelow) const
{
    const double r = collisionRadius();
    const double inset = 1.0;

    const double left = nextPosition.x() - r + inset;
    const double right = nextPosition.x() + r - inset;
    const double top = nextPosition.y() - r + inset;
    const double bottom = nextPosition.y() + r - inset;
    const double centerX = nextPosition.x();
    const double centerY = nextPosition.y();

    auto blocked = [this](double x, double y) {
        return isBlockingAt(QPointF(x, y));
    };

    // 仅在竖直坠落脱困时可忽略侧墙。
    if (!ignoreLeft && blocked(left, centerY)) {
        return false;
    }

    if (!ignoreRight && blocked(right, centerY)) {
        return false;
    }

    // 非支撑上边需要完整检测。
    if (!ignoreAbove) {
        if (blocked(left, top)
            || blocked(centerX, top)
            || blocked(right, top)) {
            return false;
        }
    }

    // 非支撑下边需要完整检测。
    if (!ignoreBelow) {
        if (blocked(left, bottom)
            || blocked(centerX, bottom)
            || blocked(right, bottom)) {
            return false;
        }
    }

    return true;
}

int GameScene::collisionRadius() const
{
    // 方形碰撞体留出 2px 安全边，减少贴墙和墙角卡死。
    int r = ball.collisionHalfSize - 2;

    if (r < 1) {
        r = 1;
    }

    return r;
}

void GameScene::updateLaserItems()
{
    const bool active = isLaserActive();

    static QPixmap laserPixmap(":/images/resources/images/laser.png");

    QColor beamColor = active ? QColor("#ff1744") : QColor("#33415c");
    QColor penColor = active ? QColor("#ff8fa3") : QColor("#62708a");
    QColor textColor = active ? QColor("#ffffff") : QColor("#8a96ad");
    qreal rectOpacity = active ? 1.0 : 0.28;

    for (auto it = laserItems.begin(); it != laserItems.end(); ++it) {
        QGraphicsRectItem *rectItem = qgraphicsitem_cast<QGraphicsRectItem *>(it.value());
        QGraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem *>(it.value());

        if (rectItem != nullptr) {
            QPen pen(penColor, active ? 2 : 1);
            pen.setStyle(active ? Qt::SolidLine : Qt::DashLine);

            rectItem->setPen(pen);
            rectItem->setBrush(QBrush(beamColor));
            rectItem->setOpacity(rectOpacity);
        } else if (pixmapItem != nullptr) {
            // 图片模式：始终使用 laser.png，通过透明度区分亮灭状态。
            if (!laserPixmap.isNull()) {
                pixmapItem->setPixmap(laserPixmap.scaled(
                    TILE_SIZE, TILE_SIZE,
                    Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation));
            }
            pixmapItem->setOpacity(active ? 1.0 : 0.28);
        }
    }

    for (auto it = laserLabelItems.begin(); it != laserLabelItems.end(); ++it) {
        QGraphicsSimpleTextItem *textItem = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(it.value());

        if (textItem == nullptr) {
            continue;
        }

        textItem->setBrush(textColor);
        textItem->setOpacity(active ? 1.0 : 0.45);
    }
}

void GameScene::updateDoorItems()
{
    const bool open = isDoorOpen();

    for (auto it = doorItems.begin(); it != doorItems.end(); ++it) {
        if (it.value() != nullptr) {
            it.value()->setOpacity(open ? 0.22 : 1.0);
            it.value()->setVisible(true);
        }
    }

    for (auto it = doorLabelItems.begin(); it != doorLabelItems.end(); ++it) {
        if (it.value() != nullptr) {
            it.value()->setOpacity(open ? 0.22 : 1.0);
            it.value()->setVisible(true);
        }
    }
}

bool GameScene::wouldCollideWithActiveLaser(const QPointF &nextPosition) const
{
    if (!isLaserActive()) {
        return false;
    }

    const double r = collisionRadius();
    const double inset = 1.0;

    const double left = nextPosition.x() - r + inset;
    const double right = nextPosition.x() + r - inset;
    const double top = nextPosition.y() - r + inset;
    const double bottom = nextPosition.y() + r - inset;
    const double centerX = nextPosition.x();
    const double centerY = nextPosition.y();

    return isActiveLaserAt(QPointF(left, centerY))
           || isActiveLaserAt(QPointF(right, centerY))
           || isActiveLaserAt(QPointF(centerX, top))
           || isActiveLaserAt(QPointF(centerX, bottom))
           || isActiveLaserAt(QPointF(left, top))
           || isActiveLaserAt(QPointF(right, top))
           || isActiveLaserAt(QPointF(left, bottom))
           || isActiveLaserAt(QPointF(right, bottom));
}

void GameScene::repelFromLaserCollision(const QPointF &blockedMovement)
{
    QPointF nextVelocity = velocity;

    if (blockedMovement.x() != 0) {
        nextVelocity.setX(-blockedMovement.x());
    }

    if (blockedMovement.y() != 0) {
        nextVelocity.setY(-blockedMovement.y());
    }

    if (nextVelocity == QPointF(0, 0)) {
        nextVelocity = -velocityForGravityDirection(gravityDirection);
    }

    velocity = nextVelocity;
    moveSpeed = BALL_SPEED;
    isTrampolineLaunchMove = false;

    // 激光只在竖直反弹时改变重力，水平碰撞只反向速度。
    if (blockedMovement.y() != 0) {
        if (velocity.y() < 0) {
            gravityDirection = GravityDirection::Up;
        }
        else if (velocity.y() > 0) {
            gravityDirection = GravityDirection::Down;
        }
    }

    qDebug() << "Laser repelled player. Active:" << isLaserActive()
             << "Velocity:" << velocity
             << "Gravity:" << gravityDirectionToString();
}

void GameScene::rememberLastNonLaserPosition()
{
    if (ball.item == nullptr) {
        return;
    }

    if (!TileDefs::isLaser(tileAtScenePos(ball.position))) {
        lastNonLaserBallPosition = ball.position;
    }
}

bool GameScene::isTouchingWallAbove() const
{
    if (ball.item == nullptr) {
        return false;
    }

    // 检测方形碰撞体上边缘。
    const double supportRadius = collisionRadius();
    const double probeY = ball.position.y() - supportRadius - BALL_SPEED - 2.0;
    const double leftX = ball.position.x() - supportRadius + 1.0;
    const double rightX = ball.position.x() + supportRadius - 1.0;

    for (double x = leftX; x <= rightX; x += 4.0) {
        if (isWallAt(QPointF(x, probeY))) {
            return true;
        }
    }

    return isWallAt(QPointF(rightX, probeY));
}

bool GameScene::isTouchingWallBelow() const
{
    if (ball.item == nullptr) {
        return false;
    }

    // 检测方形碰撞体下边缘。
    const double supportRadius = collisionRadius();
    const double probeY = ball.position.y() + supportRadius + BALL_SPEED + 2.0;
    const double leftX = ball.position.x() - supportRadius + 1.0;
    const double rightX = ball.position.x() + supportRadius - 1.0;

    for (double x = leftX; x <= rightX; x += 4.0) {
        if (isWallAt(QPointF(x, probeY))) {
            return true;
        }
    }

    return isWallAt(QPointF(rightX, probeY));
}

bool GameScene::hasDirectSupportAbove() const
{
    if (ball.item == nullptr) {
        return false;
    }

    const double probeY = ball.position.y() - collisionRadius() - BALL_SPEED - 2.0;

    // 只检测球心正上方，用于识别内凹角误判。
    return isWallAt(QPointF(ball.position.x(), probeY));
}

bool GameScene::hasDirectSupportBelow() const
{
    if (ball.item == nullptr) {
        return false;
    }

    const double probeY = ball.position.y() + collisionRadius() + BALL_SPEED + 2.0;

    // 只检测球心正下方。
    return isWallAt(QPointF(ball.position.x(), probeY));
}

bool GameScene::isTouchingWallLeft() const
{
    if (ball.item == nullptr) {
        return false;
    }

    // 检测方形碰撞体左边缘。
    const double r = collisionRadius();
    const double probeX = ball.position.x() - r - BALL_SPEED - 2.0;
    const double topY = ball.position.y() - r + 1.0;
    const double bottomY = ball.position.y() + r - 1.0;

    for (double y = topY; y <= bottomY; y += 4.0) {
        if (isWallAt(QPointF(probeX, y))) {
            return true;
        }
    }

    return isWallAt(QPointF(probeX, bottomY));
}

bool GameScene::isTouchingWallRight() const
{
    if (ball.item == nullptr) {
        return false;
    }

    // 检测方形碰撞体右边缘。
    const double r = collisionRadius();
    const double probeX = ball.position.x() + r + BALL_SPEED + 2.0;
    const double topY = ball.position.y() - r + 1.0;
    const double bottomY = ball.position.y() + r - 1.0;

    for (double y = topY; y <= bottomY; y += 4.0) {
        if (isWallAt(QPointF(probeX, y))) {
            return true;
        }
    }

    return isWallAt(QPointF(probeX, bottomY));
}

bool GameScene::hasAnyWallContact() const
{
    // 只有上下墙算可操作支撑面。
    return isTouchingWallAbove() || isTouchingWallBelow();
}

QPointF GameScene::velocityForGravityDirection(GravityDirection direction) const
{
    int speed = moveSpeed;

    if (speed <= 0) {
        speed = BALL_SPEED;
    }

    if (direction == GravityDirection::Up) {
        return QPointF(0, -speed);
    }

    if (direction == GravityDirection::Down) {
        return QPointF(0, speed);
    }

    if (direction == GravityDirection::Left) {
        return QPointF(-speed, 0);
    }

    if (direction == GravityDirection::Right) {
        return QPointF(speed, 0);
    }

    return QPointF(0, 0);
}

bool GameScene::isGravityChangeAllowed(GravityDirection newDirection) const
{
    if (ball.item == nullptr) {
        return false;
    }

    const bool touchingAbove = isTouchingWallAbove();
    const bool touchingBelow = isTouchingWallBelow();

    if (!touchingAbove && !touchingBelow) {
        return false;
    }

    QPointF testVelocity(0, 0);

    if (touchingBelow) {
        if (newDirection == GravityDirection::Left) {
            testVelocity = QPointF(-moveSpeed, 0);
        }
        else if (newDirection == GravityDirection::Right) {
            testVelocity = QPointF(moveSpeed, 0);
        }
        else if (newDirection == GravityDirection::Up) {
            testVelocity = QPointF(0, -moveSpeed);
        }
        else {
            return false;
        }

        return canBallMoveToForVelocity(ball.position + testVelocity, testVelocity);
    }

    if (touchingAbove) {
        if (newDirection == GravityDirection::Left) {
            testVelocity = QPointF(-moveSpeed, 0);
        }
        else if (newDirection == GravityDirection::Right) {
            testVelocity = QPointF(moveSpeed, 0);
        }
        else if (newDirection == GravityDirection::Down) {
            testVelocity = QPointF(0, moveSpeed);
        }
        else {
            return false;
        }

        return canBallMoveToForVelocity(ball.position + testVelocity, testVelocity);
    }

    return false;
}

bool GameScene::tryEscapeCornerAndFall(const QPointF &basePosition,
                                        const QPointF &fallVelocity,
                                        QPointF *escapedPosition) const
{
    if (escapedPosition == nullptr) {
        return false;
    }

    // 最多横向探出一个格子，用于从内凹角脱困。
    const int maxEscapeDistance = TILE_SIZE;

    double firstSign = 1.0;
    double secondSign = -1.0;

    if (velocity.x() < 0) {
        firstSign = -1.0;
        secondSign = 1.0;
    }

    for (int offset = BALL_SPEED; offset <= maxEscapeDistance; offset += BALL_SPEED) {
        double signs[2] = { firstSign, secondSign };

        for (double directionSign : signs) {
            QPointF escapePosition(
                basePosition.x() + directionSign * offset,
                basePosition.y()
                );

            if (isWallAt(escapePosition)) {
                continue;
            }

            QPointF fallPosition = escapePosition + fallVelocity;

            if (canBallMoveTo(escapePosition)
                && canBallMoveTo(fallPosition)) {
                *escapedPosition = fallPosition;
                return true;
            }
        }
    }

    return false;
}

void GameScene::applyGravityAfterLeavingWall()
{
    if (ball.item == nullptr || gameEnded || isPaused) {
        return;
    }

    // 蹦床弹射期间不触发离墙坠落修正。
    if (isTrampolineLaunchMove || (velocity.x() != 0 && velocity.y() != 0)) {
        return;
    }

    // 宽投影和直接支撑不一致时，按内凹角脱困处理。
    if (gravityDirection == GravityDirection::Up) {
        QPointF fallVelocity = velocityForGravityDirection(GravityDirection::Up);

        if (!isTouchingWallAbove()) {
            velocity = fallVelocity;
            return;
        }

        if (!hasDirectSupportAbove()) {
            if (canBallMoveTo(ball.position + fallVelocity)) {
                velocity = fallVelocity;
                return;
            }

            QPointF escapedPosition;

            if (tryEscapeCornerAndFall(ball.position, fallVelocity, &escapedPosition)) {
                ball.setPosition(escapedPosition);
                velocity = fallVelocity;
                return;
            }
        }
    }
    else if (gravityDirection == GravityDirection::Down) {
        QPointF fallVelocity = velocityForGravityDirection(GravityDirection::Down);

        if (!isTouchingWallBelow()) {
            velocity = fallVelocity;
            return;
        }

        if (!hasDirectSupportBelow()) {
            if (canBallMoveTo(ball.position + fallVelocity)) {
                velocity = fallVelocity;
                return;
            }

            QPointF escapedPosition;

            if (tryEscapeCornerAndFall(ball.position, fallVelocity, &escapedPosition)) {
                ball.setPosition(escapedPosition);
                velocity = fallVelocity;
                return;
            }
        }
    }
    else {
        // 兼容旧状态：左右重力统一恢复为向下坠落。
        gravityDirection = GravityDirection::Down;
        velocity = velocityForGravityDirection(GravityDirection::Down);
    }
}

void GameScene::resetVelocityByGravity()
{
    if (gravityDirection == GravityDirection::Up) {
        velocity = QPointF(0, -moveSpeed);
    }
    else if (gravityDirection == GravityDirection::Down) {
        velocity = QPointF(0, moveSpeed);
    }
    else if (gravityDirection == GravityDirection::Left) {
        velocity = QPointF(-moveSpeed, 0);
    }
    else if (gravityDirection == GravityDirection::Right) {
        velocity = QPointF(moveSpeed, 0);
    }
}

QString GameScene::elapsedTimeText() const
{
    return QString::number(elapsedMs / 1000.0, 'f', 1);
}

void GameScene::checkCurrentTile()
{
    if (gameEnded) {
        return;
    }

    QChar currentTile = tileAtScenePos(ball.position);

    if (TileDefs::isKey(currentTile)) {
        collectKeyAtCurrentPosition();
        return;
    }

    if (TileDefs::isData(currentTile)) {
        collectDataFragmentAtCurrentPosition();
        return;
    }

    if (TileDefs::isDeath(currentTile)) {
        handleFailure();
        return;
    }

    if (TileDefs::isEnd(currentTile)) {
        handleVictory();
        return;
    }
}

void GameScene::handleFailure()
{
    if (gameEnded) {
        return;
    }

    deathCount++;

    timer->stop();
    isPaused = true;
    gameEnded = true;

    updateStatusText();

    QWidget *parentWidget = nullptr;

    if (!views().isEmpty()) {
        parentWidget = views().first();
    }

    QMessageBox::information(
        parentWidget,
        "游戏失败",
        QString("小球进入死亡区！\n\n用时：%1 秒\n反转次数：%2\n死亡次数：%3\n\n现在返回编辑模式，你可以继续调整候选点机关。")
            .arg(elapsedTimeText())
            .arg(reverseCount)
            .arg(deathCount)
        );

    enterEditMode();
}

void GameScene::handleVictory()
{
    if (gameEnded) {
        return;
    }

    timer->stop();
    isPaused = true;
    gameEnded = true;

    updateStatusText();

    Level currentLevel = isTemporaryTestLevel
                             ? temporaryTestLevel
                             : levelManager.levelAt(currentLevelIndex);

    const int stars = calculateStars();

    if (!isTemporaryTestLevel) {
        ProgressManager progressManager;
        QString progressError;
        if (!progressManager.updateAfterCompletion(currentLevel,
                                                   currentLevelIndex,
                                                   stars,
                                                   elapsedMs,
                                                   reverseCount,
                                                   deathCount,
                                                   &progressError)) {
            qWarning() << "Progress save failed:" << progressError;
        } else {
            qDebug() << "Progress saved for level:" << currentLevel.name;
        }
    }

    const CompletionAction action = showCompletionDialog(currentLevel, stars);

    if (action == CompletionAction::Next) {
        if (!isTemporaryTestLevel && levelManager.isValidLevelIndex(currentLevelIndex + 1)) {
            nextLevel();
            return;
        }
    }
    else if (action == CompletionAction::Restart) {
        restartLevel();
        return;
    }

    setFocus();
}

GameScene::CompletionAction GameScene::showCompletionDialog(const Level &currentLevel, int stars)
{
    QWidget *parentWidget = nullptr;

    if (!views().isEmpty()) {
        parentWidget = views().first();
    }

    const bool hasNextLevel = !isTemporaryTestLevel &&
                              levelManager.isValidLevelIndex(currentLevelIndex + 1);
    const bool passedReverseTarget = reverseCount <= currentLevel.targetReverseCount;
    const bool collectedAllData = totalDataFragmentCount > 0 &&
                                  collectedDataFragmentCount >= totalDataFragmentCount;

    QDialog dialog(parentWidget);
    dialog.setModal(true);
    dialog.setWindowTitle(isTemporaryTestLevel ? QStringLiteral("测试关卡结算")
                                               : QStringLiteral("通关结算"));
    dialog.setMinimumWidth(460);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(24, 22, 24, 20);
    mainLayout->setSpacing(14);

    QLabel *titleLabel = new QLabel(isTemporaryTestLevel
                                        ? QStringLiteral("测试通过！")
                                        : QStringLiteral("关卡完成！"), &dialog);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral("color: #f8f8f2;"));
    mainLayout->addWidget(titleLabel);

    QLabel *starLabel = new QLabel(starText(stars), &dialog);
    QFont starFont = starLabel->font();
    starFont.setPointSize(starFont.pointSize() + 14);
    starFont.setBold(true);
    starLabel->setFont(starFont);
    starLabel->setAlignment(Qt::AlignCenter);
    starLabel->setStyleSheet(QStringLiteral("color: #ffd166;"));
    mainLayout->addWidget(starLabel);

    QFrame *summaryFrame = new QFrame(&dialog);
    summaryFrame->setFrameShape(QFrame::StyledPanel);
    summaryFrame->setStyleSheet(QStringLiteral(
        "QFrame { background: #202638; border: 1px solid #3b4564; border-radius: 10px; }"
        "QLabel { color: #f8f8f2; background: transparent; border: none; }"
    ));

    QGridLayout *summaryLayout = new QGridLayout(summaryFrame);
    summaryLayout->setContentsMargins(16, 14, 16, 14);
    summaryLayout->setHorizontalSpacing(16);
    summaryLayout->setVerticalSpacing(8);

    auto addMetricRow = [&](int row, const QString &label, const QString &value, const QString &color = QString()) {
        QLabel *nameLabel = new QLabel(label, summaryFrame);
        nameLabel->setStyleSheet(QStringLiteral("color: #bdc7e6; background: transparent; border: none;"));

        QLabel *valueLabel = new QLabel(value, summaryFrame);
        valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (!color.isEmpty()) {
            valueLabel->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none; font-weight: 700;").arg(color));
        } else {
            valueLabel->setStyleSheet(QStringLiteral("color: #f8f8f2; background: transparent; border: none; font-weight: 700;"));
        }

        summaryLayout->addWidget(nameLabel, row, 0, Qt::AlignLeft | Qt::AlignTop);
        summaryLayout->addWidget(valueLabel, row, 1, Qt::AlignLeft | Qt::AlignTop);
    };

    int row = 0;
    addMetricRow(row++, QStringLiteral("关卡"), currentLevel.name);
    addMetricRow(row++, QStringLiteral("用时"), QStringLiteral("%1 秒").arg(elapsedTimeText()));
    addMetricRow(row++, QStringLiteral("反转次数"),
                 QStringLiteral("%1 / 目标 %2  %3")
                     .arg(reverseCount)
                     .arg(currentLevel.targetReverseCount)
                     .arg(passedReverseTarget ? QStringLiteral("达标") : QStringLiteral("未达标")),
                 passedReverseTarget ? QStringLiteral("#50fa7b") : QStringLiteral("#ffb86c"));

    const QString dataText = totalDataFragmentCount > 0
                                 ? QStringLiteral("%1 / %2  %3")
                                       .arg(collectedDataFragmentCount)
                                       .arg(totalDataFragmentCount)
                                       .arg(collectedAllData ? QStringLiteral("全收集") : QStringLiteral("未全收集"))
                                 : QStringLiteral("本关无数据碎片");
    addMetricRow(row++, QStringLiteral("数据碎片"), dataText,
                 (totalDataFragmentCount == 0 || collectedAllData) ? QStringLiteral("#50fa7b") : QStringLiteral("#ffb86c"));

    if (totalKeyCount > 0) {
        addMetricRow(row++, QStringLiteral("钥匙"),
                     QStringLiteral("%1 / %2").arg(collectedKeyCount).arg(totalKeyCount),
                     collectedKeyCount >= totalKeyCount ? QStringLiteral("#50fa7b") : QStringLiteral("#8be9fd"));
    }

    addMetricRow(row++, QStringLiteral("死亡次数"), QString::number(deathCount));
    addMetricRow(row++, QStringLiteral("星级"), starText(stars));

    if (!isTemporaryTestLevel) {
        ProgressManager progressManager;
        ProgressManager::Record bestRecord = progressManager.recordForLevel(currentLevel);
        if (bestRecord.completed) {
            addMetricRow(row++,
                         QStringLiteral("历史最佳"),
                         QStringLiteral("%1  最快 %2 秒  最少反转 %3  最少死亡 %4")
                             .arg(ProgressManager::starText(bestRecord.bestStars))
                             .arg(ProgressManager::formatTimeMs(bestRecord.bestTimeMs))
                             .arg(bestRecord.bestReverseCount)
                             .arg(bestRecord.bestDeathCount),
                         QStringLiteral("#8be9fd"));
        }
    }

    summaryLayout->setColumnStretch(1, 1);
    mainLayout->addWidget(summaryFrame);

    QFrame *ruleFrame = new QFrame(&dialog);
    ruleFrame->setFrameShape(QFrame::NoFrame);
    ruleFrame->setStyleSheet(QStringLiteral(
        "QFrame { background: #161b29; border: 1px solid #2c344d; border-radius: 8px; }"
        "QLabel { color: #d7def7; background: transparent; border: none; }"
    ));

    QVBoxLayout *ruleLayout = new QVBoxLayout(ruleFrame);
    ruleLayout->setContentsMargins(14, 12, 14, 12);
    ruleLayout->setSpacing(6);

    QLabel *ruleTitle = new QLabel(QStringLiteral("星级计算"), ruleFrame);
    QFont ruleTitleFont = ruleTitle->font();
    ruleTitleFont.setBold(true);
    ruleTitle->setFont(ruleTitleFont);
    ruleTitle->setStyleSheet(QStringLiteral("color: #8be9fd; background: transparent; border: none;"));
    ruleLayout->addWidget(ruleTitle);

    QStringList ruleLines;
    ruleLines << QStringLiteral("✓ 完成关卡：+1 星");
    ruleLines << (passedReverseTarget
                      ? QStringLiteral("✓ 反转次数未超过目标：+1 星")
                      : QStringLiteral("— 反转次数超过目标：+0 星"));

    if (totalDataFragmentCount > 0) {
        ruleLines << (collectedAllData
                          ? QStringLiteral("✓ 收集全部数据碎片：+1 星")
                          : QStringLiteral("— 未收集全部数据碎片：+0 星"));
    } else {
        ruleLines << QStringLiteral("— 本关无数据碎片：该项不计星");
    }

    for (const QString &line : ruleLines) {
        QLabel *lineLabel = new QLabel(line, ruleFrame);
        lineLabel->setWordWrap(true);
        ruleLayout->addWidget(lineLabel);
    }

    mainLayout->addWidget(ruleFrame);

    QLabel *hintLabel = new QLabel(&dialog);
    hintLabel->setWordWrap(true);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet(QStringLiteral("color: #bdc7e6;"));
    if (isTemporaryTestLevel) {
        hintLabel->setText(QStringLiteral("当前设计器关卡可以正常通关。你可以继续测试，或关闭结果返回当前测试画面。"));
    } else if (hasNextLevel) {
        hintLabel->setText(QStringLiteral("成绩已保存，下一关已解锁。你可以进入下一关，也可以重玩本关挑战更高星级。"));
    } else {
        hintLabel->setText(QStringLiteral("成绩已保存。你已经完成最后一关，可以重玩本关继续挑战成绩。"));
    }
    mainLayout->addWidget(hintLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    buttonLayout->addStretch();

    CompletionAction selectedAction = CompletionAction::Stay;

    QPushButton *stayButton = new QPushButton(isTemporaryTestLevel
                                                  ? QStringLiteral("关闭结果")
                                                  : QStringLiteral("停留查看"), &dialog);
    QPushButton *restartButton = new QPushButton(isTemporaryTestLevel
                                                     ? QStringLiteral("重新测试")
                                                     : QStringLiteral("重玩本关"), &dialog);

    stayButton->setMinimumHeight(34);
    restartButton->setMinimumHeight(34);

    buttonLayout->addWidget(stayButton);
    buttonLayout->addWidget(restartButton);

    QObject::connect(stayButton, &QPushButton::clicked, &dialog, [&]() {
        selectedAction = CompletionAction::Stay;
        dialog.accept();
    });

    QObject::connect(restartButton, &QPushButton::clicked, &dialog, [&]() {
        selectedAction = CompletionAction::Restart;
        dialog.accept();
    });

    QPushButton *nextButton = nullptr;
    if (hasNextLevel) {
        nextButton = new QPushButton(QStringLiteral("下一关"), &dialog);
        nextButton->setMinimumHeight(34);
        nextButton->setDefault(true);
        buttonLayout->addWidget(nextButton);

        QObject::connect(nextButton, &QPushButton::clicked, &dialog, [&]() {
            selectedAction = CompletionAction::Next;
            dialog.accept();
        });
    } else {
        restartButton->setDefault(true);
    }

    mainLayout->addLayout(buttonLayout);

    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background: #0f1421; }"
        "QPushButton { padding: 7px 16px; border-radius: 6px; background: #38415f; color: #f8f8f2; }"
        "QPushButton:hover { background: #4a567c; }"
        "QPushButton:default { background: #4b6cff; }"
    ));

    dialog.exec();

    return selectedAction;
}

void GameScene::applyPortalEffect(QChar currentTile)
{
    if (!TileDefs::isPortal(currentTile)) {
        // 只有离开传送门格后，才解除目标传送门锁，避免刚传过去又立即传回来。
        if (!teleportLockPortalKey.isEmpty()) {
            QPoint currentGridPos = gridPosAtScenePos(ball.position);
            if (gridKey(currentGridPos) != teleportLockPortalKey) {
                teleportLockPortalKey.clear();
            }
        }
        return;
    }

    tryTeleportAtCurrentPosition();
}

void GameScene::rebuildPortalPairs()
{
    portalPairTargets.clear();

    QVector<QPoint> portals;

    for (int row = 0; row < mapData.size(); ++row) {
        for (int col = 0; col < mapData[row].size(); ++col) {
            if (TileDefs::isPortal(mapData[row][col])) {
                portals.append(QPoint(col, row));
            }
        }
    }

    // 传送门按读取顺序两两配对：第 1 个 <-> 第 2 个，第 3 个 <-> 第 4 个，以此类推。
    for (int i = 0; i + 1 < portals.size(); i += 2) {
        const QPoint first = portals[i];
        const QPoint second = portals[i + 1];
        portalPairTargets.insert(gridKey(first), second);
        portalPairTargets.insert(gridKey(second), first);
    }
}

bool GameScene::tryTeleportAtCurrentPosition()
{
    QPoint currentGridPos = gridPosAtScenePos(ball.position);
    const QString currentKey = gridKey(currentGridPos);

    if (currentKey == teleportLockPortalKey) {
        return false;
    }

    if (!portalPairTargets.contains(currentKey)) {
        return false;
    }

    const QPoint targetGridPos = portalPairTargets.value(currentKey);

    if (!isGridPosInMap(targetGridPos)) {
        return false;
    }

    const QPointF targetPosition = gridCenterToScenePos(targetGridPos);

    if (!canBallMoveTo(targetPosition)) {
        return false;
    }

    ball.setPosition(targetPosition);
    lastNonLaserBallPosition = targetPosition;
    teleportLockPortalKey = gridKey(targetGridPos);

    qDebug() << "Portal teleport:" << currentGridPos << "->" << targetGridPos;
    return true;
}

void GameScene::adjustVelocityToSpeed(int newSpeed)
{
    moveSpeed = newSpeed;

    if (velocity.x() > 0) {
        velocity.setX(moveSpeed);
    }
    else if (velocity.x() < 0) {
        velocity.setX(-moveSpeed);
    }

    if (velocity.y() > 0) {
        velocity.setY(moveSpeed);
    }
    else if (velocity.y() < 0) {
        velocity.setY(-moveSpeed);
    }
}

void GameScene::applyTileEffects()
{
    QChar currentTile = tileAtScenePos(ball.position);

    // 速度类机关。
    applySpeedEffect(currentTile);

    // 方向类机关。
    applyBounceEffect(currentTile);
    applyTrampolineEffect(currentTile);

    // 位置类机关。
    applyConveyorEffect(currentTile);
    applyPortalEffect(currentTile);
}
void GameScene::applySpeedEffect(QChar currentTile)
{
    // 缓冲区减速，离开后恢复。
    if (TileDefs::isSlow(currentTile)) {
        adjustVelocityToSpeed(SLOW_SPEED);
    } else {
        adjustVelocityToSpeed(BALL_SPEED);
    }

}
void GameScene::applyBounceEffect(QChar currentTile)
{
    // 旧版弹射块：强制向上。
    if (!TileDefs::isBounce(currentTile)) {
        return;
    }

    gravityDirection = GravityDirection::Up;
    moveSpeed = BALL_SPEED;
    velocity = QPointF(0, -moveSpeed);
}

void GameScene::applyTrampolineEffect(QChar currentTile)
{
    if (!TileDefs::isTrampoline(currentTile)) {
        wasOnTrampoline = false;
        return;
    }

    // 只有竖直进入蹦床才触发。
    if (velocity.y() == 0) {
        return;
    }

    const bool isHorizontalTrampoline =
        TileDefs::isTrampolineRight(currentTile)
        || TileDefs::isTrampolineLeft(currentTile);

    // 水平蹦床等角色到达格子中心后再触发，避免墙角卡死。
    if (isHorizontalTrampoline) {
        QPoint gridPos = gridPosAtScenePos(ball.position);

        const double centerX = gridPos.x() * TILE_SIZE + TILE_SIZE / 2.0;
        const double centerY = gridPos.y() * TILE_SIZE + TILE_SIZE / 2.0;

        if (velocity.y() > 0 && ball.position.y() < centerY) {
            wasOnTrampoline = false;
            return;
        }

        if (velocity.y() < 0 && ball.position.y() > centerY) {
            wasOnTrampoline = false;
            return;
        }

        // 校准到格子中心后再弹出。
        ball.setPosition(QPointF(centerX, centerY));
    }

    // 同一蹦床格子只触发一次。
    if (wasOnTrampoline) {
        return;
    }

    wasOnTrampoline = true;

    moveSpeed = BALL_SPEED;
    isTrampolineLaunchMove = true;

    if (TileDefs::isTrampolineUpRight(currentTile)) {
        gravityDirection = GravityDirection::Up;
        velocity = QPointF(moveSpeed, -moveSpeed);
    }
    else if (TileDefs::isTrampolineUpLeft(currentTile)) {
        gravityDirection = GravityDirection::Up;
        velocity = QPointF(-moveSpeed, -moveSpeed);
    }
    else if (TileDefs::isTrampolineDownRight(currentTile)) {
        gravityDirection = GravityDirection::Down;
        velocity = QPointF(moveSpeed, moveSpeed);
    }
    else if (TileDefs::isTrampolineDownLeft(currentTile)) {
        gravityDirection = GravityDirection::Down;
        velocity = QPointF(-moveSpeed, moveSpeed);
    }
    else if (TileDefs::isTrampolineRight(currentTile)) {
        gravityDirection = GravityDirection::Right;
        velocity = QPointF(moveSpeed, 0);
    }
    else if (TileDefs::isTrampolineLeft(currentTile)) {
        gravityDirection = GravityDirection::Left;
        velocity = QPointF(-moveSpeed, 0);
    }

    qDebug() << "Directional trampoline triggered at center when needed."
             << TileDefs::nameOf(currentTile)
             << "Arrow:" << TileDefs::trampolineArrow(currentTile)
             << "Gravity:" << gravityDirectionToString()
             << "Velocity:" << velocity
             << "BallPos:" << ball.position;
}

void GameScene::applyConveyorEffect(QChar currentTile)
{
    // 旧版传送带：向右推送。
    if (!TileDefs::isConveyor(currentTile)) {
        return;
    }

    QPointF conveyorPosition(
        ball.position.x() + CONVEYOR_SPEED,
        ball.position.y()
        );

    if (canBallMoveTo(conveyorPosition)) {
        ball.setPosition(conveyorPosition);
    }
}
void GameScene::refreshStatus()
{
    updateStatusText();
}
void GameScene::saveCurrentEditedLevel()
{
    QWidget *parentWidget = nullptr;

    if (!views().isEmpty()) {
        parentWidget = views().first();
    }

    if (!isTemporaryTestLevel && !levelManager.isValidLevelIndex(currentLevelIndex)) {
        QMessageBox::warning(
            parentWidget,
            "保存失败",
            "当前关卡编号无效，不能保存。"
            );

        setFocus();
        return;
    }

    Level currentLevel = isTemporaryTestLevel
                             ? temporaryTestLevel
                             : levelManager.levelAt(currentLevelIndex);

    bool ok = false;

    QString levelName = QInputDialog::getText(
        parentWidget,
        "保存自定义地图",
        "请输入自定义关卡名：",
        QLineEdit::Normal,
        defaultCustomLevelName(),
        &ok
        );

    if (!ok) {
        setFocus();
        return;
    }

    levelName = levelName.trimmed();

    if (levelName.isEmpty()) {
        QMessageBox::warning(
            parentWidget,
            "拒绝保存",
            "关卡名不能为空。"
            );

        setFocus();
        return;
    }

    // 保存编辑地图，不保存运行中被碎片收集改变的临时地图。
    QStringList saveMapData;

    if (!editMapData.isEmpty()) {
        saveMapData = editMapData;
    } else {
        saveMapData = mapData;
    }

    Level saveLevel(
        levelName,
        saveMapData,
        currentLevel.targetReverseCount,
        candidateEditPoints
        );

    QString errorMessage;

    if (!levelManager.validateLevelForSave(saveLevel, &errorMessage)) {
        QMessageBox::warning(
            parentWidget,
            "拒绝保存",
            QString("地图不合法，不能保存。\n\n错误原因：%1")
                .arg(errorMessage)
            );

        setFocus();
        return;
    }

    QString filePath = createCustomLevelFilePath(levelName);

    if (!levelManager.saveLevelToFile(saveLevel, filePath, &errorMessage)) {
        QMessageBox::critical(
            parentWidget,
            "保存失败",
            QString("保存自定义地图失败。\n\n错误原因：%1")
                .arg(errorMessage)
            );

        setFocus();
        return;
    }

    // 保存后立即回读校验。
    Level reloadedLevel;

    if (!levelManager.readLevelFromFile(filePath, &reloadedLevel, &errorMessage)) {
        QMessageBox::warning(
            parentWidget,
            "保存后校验失败",
            QString("文件已经写入，但重新读取失败。\n\n路径：%1\n\n错误原因：%2")
                .arg(QDir::toNativeSeparators(filePath))
                .arg(errorMessage)
            );

        setFocus();
        return;
    }

    bool sameLevel =
        reloadedLevel.name == saveLevel.name
        && reloadedLevel.targetReverseCount == saveLevel.targetReverseCount
        && reloadedLevel.mapData == saveLevel.mapData
        && reloadedLevel.editablePoints == saveLevel.editablePoints;

    if (!sameLevel) {
        QMessageBox::warning(
            parentWidget,
            "保存后校验失败",
            QString("文件已经写入，但重新读取后的内容和保存前不一致。\n\n路径：%1")
                .arg(QDir::toNativeSeparators(filePath))
            );

        setFocus();
        return;
    }

    // 加入当前关卡列表，避免重启后才可选择。
    levelManager.loadLevelFromFile(filePath, true);

    updateStatusText();

    QMessageBox::information(
        parentWidget,
        "保存成功",
        QString("保存成功！\n\n保存路径：\n%1\n\n已验证：保存后的地图可以重新读取，并且内容和保存前一致。")
            .arg(QDir::toNativeSeparators(filePath))
        );

    setFocus();
}
void GameScene::enterEditMode()
{
    if (!editMapData.isEmpty()) {
        mapData = editMapData;
    } else {
        editMapData = mapData;
    }

    timer->stop();

    isEditMode = true;
    isPaused = true;
    gameEnded = false;

    resetRuntimeStateForCurrentMap();

    drawMap();
    updateStatusText();
    setFocus();

    qDebug() << "Entered edit mode.";
}

void GameScene::startRunMode()
{
    if (isEditMode) {
        editMapData = mapData;
    } else if (!editMapData.isEmpty()) {
        mapData = editMapData;
    }

    isEditMode = false;
    isPaused = false;
    gameEnded = false;

    resetRuntimeStateForCurrentMap();

    drawMap();

    timer->start(TIMER_INTERVAL);

    updateStatusText();
    setFocus();

    qDebug() << "Started run mode from edited map.";
}

void GameScene::resetRuntimeStateForCurrentMap()
{
    gravityDirection = GravityDirection::Down;

    moveSpeed = BALL_SPEED;
    resetVelocityByGravity();

    reverseCount = 0;
    collectedDataFragmentCount = 0;
    totalDataFragmentCount = countDataFragments();
    collectedKeyCount = 0;
    totalKeyCount = countKeys();
    teleportLockPortalKey.clear();
    rebuildPortalPairs();

    elapsedMs = 0;
    gameEnded = false;
    wasOnTrampoline = false;
    isTrampolineLaunchMove = false;
}

void GameScene::selectSlowBlock()
{
    selectEditTile(TileDefs::Slow);
}

void GameScene::selectTrampolineBlock()
{
    selectTrampolineUpRightBlock();
}

void GameScene::selectTrampolineUpRightBlock()
{
    selectEditTile(TileDefs::TrampolineUpRight);
}

void GameScene::selectTrampolineUpLeftBlock()
{
    selectEditTile(TileDefs::TrampolineUpLeft);
}

void GameScene::selectTrampolineDownRightBlock()
{
    selectEditTile(TileDefs::TrampolineDownRight);
}

void GameScene::selectTrampolineDownLeftBlock()
{
    selectEditTile(TileDefs::TrampolineDownLeft);
}

void GameScene::selectTrampolineRightBlock()
{
    selectEditTile(TileDefs::TrampolineRight);
}

void GameScene::selectTrampolineLeftBlock()
{
    selectEditTile(TileDefs::TrampolineLeft);
}

void GameScene::selectLaserBlock()
{
    selectEditTile(TileDefs::Laser);
}

void GameScene::selectEditTile(QChar tile)
{
    if (!isEditableMechanism(tile)) {
        return;
    }

    selectedEditTile = tile;

    updateStatusText();
    setFocus();

    qDebug() << "Selected edit tile:" << TileDefs::nameOf(tile);
}

QString GameScene::selectedEditTileName() const
{
    return TileDefs::nameOf(selectedEditTile);
}

void GameScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!isEditMode) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    QPoint gridPos = gridPosAtScenePos(event->scenePos());

    if (!isGridPosInMap(gridPos)) {
        event->accept();
        return;
    }

    // 玩家编辑模式只能修改候选点。
    if (!isCandidateEditPoint(gridPos)) {
        qDebug() << "Clicked non-candidate point:" << gridPos;
        event->accept();
        return;
    }

    QChar currentTile = tileAtGridPos(gridPos);

    if (event->button() == Qt::LeftButton) {
        // 下方按钮选中了什么，就直接把候选点改成什么。
        // 不再让玩家反复点同一个格子循环很多次。
        setTileAtGridPos(gridPos, selectedEditTile);
        redrawEditedMap();

        qDebug() << "Candidate point set directly:"
                 << gridPos
                 << TileDefs::nameOf(currentTile)
                 << "->"
                 << TileDefs::nameOf(selectedEditTile);
    }
    else if (event->button() == Qt::RightButton) {
        setTileAtGridPos(gridPos, TileDefs::Empty);
        redrawEditedMap();

        qDebug() << "Candidate point reset to empty:" << gridPos;
    }

    event->accept();
    setFocus();
}

bool GameScene::isGridPosInMap(const QPoint &gridPos) const
{
    int col = gridPos.x();
    int row = gridPos.y();

    if (row < 0 || row >= mapData.size()) {
        return false;
    }

    if (col < 0 || col >= mapData[row].size()) {
        return false;
    }

    return true;
}

bool GameScene::isEditableMechanism(QChar tile) const
{
    return TileDefs::isSlow(tile)
           || TileDefs::isTrampoline(tile)
           || TileDefs::isLaser(tile);
}

QChar GameScene::tileAtGridPos(const QPoint &gridPos) const
{
    if (!isGridPosInMap(gridPos)) {
        return TileDefs::Wall;
    }

    return mapData[gridPos.y()][gridPos.x()];
}

void GameScene::setTileAtGridPos(const QPoint &gridPos, QChar tile)
{
    if (!isGridPosInMap(gridPos)) {
        return;
    }

    QString rowText = mapData[gridPos.y()];
    rowText[gridPos.x()] = tile;
    mapData[gridPos.y()] = rowText;
}

void GameScene::redrawEditedMap()
{
    editMapData = mapData;

    collectedDataFragmentCount = 0;
    totalDataFragmentCount = countDataFragments();

    drawMap();
    updateStatusText();
    setFocus();
}

void GameScene::rebuildCandidateEditPointKeys()
{
    candidateEditPointKeys.clear();

    for (const QPoint &point : candidateEditPoints) {
        candidateEditPointKeys.insert(gridKey(point));
    }
}

QVector<QPoint> GameScene::fallbackCandidateEditPoints(int maxCount) const
{
    QVector<QPoint> points;

    for (int row = 0; row < mapData.size(); ++row) {
        for (int col = 0; col < mapData[row].size(); ++col) {
            QChar tile = mapData[row][col];

            if (TileDefs::isEmpty(tile) || isEditableMechanism(tile)) {
                points.append(QPoint(col, row));

                if (points.size() >= maxCount) {
                    return points;
                }
            }
        }
    }

    return points;
}

bool GameScene::isCandidateEditPoint(const QPoint &gridPos) const
{
    return candidateEditPointKeys.contains(gridKey(gridPos));
}

QChar GameScene::nextCandidateTile(QChar currentTile) const
{
    // 兼容旧版点击循环；新逻辑优先使用按钮选择。
    if (TileDefs::isEmpty(currentTile)) {
        return TileDefs::Slow;
    }

    if (TileDefs::isSlow(currentTile)) {
        return TileDefs::TrampolineUpRight;
    }

    if (TileDefs::isTrampolineUpRight(currentTile)) {
        return TileDefs::TrampolineUpLeft;
    }

    if (TileDefs::isTrampolineUpLeft(currentTile)) {
        return TileDefs::TrampolineDownRight;
    }

    if (TileDefs::isTrampolineDownRight(currentTile)) {
        return TileDefs::TrampolineDownLeft;
    }

    if (TileDefs::isTrampolineDownLeft(currentTile)) {
        return TileDefs::TrampolineRight;
    }

    if (TileDefs::isTrampolineRight(currentTile)) {
        return TileDefs::TrampolineLeft;
    }

    if (TileDefs::isTrampolineLeft(currentTile)) {
        return TileDefs::Laser;
    }

    if (TileDefs::isLaser(currentTile)) {
        return TileDefs::Empty;
    }

    return TileDefs::Slow;
}

void GameScene::drawCandidateEditPoints()
{
    if (!isEditMode) {
        return;
    }

    static QPixmap candidatePixmap(":/images/resources/images/candidate_edit.png");

    QFont hintFont("Microsoft YaHei", 8, QFont::Bold);

    for (const QPoint &gridPos : candidateEditPoints) {
        if (!isGridPosInMap(gridPos)) {
            continue;
        }

        int x = gridPos.x() * TILE_SIZE;
        int y = gridPos.y() * TILE_SIZE;

        if (!candidatePixmap.isNull()) {
            QGraphicsPixmapItem *p = addPixmap(candidatePixmap);
            p->setPos(x, y);
            p->setZValue(12);
        } else {
            QRectF rect(x + 4, y + 4, TILE_SIZE - 8, TILE_SIZE - 8);
            QPen candidatePen(QColor("#80f7ff"));
            candidatePen.setWidth(2);
            candidatePen.setStyle(Qt::DashLine);
            QBrush candidateBrush(QColor(128, 247, 255, 45));
            QGraphicsRectItem *candidateRect = addRect(rect, candidatePen, candidateBrush);
            candidateRect->setZValue(12);
        }

        QGraphicsSimpleTextItem *hintText = addSimpleText("可改", hintFont);
        hintText->setBrush(QColor("#80f7ff"));
        hintText->setZValue(13);
        QRectF textRect = hintText->boundingRect();
        hintText->setPos(x + TILE_SIZE / 2.0 - textRect.width() / 2.0,
                         y + TILE_SIZE / 2.0 - textRect.height() / 2.0);
    }
}

int GameScene::countDataFragments() const
{
    int count = 0;

    for (int row = 0; row < mapData.size(); ++row) {
        for (int col = 0; col < mapData[row].size(); ++col) {
            if (TileDefs::isData(mapData[row][col])) {
                count++;
            }
        }
    }

    return count;
}

int GameScene::countKeys() const
{
    int count = 0;

    for (int row = 0; row < mapData.size(); ++row) {
        for (int col = 0; col < mapData[row].size(); ++col) {
            if (TileDefs::isKey(mapData[row][col])) {
                count++;
            }
        }
    }

    return count;
}

QPoint GameScene::gridPosAtScenePos(const QPointF &scenePos) const
{
    int col = static_cast<int>(scenePos.x()) / TILE_SIZE;
    int row = static_cast<int>(scenePos.y()) / TILE_SIZE;

    return QPoint(col, row);
}

QString GameScene::gridKey(const QPoint &gridPos) const
{
    return QString("%1,%2")
    .arg(gridPos.x())
        .arg(gridPos.y());
}

void GameScene::collectDataFragmentAtCurrentPosition()
{
    QPoint gridPos = gridPosAtScenePos(ball.position);

    int col = gridPos.x();
    int row = gridPos.y();

    if (row < 0 || row >= mapData.size()) {
        return;
    }

    if (col < 0 || col >= mapData[row].size()) {
        return;
    }

    if (!TileDefs::isData(mapData[row][col])) {
        return;
    }

    QString rowText = mapData[row];
    rowText[col] = TileDefs::Empty;
    mapData[row] = rowText;

    collectedDataFragmentCount++;

    QString key = gridKey(gridPos);

    if (dataFragmentItems.contains(key)) {
        QGraphicsItem *item = dataFragmentItems.take(key);
        removeItem(item);
        delete item;
    }

    // 碎片收集后恢复为空地背景。
    QPixmap emptyBgPixmap(":/images/resources/images/empty.png");
    if (!emptyBgPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = addPixmap(emptyBgPixmap.scaled(
            TILE_SIZE, TILE_SIZE,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));
        bgItem->setPos(gridPos.x() * TILE_SIZE, gridPos.y() * TILE_SIZE);
        bgItem->setZValue(0);
    }

    updateStatusText();

    qDebug() << "Data fragment collected:"
             << collectedDataFragmentCount
             << "/"
             << totalDataFragmentCount;
}

void GameScene::collectKeyAtCurrentPosition()
{
    QPoint gridPos = gridPosAtScenePos(ball.position);

    int col = gridPos.x();
    int row = gridPos.y();

    if (row < 0 || row >= mapData.size()) {
        return;
    }

    if (col < 0 || col >= mapData[row].size()) {
        return;
    }

    if (!TileDefs::isKey(mapData[row][col])) {
        return;
    }

    QString rowText = mapData[row];
    rowText[col] = TileDefs::Empty;
    mapData[row] = rowText;

    collectedKeyCount++;

    QString key = gridKey(gridPos);

    if (keyItems.contains(key)) {
        QGraphicsItem *item = keyItems.take(key);
        removeItem(item);
        delete item;
    }

    // 钥匙拾取后恢复为空地背景。
    QPixmap emptyBgPixmap(":/images/resources/images/empty.png");
    if (!emptyBgPixmap.isNull()) {
        QGraphicsPixmapItem *bgItem = addPixmap(emptyBgPixmap.scaled(
            TILE_SIZE, TILE_SIZE,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));
        bgItem->setPos(gridPos.x() * TILE_SIZE, gridPos.y() * TILE_SIZE);
        bgItem->setZValue(0);
    }

    updateDoorItems();
    updateStatusText();

    qDebug() << "Key collected:" << collectedKeyCount << "/" << totalKeyCount;
}

int GameScene::calculateStars() const
{
    int stars = 1;

    int targetReverseCount = 0;
    bool hasTarget = false;

    if (isTemporaryTestLevel) {
        targetReverseCount = temporaryTestLevel.targetReverseCount;
        hasTarget = true;
    }
    else if (levelManager.isValidLevelIndex(currentLevelIndex)) {
        Level currentLevel = levelManager.levelAt(currentLevelIndex);
        targetReverseCount = currentLevel.targetReverseCount;
        hasTarget = true;
    }

    if (hasTarget && reverseCount <= targetReverseCount) {
        stars++;
    }

    if (totalDataFragmentCount > 0 &&
        collectedDataFragmentCount >= totalDataFragmentCount) {
        stars++;
    }

    return stars;
}
QString GameScene::defaultCustomLevelName() const
{
    if (isTemporaryTestLevel) {
        return temporaryTestLevel.name + "_自定义";
    }

    if (levelManager.isValidLevelIndex(currentLevelIndex)) {
        Level currentLevel = levelManager.levelAt(currentLevelIndex);
        return currentLevel.name + "_自定义";
    }

    return "自定义地图";
}

QString GameScene::makeSafeFileBaseName(const QString &text) const
{
    QString result = text.trimmed();

    result.replace(QRegularExpression("[\\\\/:*?\"<>|\\s]+"), "_");
    result.replace(QRegularExpression("_+"), "_");

    if (result.isEmpty()) {
        result = "custom_level";
    }

    if (result.size() > 60) {
        result = result.left(60);
    }

    return result;
}

QString GameScene::createCustomLevelFilePath(const QString &levelName) const
{
    QString folderPath = levelManager.customLevelFolderPath();

    QDir().mkpath(folderPath);

    QDir dir(folderPath);

    QString timeText = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    QString fileName = QString("%1_%2.json")
                           .arg(makeSafeFileBaseName(levelName))
                           .arg(timeText);

    return dir.filePath(fileName);
}

QString GameScene::starText(int stars) const
{
    QString text;

    for (int i = 0; i < stars; ++i) {
        text += "★";
    }

    for (int i = stars; i < 3; ++i) {
        text += "☆";
    }

    return text;
}
QString GameScene::resolveBallMoviePath() const
{
    const bool inTheAir = !hasAnyWallContact();

    // 空中使用坠落动画。
    if (inTheAir) {
        if (gravityDirection == GravityDirection::Up) {
            return ":/images/resources/images/character_in_theair_and_graveup.gif";
        }
        // 左右重力复用向下坠落图。
        return ":/images/resources/images/character_in_theair_and_gravedown.gif";
    }

    // 贴墙时按重力和水平速度选择动画。
    if (gravityDirection == GravityDirection::Down) {
        return (velocity.x() < 0)
            ? ":/images/resources/images/character_gravedown_workleft.gif"
            : ":/images/resources/images/character_gravedown_workright.gif";
    }
    if (gravityDirection == GravityDirection::Up) {
        return (velocity.x() < 0)
            ? ":/images/resources/images/character_graveup_workleft.gif"
            : ":/images/resources/images/character_graveup_workright.gif";
    }
    // 左右重力暂无专用贴图，回退默认动画。
    return ":/images/resources/images/character_gravedown_workright.gif";
}

void GameScene::updateBallMovie()
{
    if (ball.item == nullptr) {
        return;
    }

    QString desiredPath = resolveBallMoviePath();
    if (desiredPath == currentBallMoviePath) {
        return;   // 路径没变，无需重新加载
    }
    currentBallMoviePath = desiredPath;

    // 释放旧动画。
    if (ballMovie != nullptr) {
        ballMovie->stop();
        delete ballMovie;
        ballMovie = nullptr;
    }

    const int targetSize = ball.radius * 3;

    // 加载新动画。
    ballMovie = new QMovie(desiredPath);
    if (ballMovie->isValid()) {
        connect(ballMovie, &QMovie::frameChanged, this, [this, targetSize]() {
            if (ball.item != nullptr && ballMovie != nullptr) {
                QPixmap frame = ballMovie->currentPixmap().scaled(
                    targetSize, targetSize,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
                ball.item->setPixmap(frame);
                ball.item->setOffset(-frame.width() / 2.0, -frame.height() / 2.0);
            }
        });
        ballMovie->start();

        // 先显示第一帧，避免闪烁。
        QPixmap firstFrame = ballMovie->currentPixmap().scaled(
            targetSize, targetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        ball.item->setPixmap(firstFrame);
        ball.item->setOffset(-firstFrame.width() / 2.0, -firstFrame.height() / 2.0);
    } else {
        // 动画加载失败时回退静态图。
        static QPixmap fallbackPixmap(":/images/resources/images/ball.png");
        QPixmap pixmapToUse = fallbackPixmap;
        if (!pixmapToUse.isNull()) {
            pixmapToUse = pixmapToUse.scaled(
                targetSize, targetSize,
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            ball.item->setPixmap(pixmapToUse);
        }
    }
}
