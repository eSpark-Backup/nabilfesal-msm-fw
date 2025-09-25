#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include <QWidget>
#include "msmkeyboard.h"
#include "commonSetting.h"
#include "awsmqttmanager.h"

namespace Ui {
class DeviceSettings;
}

class DeviceSettings : public settingsManager
{
    Q_OBJECT

public:
    explicit DeviceSettings(QWidget *parent = nullptr);
    void loadAwsMqttsSettings(bool showMessages = true);
    ~DeviceSettings();
    DeviceInfo curDevInfo, prvDevInfo;

signals:
    void channels_view();
    void usbExport();
    void aws_mqtts_load();
    void deviceInfoChanged(const DeviceInfo &info);
    void ntpSettingsChanged(const NtpSettings &settings);

public slots:
    void on_mqtt_load_btn_clicked(bool msgBox = true);
    void on_sftp_load_btn_clicked(bool msgBox = true);

private slots:
    void on_channels_btn_clicked();
    void on_continue_btn_clicked();
    void on_back_btn_clicked();
    void on_ntp_load_btn_clicked();
    void on_ntp_save_btn_clicked();
    void on_mqtt_save_btn_clicked();
    void on_sftp_save_btn_clicked();
    void on_export_btn_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void parseNtpJson(const QJsonObject& data);
    bool isDifferent(const DeviceInfo &a, const DeviceInfo &b);
    void refreshInfo();
    Ui::DeviceSettings *ui;
    MSMKeyboard *keyboard;
    NtpSettings currentNtp, previousNtp;
};

#endif // DEVICESETTINGS_H
