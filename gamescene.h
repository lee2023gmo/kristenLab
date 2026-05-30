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

    void enterEditMode();
    void startRunMode();

    void selectBounceBlock();
    void selectSlowBlock();
    void selectConveyorBlock();
    void saveCurrentEditedLevel();

signals:
    void statusChanged(const QString &levelText,
                       const QString &gravityText,
                       const QString &timeText,
                       const QString &reverseText,
                       const QString &deathText,
                       const QString &stateText);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
private:
    bool isEditMode;
    QChar selectedEditTile;

    QStringList editMapData;
    QSet<QString> playerPlacedMechanismKeys;
    LevelManager levelManager;
    int currentLevelIndex;

    QStringList mapData;

    QPoint startGridPos;
    QPoint endGridPos;

    Ball ball;

    QTimer *timer;
    GravityDirection gravityDirection;
    QPointF velocity;
    int moveSpeed;

    int reverseCount;
    int deathCount;

    int collectedDataFragmentCount;
    int totalDataFragmentCount;

    bool isPaused;
    bool gameEnded;

    int elapsedMs;

    QGraphicsSimpleTextItem *statusText;
    QMap<QString, QGraphicsItem *> dataFragmentItems;

    void loadLevel(int levelIndex);

    void drawMap();
    void drawGridBackground(int rows, int cols);

    void createBallAtStart();
    void createStatusText();
    void updateStatusText();

    QPointF gridCenterToScenePos(const QPoint &gridPos) const;

    void updateGame();

    // 阶段 18：把移动逻辑从 updateGame 中拆出来
    void moveBallOneStep();

    void setGravityDirection(GravityDirection newDirection);
    QString gravityDirectionToString() const;

    QChar tileAtScenePos(const QPointF &scenePos) const;
    bool isWallAt(const QPointF &scenePos) const;
    bool canBallMoveTo(const QPointF &nextPosition) const;
    void resetVelocityByGravity();

    void applyTileEffects();

    void applySpeedEffect(QChar currentTile);
    void applyBounceEffect(QChar currentTile);
    void applyConveyorEffect(QChar currentTile);

    void adjustVelocityToSpeed(int newSpeed);


    void checkCurrentTile();
    void handleFailure();
    void handleVictory();
    QString elapsedTimeText() const;

    int countDataFragments() const;
    QPoint gridPosAtScenePos(const QPointF &scenePos) const;
    QString gridKey(const QPoint &gridPos) const;
    void collectDataFragmentAtCurrentPosition();
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
    // 阶段 20：固定候选点编辑模式
    QVector<QPoint> candidateEditPoints;
    QSet<QString> candidateEditPointKeys;
    // 阶段 20：候选点编辑模式
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