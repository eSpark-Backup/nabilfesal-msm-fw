#ifndef COMMONSETTING_H
#define COMMONSETTING_H

#include <QWidget>
#include <QLineEdit> 
#include "mqttManager.h"
#include "sftpManager.h"
#include "common_headers.h"
#include "awsmqttmanager.h"
#include <QJsonArray>

class settingsManager : public QWidget
{
    Q_OBJECT

public:
    explicit settingsManager(QWidget *parent = nullptr) {};
    ~settingsManager() {};
   
signals:
    void mqtt_load();
    void sftp_load();
    void aws_mqtts_load();
    void mqttSettingsChanged(const QJsonObject &mqttSetting);
    void sftpSettingsChanged(const QJsonObject &sftpSetting);
    void mqttsSettingsChanged(const QJsonObject &mqttsSetting);
    void ntpSettingsChanged(const NtpSettings &settings);

public slots:
    void parse_load_store_sftp(const QJsonObject &payload);
    void parse_load_store_mqtt(const QJsonObject &payload);
    void parse_load_store_mqtts(const QJsonObject &payload);
    
protected:
    QJsonObject getMqttSettings();
    QJsonObject getSftpSettings();
    QJsonObject getMqttsSettings();
    virtual void send_aws_updated_data() = 0;
    virtual QString mqttConfigPath() const = 0;
    virtual QString sftpConfigPath() const = 0;
    virtual QString mqttsConfigPath() const = 0;
    virtual QString configJsonPath() const = 0;
    virtual bool eventFilter(QObject *obj, QEvent *event) = 0;
    QJsonObject loadMqttSettings(bool showMessages, bool reloadSettings = true);
    QJsonObject loadSftpSettings(bool showMessages, bool reloadSettings = true);
    QJsonObject loadMqttsSettings(bool showMessages, bool reloadSettings = true);
    void apply_mqtt_Settings(const QJsonObject &mqttSetting, bool msgBox = true);
    void apply_sftp_Settings(const QJsonObject &sftpSetting, bool msgBox = true);
    void apply_mqtts_Settings(const QJsonObject &mqttsSettings, bool msgBox = true);
    void saveMqttSettings(QJsonObject mqttSetting, bool showMessages = true);
    void saveSftpSettings(QJsonObject sftpSetting, bool showMessages = true);
    void saveMqttsSettings(QJsonObject mqttsSetting, bool showMessages = true);
    bool jsonDifferent(const QJsonObject &a, const QString &path);
    bool hasEmptyField(const QJsonObject &obj);
    void fetch_claim_certs();
    void runFleetProvisionerAsync();
    void checkCertificates();

private:
    QString caPath      = CLAIM_PATH + "AmazonRootCA1.pem";
    QString certPath    = CLAIM_PATH + "claim_cert.pem";
    QString keyPath     = CLAIM_PATH + "claim_private.key";
    QString prvCertPath = PROV_PATH  + "deviceCert.pem";
    QString prvkeyPath  = PROV_PATH  + "privateKey.pem";

protected:
    QWidget *uiBase = nullptr;

public:
    QString claimBasePath, provBasePath;
    void setUiPtr(QWidget *ptr) { uiBase = ptr;}
    void setCertPath(const QString claimPath, const QString provPath)
        {claimBasePath = claimPath; provBasePath = provPath;}

};

#endif // COMMONSETTING_H
