#include "commonSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDir>


QJsonObject settingsManager::loadMqttSettings(bool showMessages)
{
    QFile file("config/mqtt_settings.json");
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


void settingsManager::saveMqttSettings(QJsonObject mqttSetting)
{
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file("config/mqtt_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "MQTT Settings", "Failed to save MQTT settings.");
        return;
    }

    QJsonDocument doc(mqttSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "MQTT Settings", "MQTT settings saved successfully!");
    emit mqtt_load();
}


void settingsManager::saveSftpSettings(QJsonObject sftpSetting)
{
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/factory_sftp_settings.json
    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "SFTP Settings", "Failed to save SFTP settings.");
        return;
    }

    QJsonDocument doc(sftpSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "SFTP Settings", "SFTP settings saved successfully!");
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

