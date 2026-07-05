#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <QGraphicsScene>
#include <QStringList>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QMap>
#include <QColor>
#include <QRectF>
#include <QSet>
#include <QSet>
#include <QVector>


#include "ball.h"
#include "levelmanager.h"

class QTimer;
class QKeyEvent;
class QGraphicsSimpleTextItem;
class QGraphicsItem;
class QGraphicsSceneMouseEvent;
class QMovie;

enum class GravityDirection {
    Up,
    Down,
    Left,
    Right
};

class GameScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GameScene(QObject *parent = nullptr);

    void restartLevel();
    void togglePause();

    void nextLevel();
    void previousLevel();

    int currentLevelNumber() const;
    int totalLevelCount() const;
    QString levelNameByNumber(int levelNumber) const;

    void refreshStatus();
    void loadLevelByNumber(int levelNumber);
    void loadTemporaryLevelForTest(const Level &level);

    void enterEditMode();
    void startRunMode();

    void selectSlowBlock();
    void selectTrampolineBlock();
    void selectTrampolineUpRightBlock();
    void selectTrampolineUpLeftBlock();
    void selectTrampolineDownRightBlock();
    void selectTrampolineDownLeftBlock();
    void selectTrampolineRightBlock();
    void selectTrampolineLeftBlock();
    void selectLaserBlock();
    void saveCurrentEditedLevel();

signals:
    void statusChanged(const QString &levelText,
                       const QString &gravityText,
                       const QString &timeText,
                       const QString &reverseText,
                       const QString &deathText,
                       const QString &stateText);
    void inputDirectionChanged(const QString &activeKey);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
