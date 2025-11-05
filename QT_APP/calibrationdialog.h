#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include "numkeyboard.h"

namespace Ui {
class CalibrationDialog;
}

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    ~CalibrationDialog();

signals:
    void requestSerialCommand(const QString &cmd);

private slots:
    void on_sendpushBtn_clicked();
    void on_backPushBtn_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::CalibrationDialog *ui;
    NumKeyboard *keyboard;
};

#endif // CALIBRATIONDIALOG_H
