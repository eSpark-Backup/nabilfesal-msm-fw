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
    void parse_and_save_response(const QJsonObject &response);
    void enableButton();
    ~CalibrationDialog();

signals:
    void exitScreen();
    void requestSerialCommand(const QString &cmd);

private slots:
    void on_sendpushBtn_clicked();
    void on_backPushBtn_clicked();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void setupTouchSpin();

private:
    Ui::CalibrationDialog *ui;
    NumKeyboard *keyboard;

    bool isCurrentData;
    const int minValue = 1;
    const int maxValue = 12;
    int value = 1;
};

#endif // CALIBRATIONDIALOG_H
