#ifndef COMMONSETTING_H
#define COMMONSETTING_H

#include <QWidget>
#include <QLineEdit> 
#include "mqttManager.h"
#include "sftpManager.h"


class settingsManager : public QWidget
{
    Q_OBJECT

public:
    explicit settingsManager(QWidget *parent = nullptr) {};
    ~settingsManager() {};
   

signals:
    void mqtt_load();
    void sftp_load();
    void mqttSettingsChanged(const QJsonObject &mqttSetting);
    void sftpSettingsChanged(const QJsonObject &sftpSetting);

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) = 0;
    QJsonObject loadMqttSettings(bool showMessages, bool reloadSettings = true);
    QJsonObject loadSftpSettings(bool showMessages, bool reloadSettings = true);
    void saveMqttSettings(QJsonObject sftpSetting, bool showMessages = true);
    void saveSftpSettings(QJsonObject sftpSetting, bool showMessages = true);
    bool jsonDifferent(const QJsonObject &a, const QString &path);
};

#endif // COMMONSETTING_H