private:
    bool isEditMode;
    QChar selectedEditTile;

    QStringList editMapData;
    QSet<QString> playerPlacedMechanismKeys;
    LevelManager levelManager;
    bool isTemporaryTestLevel;
    Level temporaryTestLevel;
    int currentLevelIndex;

    QStringList mapData;

    QPoint startGridPos;
    QPoint endGridPos;

    Ball ball;
    QMovie *ballMovie;

    QTimer *timer;
    GravityDirection gravityDirection;
    QPointF velocity;
    int moveSpeed;

    int reverseCount;
    int deathCount;

    int collectedDataFragmentCount;
    int totalDataFragmentCount;

    int collectedKeyCount;
    int totalKeyCount;

    bool isPaused;
    bool gameEnded;
    bool wasOnTrampoline;
    bool isTrampolineLaunchMove;

    int elapsedMs;

    QGraphicsSimpleTextItem *statusText;
    QMap<QString, QGraphicsItem *> dataFragmentItems;
    QMap<QString, QGraphicsItem *> keyItems;
    QMap<QString, QGraphicsItem *> doorItems;
    QMap<QString, QGraphicsItem *> doorLabelItems;
    QMap<QString, QPoint> portalPairTargets;
    QString teleportLockPortalKey;
    QMap<QString, QGraphicsItem *> laserItems;
    QMap<QString, QGraphicsItem *> laserLabelItems;
    QPointF lastNonLaserBallPosition;

    QString currentBallMoviePath;
    void updateBallMovie();
    QString resolveBallMoviePath() const;

    void loadLevel(int levelIndex);
    void applyLevelData(const Level &currentLevel);
    bool isLevelUnlocked(int levelIndex) const;

    void drawMap();
    void drawGridBackground(int rows, int cols);

    void createBallAtStart();
    void createStatusText();
    void updateStatusText();

    QPointF gridCenterToScenePos(const QPoint &gridPos) const;

    void updateGame();

    void moveBallOneStep();

    QString indicatorKeyForEvent(int key) const;
    void setGravityDirection(GravityDirection newDirection);
    QString gravityDirectionToString() const;

    QChar tileAtScenePos(const QPointF &scenePos) const;
    bool isWallAt(const QPointF &scenePos) const;
    bool isDoorOpen() const;
    bool isClosedDoorAt(const QPointF &scenePos) const;
    bool isPortalAt(const QPointF &scenePos) const;
    bool isActiveLaserAt(const QPointF &scenePos) const;
    bool isBlockingAt(const QPointF &scenePos) const;
    bool canBallMoveTo(const QPointF &nextPosition) const;
    bool canBallMoveToForVelocity(const QPointF &nextPosition,
                                  const QPointF &movement) const;
    bool canBallMoveToWithSupportAllowance(const QPointF &nextPosition,
                                           bool ignoreLeft,
                                           bool ignoreRight,
                                           bool ignoreAbove,
                                           bool ignoreBelow) const;
    int collisionRadius() const;

    // 修改：贴墙后才允许改变重力方向；撞墙后不反弹
    bool isTouchingWallAbove() const;
    bool isTouchingWallBelow() const;
    bool hasDirectSupportAbove() const;
    bool hasDirectSupportBelow() const;
    bool isTouchingWallLeft() const;
    bool isTouchingWallRight() const;
    bool hasAnyWallContact() const;
    QPointF velocityForGravityDirection(GravityDirection direction) const;
    bool isGravityChangeAllowed(GravityDirection newDirection) const;
    bool tryEscapeCornerAndFall(const QPointF &basePosition,
                                const QPointF &fallVelocity,
                                QPointF *escapedPosition) const;
    void applyGravityAfterLeavingWall();

    bool isLaserActive() const;
    void updateLaserItems();
    void updateDoorItems();
    bool wouldCollideWithActiveLaser(const QPointF &nextPosition) const;
    void repelFromLaserCollision(const QPointF &blockedMovement);
    void rememberLastNonLaserPosition();

    void resetVelocityByGravity();

    void applyTileEffects();

    void applySpeedEffect(QChar currentTile);
    void applyBounceEffect(QChar currentTile);
    void applyTrampolineEffect(QChar currentTile);
    void applyConveyorEffect(QChar currentTile);
    void applyPortalEffect(QChar currentTile);
    void rebuildPortalPairs();
    bool tryTeleportAtCurrentPosition();

    void adjustVelocityToSpeed(int newSpeed);


    enum class CompletionAction {
        Stay,
        Restart,
        Next
    };

    void checkCurrentTile();
    void handleFailure();
    void handleVictory();
    CompletionAction showCompletionDialog(const Level &currentLevel, int stars);
    QString elapsedTimeText() const;

    int countDataFragments() const;
    int countKeys() const;
    QPoint gridPosAtScenePos(const QPointF &scenePos) const;
    QString gridKey(const QPoint &gridPos) const;
    void collectDataFragmentAtCurrentPosition();
    void collectKeyAtCurrentPosition();
    int calculateStars() const;
    QString starText(int stars) const;

    void addCenteredTextInRect(const QString &text, const QRectF &rect, const QColor &color);
    void resetRuntimeStateForCurrentMap();

    void selectEditTile(QChar tile);
    QString selectedEditTileName() const;

    bool isGridPosInMap(const QPoint &gridPos) const;
    bool isEditableMechanism(QChar tile) const;

    QChar tileAtGridPos(const QPoint &gridPos) const;
    void setTileAtGridPos(const QPoint &gridPos, QChar tile);

    void redrawEditedMap();
    QVector<QPoint> candidateEditPoints;
    QSet<QString> candidateEditPointKeys;
    void rebuildCandidateEditPointKeys();
    QVector<QPoint> fallbackCandidateEditPoints(int maxCount) const;
    bool isCandidateEditPoint(const QPoint &gridPos) const;
    QChar nextCandidateTile(QChar currentTile) const;
    void drawCandidateEditPoints();
    QString defaultCustomLevelName() const;
    QString makeSafeFileBaseName(const QString &text) const;
    QString createCustomLevelFilePath(const QString &levelName) const;
};

#endif // GAMESCENE_H