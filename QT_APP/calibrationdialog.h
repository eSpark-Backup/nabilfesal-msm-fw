#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QWidget>
#include "numkeyboard.h"

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QWidget
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    void loadCalibrationScreen(bool isEnable);
    ~CalibrationDialog();

signals:
    void exitScreen();
    void requestSerialCommand(const QString &cmd);

private slots:
    void on_sendpushBtn_clicked();
    void on_backPushBtn_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::CalibrationDialog *ui;
    NumKeyboard *keyboard;

    bool isCurrentData;
};

#endif // CALIBRATIONDIALOG_H
