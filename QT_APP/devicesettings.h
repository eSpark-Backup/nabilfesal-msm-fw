#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include <QWidget>
#include "msmkeyboard.h"
#include "awsmqttmanager.h"
#include "sftpManager.h"

namespace Ui {
class DeviceSettings;
}

class DeviceSettings : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceSettings(QWidget *parent = nullptr);
    void load_and_start_mqtt_data();
    void load_and_start_sftp_data();
    ~DeviceSettings();

signals:
    void channels_view();
    void usbExport();
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
    void on_export_btn_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void parseNtpJson(const QJsonObject& data);
    void refreshInfo();
    Ui::DeviceSettings *ui;
    MSMKeyboard *keyboard;
    AwsMqttManager *mqttHandler = nullptr;
    sftpManager *sftpHandler = nullptr;
};

#endif // DEVICESETTINGS_H
