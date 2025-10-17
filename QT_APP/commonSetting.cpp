#include "commonSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include "toast.h"

#define MQTT_CONFIG_PATH "config/mqtt_settings.json"
#define SFTP_CONFIG_PATH "config/sftp_settings.json"

QJsonObject settingsManager::loadMqttSettings(bool showMessages, bool reloadSettings)
{
    QFile file(MQTT_CONFIG_PATH);
    if (!file.exists()) {
        if(!showMessages) Toast::show("No MQTT settings Found!", this, Toast::Warning);
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (!showMessages) Toast::show("Failed to open MQTT setting file!", this, Toast::Error);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (!showMessages) Toast::show("[MQTT] Invalid JSON format: " + parseError.errorString(), this, Toast::Error);
        return {};
    }

    QJsonObject mqttSettings = doc.object();
    if(reloadSettings) emit mqtt_load();
    return mqttSettings;
}


void settingsManager::saveMqttSettings(QJsonObject mqttSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(mqttSetting, MQTT_CONFIG_PATH);
    if (!isJsonDiff) {
        if (showMessages) Toast::show("[MQTT] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }

    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(MQTT_CONFIG_PATH);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if(showMessages) Toast::show("Failed to save MQTT settings!", this, Toast::Error);
        return;
    }

    QJsonDocument doc(mqttSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if(showMessages) 
    {
        Toast::show("MQTT settings saved successfully!", this, Toast::Info);
        emit mqttSettingsChanged(mqttSetting);
    }
    emit mqtt_load();
}


void settingsManager::saveSftpSettings(QJsonObject sftpSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(sftpSetting, SFTP_CONFIG_PATH);
    if (!isJsonDiff) {
        if (showMessages) Toast::show("[SFTP] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }
    
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/factory_sftp_settings.json
    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (showMessages) Toast::show("Failed to save SFTP settings!", this, Toast::Error);
        return;
    }

    QJsonDocument doc(sftpSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (showMessages) {
        Toast::show("SFTP settings saved successfully!", this, Toast::Info);
        emit sftpSettingsChanged(sftpSetting);
    }
    emit sftp_load();    
}


QJsonObject settingsManager::loadSftpSettings(bool showMessages, bool reloadSettings)
{
    QFile file("config/sftp_settings.json");
    if (!file.exists()) {
        if (!showMessages) Toast::show("No SFTP settings found!", this, Toast::Warning);
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (!showMessages) Toast::show("Failed to open SFTP setting file!", this, Toast::Error);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (!showMessages) Toast::show("[SFTP] Invalid JSON format: " + parseError.errorString(), this, Toast::Error);
        return {};
    }

    QJsonObject sftpSettings = doc.object();

    if(reloadSettings) emit sftp_load();
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

bool settingsManager::hasEmptyField(const QJsonObject &obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.value().toString().trimmed().isEmpty())
            return true;
    }
    return false;
}