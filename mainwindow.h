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
class QEvent;
class QWidget;
class GameScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

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

    QWidget *inputIndicatorWidget;
    QLabel *wKeyLabel;
    QLabel *aKeyLabel;
    QLabel *sKeyLabel;
    QLabel *dKeyLabel;

    double gameViewScale;

    void createInputIndicator();
    void repositionInputIndicator();
    void updateInputIndicator(const QString &activeKey);

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
