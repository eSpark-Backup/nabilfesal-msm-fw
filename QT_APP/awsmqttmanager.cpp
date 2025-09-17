#include "awsmqttmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslConfiguration>
#include "utils.h"

#define CONFIG_PATH "config/mqtt_settings.json"

QByteArray AwsMqttManager::buildTelemetryJson() const
{
    QJsonObject root;

    // Add metadata
    root["date"]       = QDate::currentDate().toString("dd-MM-yyyy");
    root["time"]       = QTime::currentTime().toString("HH:mm");
    root["ip_address"] = Utils::getDeviceIp();
    root["ofp_uid"]    = Utils::getUid();

    // Build "data" object
    QJsonObject data;

    // json
    data["V_R"]    = QJsonValue();
    data["In1_R1"] = 123.45;
    data["In1_RPF"] = 0.98;
    data["V_Y"] = 1.0;
    data["In1_Y1"] = 1.1;
    data["In1_YPF"] = 01.2;
    data["V_B"] = 1.3;
    data["In1_B1"] = 1.4;
    data["In1_BPF"] = 1.5;
    data["P1_R"] = 1.6;
    data["P1_B"] = 1.7;
    data["P1_Y"] = 1.8;
    data["P1"] = 0;
    data["Q1_R"] = 0;
    data["Q1_B"] = 0;
    data["Q1_Y"] = 0;
    data["Q1"] = 0;
    data["S1_R"] = 0;
    data["S1_B"] = 0;
    data["S1_Y"] = 0;
    data["S1"] = 0;
    data["Epimp1"] = 0;
    data["Epexp1"] = 0;
    data["Eqimp1"] = 0;
    data["Eqexp1"] = 0;
    data["F1_R"] = 0;
    data["PF_R1"] = 0;
    data["F1_Y"] = 0;
    data["PF_Y1"] = 0;
    data["F1_B"] = 0;
    data["PF_B1"] = 0;
    data["F1_P"] = 0;
    data["F1_Q"] = 0;
    data["F1_S"] = 0;
    data["F1_Epimp"] = 0;
    data["F1_Epexp"] = 0;
    data["F1_Eqimp"] = 0;
    data["F1_Eqexp"] = 0;
    data["F2_R"] = 0;
    data["PF_R2"] = 0;
    data["F2_Y"] = 0;
    data["PF_Y2"] = 0;
    data["F2_B"] = 0;
    data["PF_B2"] = 0;
    data["F2_P"] = 0;
    data["F2_Q"] = 0;
    data["F2_S"] = 0;
    data["F2_Epimp"] = 0;
    data["F2_Epexp"] = 0;
    data["F2_Eqimp"] = 0;
    data["F2_Eqexp"] = 0;
    data["F3_R"] = 0;
    data["PF_R3"] = 0;
    data["F3_Y"] = 0;
    data["PF_Y3"] = 0;
    data["F3_B"] = 0;
    data["PF_B3"] = 0;
    data["F3_P"] = 0;
    data["F3_Q"] = 0;
    data["F3_S"] = 0;
    data["F3_Epimp"] = 0;
    data["F3_Epexp"] = 0;
    data["F3_Eqimp"] = 0;
    data["F3_Eqexp"] = 0;
    data["F4_R"] = 0;
    data["PF_R4"] = 0;
    data["F4_Y"] = 0;
    data["PF_Y4"] = 0;
    data["F4_B"] = 0;
    data["PF_B4"] = 0;
    data["F4_P"] = 0;
    data["F4_Q"] = 0;
    data["F4_S"] = 0;
    data["F4_Epimp"] = 0;
    data["F4_Epexp"] = 0;
    data["F4_Eqimp"] = 0;
    data["F4_Eqexp"] = 0;
    data["F5_R"] = 0;
    data["PF_R5"] = 0;
    data["F5_Y"] = 0;
    data["PF_Y5"] = 0;
    data["F5_B"] = 0;
    data["PF_B5"] = 0;
    data["F5_P"] = 0;
    data["F5_Q"] = 0;
    data["F5_S"] = 0;
    data["F5_Epimp"] = 0;
    data["F5_Epexp"] = 0;
    data["F5_Eqimp"] = 0;
    data["F5_Eqexp"] = 0;
    data["F6_R"] = 0;
    data["PF_R6"] = 0;
    data["F6_Y"] = 0;
    data["PF_Y6"] = 0;
    data["F6_B"] = 0;
    data["PF_B6"] = 0;
    data["F6_P"] = 0;
    data["F6_Q"] = 0;
    data["F6_S"] = 0;
    data["F6_Epimp"] = 0;
    data["F6_Epexp"] = 0;
    data["F6_Eqimp"] = 0;
    data["F6_Eqexp"] = 0;
    data["F7_R"] = 0;
    data["PF_R7"] = 0;
    data["F7_Y"] = 0;
    data["PF_Y7"] = 0;
    data["F7_B"] = 0;
    data["PF_B7"] = 0;
    data["F7_P"] = 0;
    data["F7_Q"] = 0;
    data["F7_S"] = 0;
    data["F7_Epimp"] = 0;
    data["F7_Epexp"] = 0;
    data["F7_Eqimp"] = 0;
    data["F7_Eqexp"] = 0;
    data["F8_R"] = 0;
    data["PF_R8"] = 0;
    data["F8_Y"] = 0;
    data["PF_Y8"] = 0;
    data["F8_B"] = 0;
    data["PF_B8"] = 0;
    data["F8_P"] = 0;
    data["F8_Q"] = 0;
    data["F8_S"] = 0;
    data["F8_Epimp"] = 0;
    data["F8_Epexp"] = 0;
    data["F8_Eqimp"] = 0;
    data["F8_Eqexp"] = 0;
    
    // Attach to root
    root["data"] = data;

    // Serialize JSON → QByteArray
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

AwsMqttManager::AwsMqttManager(QObject *parent) : QObject(parent)
{
    mqttClient = new QMqttClient(this);
    publishTimer = new QTimer(this);
    isConnected  = false;   // add as member variable (bool)

    // Timer: publish telemetry if connected
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "Skipped publish, client not connected.";
            return;
        }
        QByteArray payload = buildTelemetryJson();
        publishMessage(payload);
    });

    // Connected signal
    connect(mqttClient, &QMqttClient::connected, this, [this]() {
        isConnected = true;
        qDebug() << "Connected to broker!";

        if (intervalSec > 0) {
            publishTimer->start(intervalSec*1000);
        }
    });

    // Disconnected signal
    connect(mqttClient, &QMqttClient::disconnected, this, [this]() {
        isConnected = false;
        qDebug() << "Disconnected from broker.";
        publishTimer->stop();
    });
}

