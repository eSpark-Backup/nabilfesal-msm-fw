#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include <QWidget>

namespace Ui {
class DeviceSettings;
}

class DeviceSettings : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceSettings(QWidget *parent = nullptr);
    ~DeviceSettings();

signals:
    void channels_view();
private slots:
    void on_channels_btn_clicked();

    void on_continue_btn_clicked();

    void on_back_btn_clicked();

    void on_ntp_load_btn_clicked();

    void on_ntp_save_btn_clicked();

    void on_mqtt_save_btn_clicked();

    void on_mqtt_load_btn_clicked();

    void on_sftp_load_btn_clicked();

    void on_sftp_save_btn_clicked();

private:
    void parseNtpJson(const QJsonObject& data);
    Ui::DeviceSettings *ui;
};

#endif // DEVICESETTINGS_H
