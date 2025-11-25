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
    void refreshInfo();
    ~DeviceSettings();
    DeviceInfo curDevInfo, prvDevInfo;

signals:
    void channels_view();
    void usbExport();
    void aws_mqtts_load();
    void resetEnergy();
    void deviceInfoChanged(const DeviceInfo &info);
    void ntpSettingsChanged(const NtpSettings &settings);
    void mqttsSettingsChanged(const QJsonObject &mqttsSetting);

public slots:
    void on_mqtt_load_btn_clicked(bool msgBox = false);
    void on_sftp_load_btn_clicked(bool msgBox = false);
    void on_mqtts_load_btn_clicked(bool msgBox = false);
    void parse_load_store_ntp(const QJsonObject &payload);
    void parse_load_store_sftp(const QJsonObject &payload);
    void parse_load_store_mqtt(const QJsonObject &payload);
    void parse_load_store_mqtts(const QJsonObject &payload);
    void send_aws_updated_data();
    void load_mqtt_settings(bool msgBox = false, bool reconnect = false);
    void load_mqtts_settings(bool msgBox = false, bool reconnect = false);
    void load_sftp_settings(bool msgBox = false, bool reconnect = false);
    QJsonObject loadMqttsSettings(bool showMessages, bool reloadSettings = true);

private slots:
    void on_channels_btn_clicked();
    void on_continue_btn_clicked();
    void on_dev_back_btn_clicked();
    void on_ntp_load_btn_clicked();
    void on_ntp_save_btn_clicked();
    void on_mqtt_save_btn_clicked();
    void on_sftp_save_btn_clicked();
    void on_export_btn_clicked();
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_mqtts_save_btn_clicked(bool msgBox = false);
    void on_resetEnergy_Btn_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void saveMqttsSettings(QJsonObject mqttsSetting, bool showMessages = true);
    
private:
    bool apply_ntp_Settings(bool msgBox= true);
    void parseNtpJson(const QJsonObject& data);
    void apply_sftp_Settings(const QJsonObject &sftpSetting, bool msgBox = true);
    void apply_mqtt_Settings(const QJsonObject &mqttSetting, bool msgBox = true);
    void apply_mqtts_Settings(const QJsonObject &mqttsSettings, bool msgBox = true);
    bool isDifferent(const DeviceInfo &a, const DeviceInfo &b);
    Ui::DeviceSettings *ui;
    MSMKeyboard *keyboard;
    QString server1, server2, server3;
    NtpSettings newNtp, previousNtp;
    QTimer *refreshTimer = nullptr;
};

#endif // DEVICESETTINGS_H