bool AwsMqttManager::loadConfig(const QString &filePath)
{
    configPath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "Config file not found:" << filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON config";
        return false;
    }

    QJsonObject obj = doc.object();
    host = obj.value("ip").toString();
    username = obj.value("user").toString();
    password = obj.value("password").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    clientId = Utils::getUid();
    topic = obj.value("topic").toString();
    QString intervalSecStr = obj.value("interval").toString();
    intervalSec = intervalSecStr.toInt();

    // Log the values
    qDebug() << "MQTT Settings Loaded:";
    qDebug() << "  Host:" << host;
    qDebug() << "  User:" << username;
    qDebug() << "  Pass:" << password;
    qDebug() << "  Port:" << port;
    qDebug() << "  Client ID:" << clientId;
    qDebug() << "  Topic:" << topic;
    qDebug() << "  Interval (ms):" << intervalSec;

    return true;
}

void AwsMqttManager::reloadSettings()
{
    configPath = CONFIG_PATH;
    if (!loadConfig(configPath)) {
        qWarning() << "No valid config found. Skipping MQTT connection.";
        return;
    }
    connectToBroker();
}

bool AwsMqttManager::loadCertificates()
{
    QFile caFile("/etc/ssl/certs/AmazonRootCA1.pem");
    QFile certFile("/etc/ssl/certs/deviceCert.crt");
    QFile keyFile("/etc/ssl/certs/privateKey.key");

    if (!caFile.open(QIODevice::ReadOnly) ||
        !certFile.open(QIODevice::ReadOnly) ||
        !keyFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open cert files";
        return false;
    }

    QSslConfiguration sslConf;
    sslConf.addCaCertificate(QSslCertificate(&caFile));
    sslConf.setLocalCertificate(QSslCertificate(&certFile));
    sslConf.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem));

    // secure socket
    QSslSocket *sslSocket = new QSslSocket(this);
    sslSocket->setSslConfiguration(sslConf);

    // bind socket to mqtt client
    mqttClient->setTransport(sslSocket, QMqttClient::IODevice);

    return true;
}

void AwsMqttManager::connectToBroker()
{
    if (host.isEmpty() || clientId.isEmpty() || topic.isEmpty()) {
        qWarning() << "MQTT settings incomplete.";
        return;
    }

    // if (!loadCertificates()) {
    //     qWarning() << "Missing certificates. Cannot connect.";
    //     return;
    // }
    mqttClient->setHostname(host);
    mqttClient->setPort(port);
    mqttClient->setClientId(clientId);
    if (!username.isEmpty()) mqttClient->setUsername(username);
    if (!password.isEmpty()) mqttClient->setPassword(password);
    mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);
    mqttClient->setCleanSession(true);
    mqttClient->setKeepAlive(30);

    // Use the transport socket set in loadCertificates()
    mqttClient->connectToHost();
}

void AwsMqttManager::disconnectClient()
{
    disconnectFromBroker();
}

void AwsMqttManager::disconnectFromBroker()
{
    if (!mqttClient) return;
    
    if (mqttClient->state() != QMqttClient::Disconnected)
    {
        publishTimer->stop();
        mqttClient->disconnectFromHost();
    }
}

void AwsMqttManager::publishMessage(const QString &payload)
{
    if (mqttClient->state() == QMqttClient::Connected) {
        auto id = mqttClient->publish(topic, payload.toUtf8(), 1);
        if (id == -1)
            qWarning() << "Publish failed!";
    }
}

void AwsMqttManager::subscribeToTopic()
{
    if (mqttClient->state() != QMqttClient::Connected)
        return;

    auto sub = mqttClient->subscribe(topic, 1);
    if (!sub) {
        qWarning() << "Subscribe failed!";
        return;
    }

    connect(sub, &QMqttSubscription::messageReceived, this,
            [](const QMqttMessage &msg) {
                qDebug() << "Message received:" << msg.payload();
            });
}
