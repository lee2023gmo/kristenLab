#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

    QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QGraphicsView;
class GameScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QGraphicsView *gameView;
    GameScene *gameScene;

    QLabel *levelLabel;
    QLabel *timeLabel;
    QLabel *gravityLabel;
    QLabel *reverseLabel;
    QLabel *deathLabel;
    QLabel *stateLabel;
    QLabel *viewZoomLabel;

    double gameViewScale;

    // 主菜单和游戏界面。
    void setupMainMenu();
    void setupGameWindow(int startLevelNumber = 1);
    void showLevelSelectDialog();
    void showLevelEditorDialog();
    void showHelpDialog();
    void clearGameScene();

    void setGameViewScale(double scale);
    void applyGameViewZoom();
    void zoomGameViewIn();
    void zoomGameViewOut();
    void resetGameViewZoom();
    void autoFitGameViewZoom();
};

#endif // MAINWINDOW_H
