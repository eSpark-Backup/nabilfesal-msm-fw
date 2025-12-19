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
    void runFleetProvisioningWrapper() {runFleetProvisionerAsync();}
    void checkCertificatesWrapper() {checkCertificates();}
    void updateInfo(const DeviceInfo &a);
    ~DeviceSettings();

signals:
    void channels_view();
    void usbExport();
    void resetEnergy();
    void runFleetProvisioning();

public slots:
    void on_mqtt_load_btn_clicked(bool msgBox = false);
    void on_sftp_load_btn_clicked(bool msgBox = false);
    void on_mqtts_load_btn_clicked(bool msgBox = false);
    void parse_load_store_ntp(const QJsonObject &payload);
    void load_mqtt_settings(bool msgBox = false, bool reconnect = false);
    void load_mqtts_settings(bool msgBox = false, bool reconnect = false);
    void load_sftp_settings(bool msgBox = false, bool reconnect = false);
    void send_aws_updated_data() override;

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
    void on_fetch_claim_btn_clicked();
    void on_provision_btn_clicked();

protected:
    QString mqttConfigPath() const override { return "config/admin_settings/mqtt_settings.json"; }
    QString sftpConfigPath() const override { return "config/admin_settings/sftp_settings.json"; }
    QString mqttsConfigPath() const override { return "config/admin_settings/aws_mqtts_settings.json"; }
    QString configJsonPath() const override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    
private:
    bool apply_ntp_Settings(bool msgBox= true);
    void parseNtpJson(const QJsonObject& data);
    Ui::DeviceSettings *ui;
    MSMKeyboard *keyboard;
    QString server1, server2, server3;
    NtpSettings newNtp, previousNtp;
};

#endif // DEVICESETTINGS_H
