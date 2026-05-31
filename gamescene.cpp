#include "gamescene.h"
#include "constants.h"
#include "tiledefs.h"


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




GameScene::GameScene(QObject *parent)
    : QGraphicsScene(parent)
    , currentLevelIndex(0)
    , timer(new QTimer(this))
    , gravityDirection(GravityDirection::Down)
    , velocity(0, BALL_SPEED)
    , moveSpeed(BALL_SPEED)
    , reverseCount(0)
    , deathCount(0)
    , collectedDataFragmentCount(0)
    , totalDataFragmentCount(0)
    , isPaused(false)
    , gameEnded(false)
    , wasOnTrampoline(false)
    , elapsedMs(0)
    , statusText(nullptr)
    , isEditMode(false)
    , selectedEditTile(TileDefs::Bounce)
{
    levelManager.loadDefaultLevels();
    loadLevel(0);

    connect(timer, &QTimer::timeout, this, &GameScene::updateGame);
    timer->start(TIMER_INTERVAL);

    setFocus();

    qDebug() << "Stage 14 started: LevelManager enabled.";
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

    currentLevelIndex = levelIndex;

    Level currentLevel = levelManager.levelAt(currentLevelIndex);
    mapData = currentLevel.mapData;

    // 阶段 20：读取当前关卡的固定候选编辑点
    candidateEditPoints = currentLevel.editablePoints;

    // 如果旧 JSON 没有写 editablePoints，自动从空地里挑几个点，方便演示
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

    elapsedMs = 0;
    wasOnTrampoline = false;

    isPaused = false;
    gameEnded = false;

    drawMap();

    timer->start(TIMER_INTERVAL);

    updateStatusText();
    setFocus();

    qDebug() << "Loaded level:" << currentLevel.name;
}



