#ifndef FACTSETTING_H
#define FACTSETTING_H

#include <QWidget>
#include "msmkeyboard.h"
#include "commonSetting.h"
#include <QLabel>

namespace Ui {
class FactSetting;
}

class FactSetting : public settingsManager
{
    Q_OBJECT

public:
    explicit FactSetting(QWidget *parent = nullptr);
    void runFleetProvisioningWrapper() {runFleetProvisionerAsync();}
    void checkCertificatesWrapper() {checkCertificates();}
    void updateInfo(const DeviceInfo &f);
    ~FactSetting();

signals:
    void rebootMCM();
    void usbExport();
    void factoryExit();
    void runFleetProvisioning();
    void OTA_status(const QJsonObject &otaData);

protected:
    QString mqttConfigPath() const override { return "config/mqtt_settings.json"; }
    QString sftpConfigPath() const override { return "config/sftp_settings.json"; }
    QString mqttsConfigPath() const override { return "config/aws_mqtts_settings.json"; }
    QString configJsonPath() const override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void on_mqtt_load_btn_clicked(bool msgBox = false);
    void on_sftp_load_btn_clicked(bool msgBox = false);
    void on_mqtts_load_btn_clicked(bool msgBox = false);
    void load_mqtt_settings(bool msgBox = false, bool reconnect = false);
    void load_sftp_settings(bool msgBox = false, bool reconnect = false);
    void load_mqtts_settings(bool msgBox = false, bool reconnect = false);
    void sendOTAData(int status);
    void startOTA(const QString &selectedVersion);
    void send_aws_updated_data() override;

private slots:
    void on_next_btn_clicked();
    void on_fact_back_btn_clicked();
    void on_exp_btn_clicked();
    void on_exit_btn_clicked();
    void on_reboot_btn_clicked();
    void on_mqtt_save_btn_clicked();
    void on_sftp_save_btn_clicked();
    void on_OTA_pushButton_clicked();
    void on_pushButton_5_clicked();
    void on_mqtts_save_btn_clicked(bool msgBox = false);
    void on_fetch_claim_btn_clicked();
    void on_provision_btn_clicked();

private:
    Ui::FactSetting *ui;
    MSMKeyboard *keyboard;
    QLabel *updatingLabel = nullptr;
};

#endif // FACTSETTING_H
