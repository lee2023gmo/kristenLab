#ifndef LEVELEDITORDIALOG_H
#define LEVELEDITORDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class LevelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LevelEditorDialog(QWidget *parent = nullptr);

private:
    QLineEdit *nameEdit;
    QSpinBox *widthSpinBox;
    QSpinBox *heightSpinBox;
    QSpinBox *targetReverseSpinBox;

    QLabel *mapPlaceholderLabel;

    QPushButton *generateButton;
    QPushButton *validateButton;
    QPushButton *saveButton;
    QPushButton *closeButton;

    void setupUi();
    void setupConnections();
    void showStageTip(const QString &actionName);
};

#endif // LEVELEDITORDIALOG_H
