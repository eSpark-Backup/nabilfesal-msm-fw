#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <QWidget>

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

private:
    Ui::Measurements *ui;
};

#endif // MEASUREMENTS_H
