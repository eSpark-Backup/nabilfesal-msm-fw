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

#define MQTT_PATH "config/mqtt_settings.json"

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
        // qDebug().noquote() << "TelemetryJson: " << payload;
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

    initializeTelemetryJson();
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

    // // Log the values
    // qDebug() << "MQTT Settings Loaded:";
    // qDebug() << "  Host:" << host;
    // qDebug() << "  User:" << username;
    // qDebug() << "  Pass:" << password;
    // qDebug() << "  Port:" << port;
    // qDebug() << "  Client ID:" << clientId;
    // qDebug() << "  Topic:" << topic;
    // qDebug() << "  Interval (sec):" << intervalSec;

    return true;
}

void AwsMqttManager::reloadSettings()
{
    configPath = MQTT_PATH;
    if (!loadConfig(configPath)) {
        qWarning() << "No valid config found. Skipping MQTT connection.";
        return;
    }
    connectToBroker();
}

bool AwsMqttManager::mqttState()
{
    return isConnected;
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

QByteArray AwsMqttManager::buildTelemetryJson() const
{
    QJsonObject root;

    // metadata
    root["date"]       = QDate::currentDate().toString("dd-MM-yyyy");
    root["time"]       = QTime::currentTime().toString("HH:mm");
    root["ip_address"] = Utils::getDeviceIp();
    root["ofp_uid"]    = Utils::getUid();

    // All channel data
    for (auto it = channelTelemetry.begin(); it != channelTelemetry.end(); ++it) {
        const QJsonObject &channelData = it.value();
        for (auto key : channelData.keys()) {
            root[key] = channelData[key];
        }
    }

    // Serialize JSON → QByteArray
    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

void AwsMqttManager::processIncomingJson(int channel, const QJsonObject &payload)
{
    // QJsonDocument doc = QJsonDocument::fromJson(payload);
    QJsonObject root = payload;
    QJsonObject data = root["data"].toObject();

    QJsonObject phaseA = data["phase_a"].toObject();
    QJsonObject phaseB = data["phase_b"].toObject();
    QJsonObject phaseC = data["phase_c"].toObject();
    QJsonObject telemetry;

    if(channel == 1)
    {
        // =============== INCOMING CHANNEL =================
        telemetry["V_R"]     = phaseA["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_R1"]  = phaseA["general"].toObject()["current"].toDouble();
        telemetry["In1_RPF"] = phaseA["power"].toObject()["factor"].toDouble();

        telemetry["V_Y"]     = phaseB["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_Y1"]  = phaseB["general"].toObject()["current"].toDouble();
        telemetry["In1_YPF"] = phaseB["power"].toObject()["factor"].toDouble();

        telemetry["V_B"]     = phaseC["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_B1"]  = phaseC["general"].toObject()["current"].toDouble();
        telemetry["In1_BPF"] = phaseC["power"].toObject()["factor"].toDouble();

        // Power
        double P1_R = phaseA["power"].toObject()["active"].toDouble();
        double P1_B = phaseB["power"].toObject()["active"].toDouble();
        double P1_Y = phaseC["power"].toObject()["active"].toDouble();
        telemetry["P1_R"] = P1_R;
        telemetry["P1_B"] = P1_B;
        telemetry["P1_Y"] = P1_Y;
        telemetry["P1"]   = P1_R + P1_B + P1_Y;

        // Reactive
        double Q1_R = phaseA["power"].toObject()["reactive"].toDouble();
        double Q1_B = phaseB["power"].toObject()["reactive"].toDouble();
        double Q1_Y = phaseC["power"].toObject()["reactive"].toDouble();
        telemetry["Q1_R"] = Q1_R;
        telemetry["Q1_B"] = Q1_B;
        telemetry["Q1_Y"] = Q1_Y;
        telemetry["Q1"]   = Q1_R + Q1_B + Q1_Y;

        // Apparent
        double S1_R = phaseA["power"].toObject()["apparent"].toDouble();
        double S1_B = phaseB["power"].toObject()["apparent"].toDouble();
        double S1_Y = phaseC["power"].toObject()["apparent"].toDouble();
        telemetry["S1_R"] = S1_R;
        telemetry["S1_B"] = S1_B;
        telemetry["S1_Y"] = S1_Y;
        telemetry["S1"]   = S1_R + S1_B + S1_Y;

        // Energy
        QJsonObject energyA = phaseA["energy"].toObject();
        QJsonObject activeA   = energyA["active"].toObject();
        QJsonObject reactiveA = energyA["reactive"].toObject();

        telemetry["Epimp1"] = activeA["negative"].toDouble();
        telemetry["Epexp1"] = activeA["positive"].toDouble();
        telemetry["Eqimp1"] = reactiveA["negative"].toDouble();
        telemetry["Eqexp1"] = reactiveA["positive"].toDouble();
    }
    else
    {
        // =============== OUTGOING CHANNELS =================
          // Outgoing channels: F(channel-1)_ prefix
        int idx = channel - 1;  // Channel 2 → F1, Channel 3 → F2, … Channel 12 → F11
        QString prefix = QString("F%1_").arg(idx);

        double P_R = phaseA["power"].toObject()["active"].toDouble();
        double P_B = phaseB["power"].toObject()["active"].toDouble();
        double P_Y = phaseC["power"].toObject()["active"].toDouble();

        double Q_R = phaseA["power"].toObject()["reactive"].toDouble();
        double Q_B = phaseB["power"].toObject()["reactive"].toDouble();
        double Q_Y = phaseC["power"].toObject()["reactive"].toDouble();

        double S_R = phaseA["power"].toObject()["apparent"].toDouble();
        double S_B = phaseB["power"].toObject()["apparent"].toDouble();
        double S_Y = phaseC["power"].toObject()["apparent"].toDouble();

        QJsonObject energyA   = phaseA["energy"].toObject();
        QJsonObject activeA   = energyA["active"].toObject();
        QJsonObject reactiveA = energyA["reactive"].toObject();

        telemetry[prefix + "R"]     = phaseA["general"].toObject()["current"].toDouble();
        telemetry["PF_R" + QString::number(idx)] = phaseA["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "Y"]     = phaseB["general"].toObject()["current"].toDouble();
        telemetry["PF_Y" + QString::number(idx)] = phaseB["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "B"]     = phaseC["general"].toObject()["current"].toDouble();
        telemetry["PF_B" + QString::number(idx)] = phaseC["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "P"] = P_R + P_B + P_Y;
        telemetry[prefix + "Q"] = Q_R + Q_B + Q_Y;
        telemetry[prefix + "S"] = S_R + S_B + S_Y;

        telemetry[prefix + "Epimp"] = activeA["negative"].toDouble();
        telemetry[prefix + "Epexp"] = activeA["positive"].toDouble();
        telemetry[prefix + "Eqimp"] = reactiveA["negative"].toDouble();
        telemetry[prefix + "Eqexp"] = reactiveA["positive"].toDouble();
    }

    channelTelemetry[channel] = telemetry;
    QJsonDocument debugDoc(telemetry);
    qDebug().noquote() << "Telemetry for channel" << channel << ":" << debugDoc.toJson(QJsonDocument::Compact);
}

void AwsMqttManager::initializeTelemetryJson()
{
 // Initialize channel telemetry with 0s
    for (int ch = 1; ch <= 12; ++ch) {
        QJsonObject defaults;

        if (ch == 1) {
            QStringList keys = {"V_R","In1_R1","In1_RPF","V_Y","In1_Y1","In1_YPF",
                                "V_B","In1_B1","In1_BPF",
                                "P1_R","P1_B","P1_Y","P1",
                                "Q1_R","Q1_B","Q1_Y","Q1",
                                "S1_R","S1_B","S1_Y","S1",
                                "Epimp1","Epexp1","Eqimp1","Eqexp1"};
            for (const QString &k : keys)
                defaults[k] = 0;
        } else {
            QString suffix = QString::number(ch - 1);
            QStringList keys = {
                "F" + suffix + "_R", "PF_R" + suffix,
                "F" + suffix + "_Y", "PF_Y" + suffix,
                "F" + suffix + "_B", "PF_B" + suffix,
                "F" + suffix + "_P",
                "F" + suffix + "_Q",
                "F" + suffix + "_S",
                "F" + suffix + "_Epimp", "F" + suffix + "_Epexp",
                "F" + suffix + "_Eqimp", "F" + suffix + "_Eqexp"
            };
            for (const QString &k : keys)
                defaults[k] = 0;
        }

        channelTelemetry[ch] = defaults;
    }
}
