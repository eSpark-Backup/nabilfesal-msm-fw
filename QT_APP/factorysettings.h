#ifndef FACTORYSETTINGS_H
#define FACTORYSETTINGS_H

#include <QWidget>
#include "measurements.h"

namespace Ui {
class FactorySettings;
}

class FactorySettings : public QWidget
{
    Q_OBJECT

public:
    explicit FactorySettings(QString hardwareVersion, QString softwareVersion, QWidget *parent = nullptr);
    ~FactorySettings();

signals:
    void factoryExit();
    void loadMeasurements();
    void loadFactSettings();
    void ctRatio();
    void i_offset();
    void v_offset();
    void calibrate(bool current);

private slots:
    void on_pushButton_clicked();

    // void on_measurement_clicked();

    void on_system_clicked();

    void on_v_cal_btn_clicked();

    void on_ration_btn_clicked();

    void on_v_off_btn_clicked();

    void on_i_off_btn_clicked();

    void on_i_cal_btn_clicked();

    void on_reboot_btn_clicked();

private:
    Ui::FactorySettings *ui;
    Measurements *measure = nullptr;
};

#endif // FACTORYSETTINGS_H
