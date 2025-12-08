#ifndef COMMONSETTING_H
#define COMMONSETTING_H

#include <QWidget>
#include <QLineEdit> 
#include "mqttManager.h"
#include "sftpManager.h"
#include "common_headers.h"

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

protected:
    virtual QString mqttConfigPath() const = 0;
    virtual QString sftpConfigPath() const = 0;
    virtual QString mqttsConfigPath() const = 0;
    virtual bool eventFilter(QObject *obj, QEvent *event) = 0;
    QJsonObject loadMqttSettings(bool showMessages, bool reloadSettings = true);
    QJsonObject loadSftpSettings(bool showMessages, bool reloadSettings = true);
    QJsonObject loadMqttsSettings(bool showMessages, bool reloadSettings = true);
    void saveMqttSettings(QJsonObject mqttSetting, bool showMessages = true);
    void saveSftpSettings(QJsonObject sftpSetting, bool showMessages = true);
    void saveMqttsSettings(QJsonObject mqttsSetting, bool showMessages = true);
    bool jsonDifferent(const QJsonObject &a, const QString &path);
    bool hasEmptyField(const QJsonObject &obj);
    void fetch_claim_certs();
    void runFleetProvisionerAsync();
    void checkCertificates();

private:

protected:
    QWidget *uiBase = nullptr;

public:
    void setUiPtr(QWidget *ptr) { uiBase = ptr; }

};

#endif // COMMONSETTING_H
