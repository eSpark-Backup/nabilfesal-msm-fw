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
    void checkCertificates();
    void runFleetProvisionerAsync();
    ~FactSetting();

signals:
    void usbExport();
    void factoryExit();
    void aws_mqtts_load();
    void runFleetProvisioning();
    void OTA_status(const QJsonObject &otaData);

protected:
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
    void on_pushButton_3_clicked();
    void on_pushButton_2_clicked();

private:
    Ui::FactSetting *ui;
    MSMKeyboard *keyboard;
    QLabel *updatingLabel = nullptr;
};

#endif // FACTSETTING_H