void GameScene::drawMap()
{
    clear();

    // 阶段 12 新增：清空数据碎片图形记录
    dataFragmentItems.clear();

    ball.item = nullptr;
    statusText = nullptr;

    startGridPos = QPoint(-1, -1);
    endGridPos = QPoint(-1, -1);

    const int rows = mapData.size();
    const int cols = mapData[0].size();

    setSceneRect(0, 0, cols * TILE_SIZE, rows * TILE_SIZE);
    setBackgroundBrush(QBrush(QColor("#10131f")));

    drawGridBackground(rows, cols);

    QPen wallPen(QColor("#596275"));
    QBrush wallBrush(QColor("#3b4252"));

    QPen endPen(QColor("#2ecc71"));
    QBrush endBrush(QColor("#2ecc71"));

    QPen deathPen(QColor("#ff4d4d"));
    QBrush deathBrush(QColor("#b83232"));

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            const QChar tile = mapData[row][col];

            const int x = col * TILE_SIZE;
            const int y = row * TILE_SIZE;

            if (TileDefs::isWall(tile)) {
                addRect(x, y, TILE_SIZE, TILE_SIZE, wallPen, wallBrush);
            }
            else if (TileDefs::isStart(tile)) {
                startGridPos = QPoint(col, row);
            }
            else if (TileDefs::isEnd(tile)) {
                endGridPos = QPoint(col, row);

                const int inset = 6;

                QRectF endRect(
                    x + inset,
                    y + inset,
                    TILE_SIZE - 2 * inset,
                    TILE_SIZE - 2 * inset
                    );

                addRect(endRect, endPen, endBrush);

                QFont endFont("Arial", 8, QFont::Bold);

                QGraphicsSimpleTextItem *text = addSimpleText("END", endFont);
                text->setBrush(Qt::white);

                QRectF textRect = text->boundingRect();

                text->setPos(
                    endRect.center().x() - textRect.width() / 2,
                    endRect.center().y() - textRect.height() / 2 + 1
                    );
            }
            else if (TileDefs::isDeath(tile)) {
                addRect(
                    x + 4,
                    y + 4,
                    TILE_SIZE - 8,
                    TILE_SIZE - 8,
                    deathPen,
                    deathBrush
                    );
            }
            else if (TileDefs::isBounce(tile)) {
                QRectF rect(
                    x + 6,
                    y + 6,
                    TILE_SIZE - 12,
                    TILE_SIZE - 12
                    );

                addRect(
                    rect,
                    QPen(QColor("#f1c40f")),
                    QBrush(QColor("#f39c12"))
                    );

                addCenteredTextInRect("5", rect, Qt::white);
            }
            else if (TileDefs::isSlow(tile)) {
                QRectF rect(
                    x + 6,
                    y + 6,
                    TILE_SIZE - 12,
                    TILE_SIZE - 12
                    );

                addRect(
                    rect,
                    QPen(QColor("#74b9ff")),
                    QBrush(QColor("#0984e3"))
                    );

                addCenteredTextInRect("6", rect, Qt::white);
            }
            else if (TileDefs::isConveyor(tile)) {
                QRectF rect(
                    x + 6,
                    y + 6,
                    TILE_SIZE - 12,
                    TILE_SIZE - 12
                    );

                addRect(
                    rect,
                    QPen(QColor("#55efc4")),
                    QBrush(QColor("#00b894"))
                    );

                addCenteredTextInRect("7", rect, Qt::white);
            }
            else if (TileDefs::isTrampoline(tile)) {
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
            else if (TileDefs::isData(tile)) {
                QRectF rect(
                    x + 8,
                    y + 8,
                    TILE_SIZE - 16,
                    TILE_SIZE - 16
                    );

                QGraphicsEllipseItem *fragmentItem = addEllipse(
                    rect,
                    QPen(QColor("#ffffff")),
                    QBrush(QColor("#00f5d4"))
                    );

                fragmentItem->setZValue(5);

                QGraphicsSimpleTextItem *dataText = addSimpleText("8");
                dataText->setBrush(Qt::black);
                dataText->setZValue(6);

                // 关键修改：
                // 把文字设置为圆点的子项。
                // 之后删除 fragmentItem 时，文字也会一起被删除。
                dataText->setParentItem(fragmentItem);

                QRectF textRect = dataText->boundingRect();

                dataText->setPos(
                    rect.center().x() - textRect.width() / 2,
                    rect.center().y() - textRect.height() / 2
                    );

                QString key = gridKey(QPoint(col, row));

                // 关键修改：
                // 这里保存圆点本体，而不是只保存文字。
                dataFragmentItems.insert(key, fragmentItem);
            }
        }
    }

    createBallAtStart();

    // 阶段 20：编辑模式下绘制候选编辑点提示
    drawCandidateEditPoints();

    createStatusText();


    qDebug() << "Stage 12 map loaded.";
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

    QPen ballPen(QColor("#80f7ff"));
    ballPen.setWidth(2);

    QBrush ballBrush(QColor("#8a5cff"));

    ball.item = addEllipse(
        ball.position.x() - ball.radius,
        ball.position.y() - ball.radius,
        ball.radius * 2,
        ball.radius * 2,
        ballPen,
        ballBrush
        );

    ball.item->setZValue(10);
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
        stateText = "候选点编辑：点击切换";
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

    if (levelManager.isValidLevelIndex(currentLevelIndex)) {
        Level currentLevel = levelManager.levelAt(currentLevelIndex);
        levelName = currentLevel.name;
        if (currentLevel.isCustomLevel) {
            levelName = "[自定义] " + levelName;
        }
        targetCount = currentLevel.targetReverseCount;

    }

    QString levelText = QString("关卡：%1/%2  %3")
                            .arg(currentLevelIndex + 1)
                            .arg(levelManager.levelCount())
                            .arg(levelName);

    QString gravityText = QString("重力：%1")
                              .arg(gravityDirectionToString());

    QString timeText = QString("时间：%1s")
                           .arg(elapsedTimeText());

    QString reverseText = QString("反转：%1 / 目标：%2   碎片：%3/%4")
                              .arg(reverseCount)
                              .arg(targetCount)
                              .arg(collectedDataFragmentCount)
                              .arg(totalDataFragmentCount);

    QString deathText = QString("死亡：%1")
                            .arg(deathCount);

    QString fullStateText = QString("状态：%1")
                                .arg(stateText);

    if (statusText != nullptr) {
        statusText->setText(
            QString("%1   %2   %3   %4   %5   %6")
                .arg(levelText)
                .arg(gravityText)
                .arg(timeText)
                .arg(reverseText)
                .arg(deathText)
                .arg(fullStateText)
            );
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

    moveBallOneStep();

    // 如果小球沿着上/下墙滚动，滚出支撑墙后，立刻按当前重力方向坠落。
    applyGravityAfterLeavingWall();

    applyTileEffects();

    checkCurrentTile();

    if (!gameEnded) {
        updateStatusText();
    }
}
void GameScene::moveBallOneStep()
{
    QPointF newPosition = ball.position;

    const bool isDiagonalAirMove = (velocity.x() != 0 && velocity.y() != 0);

    // 先尝试 x 方向移动。
    //
    // 普通沿墙滚动时，如果横向被平台侧边挡住，会尝试脱困。
    // 但蹦床 45° 斜向弹出的球属于空中斜向运动：
    // 一旦碰到任何墙面，就应该直接停下，不能再沿角落脱困或吸附。
    if (velocity.x() != 0) {
        QPointF tryXPosition(
            ball.position.x() + velocity.x(),
            ball.position.y()
            );

        if (canBallMoveTo(tryXPosition)) {
            newPosition.setX(tryXPosition.x());
        } else {
            if (isDiagonalAirMove) {
                velocity = QPointF(0, 0);
                ball.setPosition(newPosition);
                return;
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

    // 再尝试 y 方向移动。
    //
    // 蹦床 45° 斜向弹出的球如果碰到上方墙/下方墙，也直接停下。
    if (velocity.y() != 0) {
        QPointF tryYPosition(
            newPosition.x(),
            newPosition.y() + velocity.y()
            );

        if (canBallMoveTo(tryYPosition)) {
            newPosition.setY(tryYPosition.y());
        } else {
            if (isDiagonalAirMove) {
                velocity = QPointF(0, 0);
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

void GameScene::keyPressEvent(QKeyEvent *event)
{
    if (isEditMode) {
        event->accept();
        return;
    }
    switch (event->key()) {
    case Qt::Key_Up:
        setGravityDirection(GravityDirection::Up);
        break;

    case Qt::Key_Down:
        setGravityDirection(GravityDirection::Down);
        break;

    case Qt::Key_Left:
        setGravityDirection(GravityDirection::Left);
        break;

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

    event->accept();
}


void GameScene::setGravityDirection(GravityDirection newDirection)
{
    if (ball.item == nullptr || gameEnded || isPaused) {
        return;
    }

    const bool touchingAbove = isTouchingWallAbove();
    const bool touchingBelow = isTouchingWallBelow();

    // 现在只把“上方墙 / 下方墙”当作支撑面。
    // 左右墙不再作为支撑面，避免小球吸在左墙或右墙上。
    if (!touchingAbove && !touchingBelow) {
        qDebug() << "Gravity change denied: ball has no upper/lower wall support.";
        updateStatusText();
        return;
    }

    GravityDirection nextGravityDirection = gravityDirection;
    QPointF nextVelocity(0, 0);
    bool allowed = false;

    // 贴着下方墙：可以向左/右滚，也可以向上离开；不能继续向下顶墙。
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

    // 贴着上方墙：可以向左/右滚，也可以向下离开；不能继续向上顶墙。
    // 如果上下都贴墙，优先使用能实际移动的方向。
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

    // 如果新方向下一步会直接撞墙，也不执行。
    if (!canBallMoveTo(ball.position + nextVelocity)) {
        qDebug() << "Gravity change denied: next movement is blocked by wall.";
        updateStatusText();
        return;
    }

    bool changed = (gravityDirection != nextGravityDirection || velocity != nextVelocity);

    gravityDirection = nextGravityDirection;
    velocity = nextVelocity;

    if (changed) {
        reverseCount++;
    }

    updateStatusText();

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
    return currentLevelIndex + 1;
}

int GameScene::totalLevelCount() const
{
    return levelManager.levelCount();
}

QString GameScene::levelNameByNumber(int levelNumber) const
{
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
    return TileDefs::isWall(tileAtScenePos(scenePos));
}

bool GameScene::canBallMoveTo(const QPointF &nextPosition) const
{
    int r = collisionRadius();

    QPointF leftPoint(
        nextPosition.x() - r,
        nextPosition.y()
        );

    QPointF rightPoint(
        nextPosition.x() + r,
        nextPosition.y()
        );

    QPointF topPoint(
        nextPosition.x(),
        nextPosition.y() - r
        );

    QPointF bottomPoint(
        nextPosition.x(),
        nextPosition.y() + r
        );

    if (isWallAt(leftPoint)) {
        return false;
    }

    if (isWallAt(rightPoint)) {
        return false;
    }

    if (isWallAt(topPoint)) {
        return false;
    }

    if (isWallAt(bottomPoint)) {
        return false;
    }

    return true;
}

int GameScene::collisionRadius() const
{
    // 视觉半径是 BALL_RADIUS，碰撞半径略小一点。
    // 这样视觉上仍然是圆球，但边缘和内凹角不会因为一点点视觉重叠就卡死。
    int r = ball.radius - 5;

    if (r < 1) {
        r = 1;
    }

    return r;
}


bool GameScene::isTouchingWallAbove() const
{
    if (ball.item == nullptr) {
        return false;
    }

    // 这是“宽投影支撑检测”：只要碰撞圆的水平投影还有一部分贴着上方墙，
    // 就认为它还在沿上方支撑面滚动。
    //
    // 注意：这个函数可能会把平台侧边也算成支撑。
    // 所以后面还会用 hasDirectSupportAbove() 判断球心正上方是否真的有支撑。
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

    // 这是“宽投影支撑检测”：只要碰撞圆的水平投影还有一部分压着下方墙，
    // 就认为它还在沿下方支撑面滚动。
    //
    // 注意：这个函数可能会把平台侧边也算成支撑。
    // 所以后面还会用 hasDirectSupportBelow() 判断球心正下方是否真的有支撑。
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

    const double r = collisionRadius();
    const double probeY = ball.position.y() - r - BALL_SPEED - 2.0;

    // 只看球心正上方。
    // 如果宽投影认为有支撑，但正上方没有支撑，
    // 多半是平台侧边/内凹角误判，需要进入脱困逻辑。
    return isWallAt(QPointF(ball.position.x(), probeY));
}

bool GameScene::hasDirectSupportBelow() const
{
    if (ball.item == nullptr) {
        return false;
    }

    const double r = collisionRadius();
    const double probeY = ball.position.y() + r + BALL_SPEED + 2.0;

    // 只看球心正下方。
    return isWallAt(QPointF(ball.position.x(), probeY));
}


bool GameScene::isTouchingWallLeft() const
{
    if (ball.item == nullptr) {
        return false;
    }

    const double probeDistance = ball.radius + BALL_SPEED + 2.0;

    return isWallAt(QPointF(
        ball.position.x() - probeDistance,
        ball.position.y()
        ));
}

bool GameScene::isTouchingWallRight() const
{
    if (ball.item == nullptr) {
        return false;
    }

    const double probeDistance = ball.radius + BALL_SPEED + 2.0;

    return isWallAt(QPointF(
        ball.position.x() + probeDistance,
        ball.position.y()
        ));
}

bool GameScene::hasAnyWallContact() const
{
    // 只有上方墙 / 下方墙算作可操作支撑面。
    // 左右墙不再算支撑面，避免小球吸在左右墙上。
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

        return canBallMoveTo(ball.position + testVelocity);
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

        return canBallMoveTo(ball.position + testVelocity);
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

    // 最多允许探出一个格子。
    // 不是瞬移过关，而是为了让球从内凹角的侧边碰撞里脱出来。
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

    // 蹦床 45° 斜向弹出后，速度同时具有 x/y 分量。
    // 这种属于空中斜向运动，不应该被“离开支撑后恢复竖直坠落”的逻辑改回纯竖直。
    // 空中仍然不能改方向，碰墙仍然会在 moveBallOneStep() 里停下。
    if (velocity.x() != 0 && velocity.y() != 0) {
        return;
    }

    // 宽投影函数负责“还沿着平台滚动”的体验；
    // 直接支撑函数负责判断“球心正上/正下是否真的有支撑”。
    //
    // 如果宽投影说有支撑，但直接支撑没有，
    // 多半是内凹角的平台侧边误判。此时不能继续卡着，要尝试按当前重力脱困坠落。
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
        // 保险处理：旧版本可能留下 Left/Right 状态。
        // 左右墙不再提供吸附，统一恢复成向下坠落。
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

    QWidget *parentWidget = nullptr;

    if (!views().isEmpty()) {
        parentWidget = views().first();
    }

    Level currentLevel = levelManager.levelAt(currentLevelIndex);

    int stars = calculateStars();

    QString message = QString("通关成功！\n\n关卡：%1\n用时：%2 秒\n反转次数：%3\n数据碎片：%4 / %5\n星级：%6")
                          .arg(currentLevel.name)
                          .arg(elapsedTimeText())
                          .arg(reverseCount)
                          .arg(collectedDataFragmentCount)
                          .arg(totalDataFragmentCount)
                          .arg(starText(stars));

    if (levelManager.isValidLevelIndex(currentLevelIndex + 1)) {
        QMessageBox::StandardButton result = QMessageBox::question(
            parentWidget,
            "通关成功",
            message + "\n\n是否进入下一关？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
            );

        if (result == QMessageBox::Yes) {
            nextLevel();
        } else {
            setFocus();
        }
    } else {
        QMessageBox::information(
            parentWidget,
            "全部通关",
            message + "\n\n你已经完成所有关卡！"
            );

        setFocus();
    }
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

    // 速度类机关：缓冲区、以后可以加加速区
    applySpeedEffect(currentTile);

    // 方向类机关：弹射块、蹦床、以后可以加反向区、一次性弹射块
    applyBounceEffect(currentTile);
    applyTrampolineEffect(currentTile);

    // 位置类机关：传送带、以后可以加传送门
    applyConveyorEffect(currentTile);
}
void GameScene::applySpeedEffect(QChar currentTile)
{
    // 缓冲区：速度变慢，离开后恢复正常速度
    if (TileDefs::isSlow(currentTile)) {
        adjustVelocityToSpeed(SLOW_SPEED);
    } else {
        adjustVelocityToSpeed(BALL_SPEED);
    }

    // 以后如果加速区，可以写在这里
    // 例如：
    // if (TileDefs::isSpeedUp(currentTile)) {
    //     adjustVelocityToSpeed(FAST_SPEED);
    // }
}
void GameScene::applyBounceEffect(QChar currentTile)
{
    // 弹射块：强制向上运动
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

    // 同一个蹦床格子只触发一次，避免球还在格子中时每帧重复弹跳。
    if (wasOnTrampoline) {
        return;
    }

    wasOnTrampoline = true;

    // 蹦床只处理竖直方向进入的球。
    // 水平滚过蹦床不触发弹跳。
    if (velocity.y() == 0) {
        return;
    }

    moveSpeed = BALL_SPEED;

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

    qDebug() << "Directional trampoline triggered."
             << TileDefs::nameOf(currentTile)
             << "Arrow:" << TileDefs::trampolineArrow(currentTile)
             << "Gravity:" << gravityDirectionToString()
             << "Velocity:" << velocity;
}

void GameScene::applyConveyorEffect(QChar currentTile)
{
    // 传送带：额外向右移动
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

    if (!levelManager.isValidLevelIndex(currentLevelIndex)) {
        QMessageBox::warning(
            parentWidget,
            "保存失败",
            "当前关卡编号无效，不能保存。"
            );

        setFocus();
        return;
    }

    Level currentLevel = levelManager.levelAt(currentLevelIndex);

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

    // 保存“编辑后的地图”，而不是运行中被收集碎片后改变过的临时地图。
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

    // 保存后立刻重新读取一次，验证文件可读，并且内容和保存前一致。
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

    // 加入当前 LevelManager，方便不重启程序也能继续切到这个自定义关卡。
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

    elapsedMs = 0;
    gameEnded = false;
    wasOnTrampoline = false;
}

void GameScene::selectBounceBlock()
{
    selectEditTile(TileDefs::Bounce);
}

void GameScene::selectSlowBlock()
{
    selectEditTile(TileDefs::Slow);
}

void GameScene::selectConveyorBlock()
{
    selectEditTile(TileDefs::Conveyor);
}

void GameScene::selectTrampolineBlock()
{
    selectEditTile(TileDefs::Trampoline);
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

    // 阶段 20：只能编辑候选点
    if (!isCandidateEditPoint(gridPos)) {
        qDebug() << "Clicked non-candidate point:" << gridPos;
        event->accept();
        return;
    }

    QChar currentTile = tileAtGridPos(gridPos);

    if (event->button() == Qt::LeftButton) {
        QChar nextTile = nextCandidateTile(currentTile);

        setTileAtGridPos(gridPos, nextTile);

        redrawEditedMap();

        qDebug() << "Candidate point switched:"
                 << gridPos
                 << TileDefs::nameOf(currentTile)
                 << "->"
                 << TileDefs::nameOf(nextTile);
    }
    else if (event->button() == Qt::RightButton) {
        // 右键直接清空，方便演示
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
    return TileDefs::isBounce(tile)
           || TileDefs::isSlow(tile)
           || TileDefs::isConveyor(tile)
           || TileDefs::isTrampoline(tile);
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
    if (TileDefs::isEmpty(currentTile)) {
        return TileDefs::Bounce;
    }

    if (TileDefs::isBounce(currentTile)) {
        return TileDefs::Slow;
    }

    if (TileDefs::isSlow(currentTile)) {
        return TileDefs::Conveyor;
    }

    if (TileDefs::isConveyor(currentTile)) {
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
        return TileDefs::Empty;
    }

    // 保险：如果候选点上出现了别的东西，先变成弹射块。
    return TileDefs::Bounce;
}

void GameScene::drawCandidateEditPoints()
{
    if (!isEditMode) {
        return;
    }

    QPen candidatePen(QColor("#80f7ff"));
    candidatePen.setWidth(2);
    candidatePen.setStyle(Qt::DashLine);

    QBrush candidateBrush(QColor(128, 247, 255, 45));

    QFont hintFont("Microsoft YaHei", 8, QFont::Bold);

    for (const QPoint &gridPos : candidateEditPoints) {
        if (!isGridPosInMap(gridPos)) {
            continue;
        }

        int x = gridPos.x() * TILE_SIZE;
        int y = gridPos.y() * TILE_SIZE;

        QRectF rect(
            x + 4,
            y + 4,
            TILE_SIZE - 8,
            TILE_SIZE - 8
            );

        QGraphicsRectItem *candidateRect = addRect(
            rect,
            candidatePen,
            candidateBrush
            );

        candidateRect->setZValue(12);

        QGraphicsSimpleTextItem *hintText = addSimpleText("可改", hintFont);
        hintText->setBrush(QColor("#80f7ff"));
        hintText->setZValue(13);

        QRectF textRect = hintText->boundingRect();

        hintText->setPos(
            rect.center().x() - textRect.width() / 2,
            rect.center().y() - textRect.height() / 2
            );
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

    updateStatusText();

    qDebug() << "Data fragment collected:"
             << collectedDataFragmentCount
             << "/"
             << totalDataFragmentCount;
}

int GameScene::calculateStars() const
{
    int stars = 1;

    if (levelManager.isValidLevelIndex(currentLevelIndex)) {
        Level currentLevel = levelManager.levelAt(currentLevelIndex);

        if (reverseCount <= currentLevel.targetReverseCount) {
            stars++;
        }
    }

    if (totalDataFragmentCount > 0 &&
        collectedDataFragmentCount >= totalDataFragmentCount) {
        stars++;
    }

    return stars;
}
QString GameScene::defaultCustomLevelName() const
{
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