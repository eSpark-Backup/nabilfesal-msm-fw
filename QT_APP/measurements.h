#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <QWidget>
#include "numkeyboard.h"
#include <QLabel>

namespace Ui {
class Measurements;
}

class Measurements : public QWidget
{
    Q_OBJECT

public:
    explicit Measurements(QWidget *parent = nullptr);
    ~Measurements();
signals:
    void measureExit();
    void requestSerialCommand(const QString &cmd);
private slots:
    void on_back_1_clicked();

    void on_back_2_clicked();

    void on_pushButton_13_clicked();

    void on_pushButton_14_clicked();

    void on_reboot_btn_clicked();

    void on_ration_btn_clicked();

    void on_i_off_btn_clicked();

    void on_v_off_btn_clicked();

    void on_cal_btn_clicked();

    void on_load_btn_clicked();

    void on_save_btn_clicked();

    void on_toolButton_clicked();

    void on_toolButton_2_clicked();

    void on_toolButton_3_clicked();

    void on_toolButton_4_clicked();

    void on_toolButton_5_clicked();

    void on_toolButton_6_clicked();

    void on_toolButton_7_clicked();

    void on_toolButton_8_clicked();

    void on_toolButton_9_clicked();

    void on_toolButton_10_clicked();

    void on_toolButton_11_clicked();

    void on_toolButton_12_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    Ui::Measurements *ui;
    bool isRatioActivated = false;
    bool isVOffsetActivated = false;
    bool isIOffsetActivated = false;
    NumKeyboard *keyboard;
    QList<QLabel*> signEdits;
    QList<QLineEdit*> edits;
};

#endif // MEASUREMENTS_H
