#include "commonSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDir>


#define MQTT_CONFIG_PATH "config/mqtt_settings.json"
#define SFTP_CONFIG_PATH "config/sftp_settings.json"

QJsonObject settingsManager::loadMqttSettings(bool showMessages)
{
    QFile file(MQTT_CONFIG_PATH);
    if (!file.exists()) {
        if (showMessages)
            QMessageBox::warning(this, "MQTT Settings", "No saved settings found.");
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (showMessages)
            QMessageBox::critical(this, "MQTT Settings", "Failed to open settings file.");
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (showMessages)
            QMessageBox::critical(this, "MQTT Settings", "Invalid JSON format: " + parseError.errorString());
        return {};
    }

    QJsonObject mqttSettings = doc.object();
    emit mqtt_load();
    return mqttSettings;
}


void settingsManager::saveMqttSettings(QJsonObject mqttSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(mqttSetting, MQTT_CONFIG_PATH);
    if (!isJsonDiff) {
        qInfo() << "[MQTT] Settings unchanged, not rewriting file.";
        return;
    }

    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(MQTT_CONFIG_PATH);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if(showMessages) QMessageBox::critical(this, "MQTT Settings", "Failed to save MQTT settings.");
        return;
    }

    QJsonDocument doc(mqttSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if(showMessages) 
    {
        QMessageBox::information(this, "MQTT Settings", "MQTT settings saved successfully!");
        emit mqttSettingsChanged(mqttSetting);
    }
    emit mqtt_load();
    qDebug() << "mqtt signal passed";
}


void settingsManager::saveSftpSettings(QJsonObject sftpSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(sftpSetting, SFTP_CONFIG_PATH);
    if (!isJsonDiff) {
        qInfo() << "[SFTP] Settings unchanged, not rewriting file.";
        return;
    }
    
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/factory_sftp_settings.json
    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (showMessages) QMessageBox::critical(this, "SFTP Settings", "Failed to save SFTP settings.");
        return;
    }

    QJsonDocument doc(sftpSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (showMessages) {
        QMessageBox::information(this, "SFTP Settings", "SFTP settings saved successfully!");
        emit sftpSettingsChanged(sftpSetting);
    }
    emit sftp_load();    
}


QJsonObject settingsManager::loadSftpSettings(bool showMessages)
{
    QFile file("config/sftp_settings.json");
    if (!file.exists()) {
        if (showMessages)
            QMessageBox::warning(this, "SFTP Settings", "No saved settings found.");
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (showMessages)
            QMessageBox::critical(this, "SFTP Settings", "Failed to open settings file.");
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (showMessages)
            QMessageBox::critical(this, "SFTP Settings",
                                  "Invalid JSON format: " + parseError.errorString());
        return {};
    }

    QJsonObject sftpSettings = doc.object();

    emit sftp_load();
    return sftpSettings;
}


bool settingsManager::jsonDifferent(const QJsonObject &a, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return true; // return empty object if not found
    }

    QJsonDocument old = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject b = old.object();

    if (a.keys() != b.keys())
        return true;

    for (const QString &key : a.keys()) {
        if (a.value(key) != b.value(key)) {
            return true; // mismatch found
        }
    }
    return false; // all fields same
}
