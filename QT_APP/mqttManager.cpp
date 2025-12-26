#include "mqttManager.h"
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
#include <cmath>

#define MQTT_PATH mqtt_path


mqttManager::mqttManager(Mode mode, QObject *parent) : clientMode(mode), QObject(parent)
{
    if(clientMode == Mode::Factory)
    {
        tag = COLOR_TAG(F_BLACK, B_WHITE, "Factory [MQTT]");
        mqtt_path = "config/mqtt_settings.json";
    }
    else
    {
        tag = COLOR_TAG(F_BLACK, B_WHITE, "Admin - MQTT");
    }
    mqttClient = new QMqttClient(this);
    publishTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);
    isConnected  = false;

    reconnectTimer->setInterval(30 * 1000);  // 30 seconds
    reconnectTimer->setSingleShot(false);    // repeat until success

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            reloadSettings();
        }
    });

    if (!reconnectTimer->isActive()) {
        reconnectTimer->start();  // retry every 1 min
    }

    // Publish timer
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            logWarn << "Skipped publish, client not connected.";
            return;
        }
        QByteArray payload = buildTelemetryJson();
        publishMessage(payload);
    });

    // Connected
    connect(mqttClient, &QMqttClient::connected, this, [this]() {
        isConnected = true;
        logDebug << "Connected to MQTT broker!";

        reconnectTimer->stop();

        if (intervalSec > 0) {
            Utils::startAlignedTimer( this,
                publishTimer, intervalSec,
                [this]() {
                    if (!isConnected) return;
                    QByteArray payload = buildTelemetryJson();
                    publishMessage(payload);
                }
            );
        }
    });


    // Disconnected
    connect(mqttClient, &QMqttClient::disconnected, this, [this]() {
        isConnected = false;
        logDebug << "Disconnected from MQTT broker.";

        publishTimer->stop();
        if (!reconnectTimer->isActive()) {
            reconnectTimer->start();  // retry every 1 min
            logWarn << "Reconnection attempt...";
        }
    });
}

bool mqttManager::loadConfig(const QString &filePath)
{
    configPath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        logWarn << "Config file not found:" << filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        logWarn << "Failed to open config file";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        logWarn << "Invalid JSON config";
        return false;
    }

    QJsonObject obj = doc.object();
    host = obj.value("ip").toString();
    username = obj.value("user").toString();
    password = obj.value("password").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    clientId = Utils::getUid();
    clientId += ((clientMode == Mode::Factory) ? "_factory" : "");
    topic = obj.value("topic").toString();
    QString intervalSecStr = obj.value("interval").toString();
    intervalSec = intervalSecStr.toInt();

    return true;
}

void mqttManager::reloadSettings()
{
    configPath = MQTT_PATH;
    if (!loadConfig(configPath)) {
        logWarn << "No valid config found. Skipping MQTT connection.";
        return;
    }
    connectToBroker();
}

bool mqttManager::mqttState()
{
    return isConnected;
}

void mqttManager::connectToBroker()
{
    if (host.isEmpty() || clientId.isEmpty() || topic.isEmpty()) {
        logWarn << "MQTT settings incomplete.";
        return;
    }
    mqttClient->setHostname(host);
    mqttClient->setPort(port);
    mqttClient->setClientId(clientId);
    mqttClient->setUsername(username);
    mqttClient->setPassword(password);
    mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);
    mqttClient->setCleanSession(true);
    mqttClient->setKeepAlive(30);

    // Use the transport socket set in loadCertificates()
    mqttClient->connectToHost();
}

void mqttManager::disconnectClient()
{
    disconnectFromBroker();
}

void mqttManager::disconnectFromBroker()
{
    if (!mqttClient) return;
    
    if (mqttClient->state() != QMqttClient::Disconnected)
    {
        publishTimer->stop();
        mqttClient->disconnectFromHost();
    }
}

void mqttManager::publishMessage(const QString &payload)
{
    if (mqttClient->state() == QMqttClient::Connected) {
        auto id = mqttClient->publish(topic, payload.toUtf8(), 1);
        if (id == -1)
            logWarn << "Publish failed!";
    }
}

void mqttManager::subscribeToTopic()
{
    if (mqttClient->state() != QMqttClient::Connected)
        return;

    auto sub = mqttClient->subscribe(topic, 1);
    if (!sub) {
        logWarn << "Subscribe failed!";
        return;
    }

    connect(sub, &QMqttSubscription::messageReceived, this,
            [this](const QMqttMessage &msg) {
                logDebug << "Message received:" << msg.payload();
            });
}

QByteArray mqttManager::buildTelemetryJson() const
{
    QJsonObject root;

    // metadata
    root["date"]       = QDate::currentDate().toString("dd-MM-yyyy");
    root["time"]       = QTime::currentTime().toString("HH:mm");
    root["ip_address"] = Utils::getDeviceIp();
    root["ofp_uid"]    = Utils::getUid();

    QReadLocker locker(&telemetryLock);

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


void mqttManager::processIncomingJson(int channel, const QJsonObject &payload)
{
    QJsonObject root = payload;
    QJsonObject data = root["data"].toObject();

    QJsonObject phaseA = data["phase_a"].toObject();
    QJsonObject phaseB = data["phase_b"].toObject();
    QJsonObject phaseC = data["phase_c"].toObject();

    // Accumulate energy for the current channel
    mqttEnergy[channel].negActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["negative"].toDouble();
    mqttEnergy[channel].negActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["negative"].toDouble();
    mqttEnergy[channel].negActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["negative"].toDouble();

    mqttEnergy[channel].posActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["positive"].toDouble();
    mqttEnergy[channel].posActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["positive"].toDouble();
    mqttEnergy[channel].posActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["positive"].toDouble();

    mqttEnergy[channel].negReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
    mqttEnergy[channel].negReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
    mqttEnergy[channel].negReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["negative"].toDouble();

    mqttEnergy[channel].posReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
    mqttEnergy[channel].posReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
    mqttEnergy[channel].posReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["positive"].toDouble();

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

        telemetry["T_C"]    = root["temperature"].toDouble();
        telemetry["Epexp1"] = mqttEnergy[1].negActEnergy[0] + mqttEnergy[1].negActEnergy[1] + mqttEnergy[1].negActEnergy[2];
        telemetry["Epimp1"] = mqttEnergy[1].posActEnergy[0] + mqttEnergy[1].posActEnergy[1] + mqttEnergy[1].posActEnergy[2];
        telemetry["Eqexp1"] = mqttEnergy[1].negReacEnergy[0] + mqttEnergy[1].negReacEnergy[1] + mqttEnergy[1].negReacEnergy[2];
        telemetry["Eqimp1"] = mqttEnergy[1].posReacEnergy[0] + mqttEnergy[1].posReacEnergy[1] + mqttEnergy[1].posReacEnergy[2];
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

        telemetry[prefix + "P"] = std::round((P_R + P_B + P_Y) * 100.0) / 100.0;
        telemetry[prefix + "Q"] = std::round((Q_R + Q_B + Q_Y) * 100.0) / 100.0;
        telemetry[prefix + "S"] = std::round((S_R + S_B + S_Y) * 100.0) / 100.0;

        telemetry[prefix + "T"]     = root["temperature"].toDouble();
        telemetry[prefix + "Epexp"] = mqttEnergy[channel].negActEnergy[0] + mqttEnergy[channel].negActEnergy[1] + mqttEnergy[channel].negActEnergy[2];
        telemetry[prefix + "Epimp"] = mqttEnergy[channel].posActEnergy[0] + mqttEnergy[channel].posActEnergy[1] + mqttEnergy[channel].posActEnergy[2];
        telemetry[prefix + "Eqexp"] = mqttEnergy[channel].negReacEnergy[0] + mqttEnergy[channel].negReacEnergy[1] + mqttEnergy[channel].negReacEnergy[2];
        telemetry[prefix + "Eqimp"] = mqttEnergy[channel].posReacEnergy[0] + mqttEnergy[channel].posReacEnergy[1] + mqttEnergy[channel].posReacEnergy[2];
    }
    
    QWriteLocker locker(&telemetryLock);
    channelTelemetry[channel] = telemetry;
}

void mqttManager::initializeTelemetryJson()
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


void mqttManager::resetAccumulatedEnergy()
{
    for(int ch = 1; ch <= 12; ++ch) {
        for(int ph = 0; ph < 3; ph++)
        {
            mqttEnergy[ch].posActEnergy[ph] = 0.0;
            mqttEnergy[ch].negActEnergy[ph] = 0.0;
            mqttEnergy[ch].posReacEnergy[ph] = 0.0;
            mqttEnergy[ch].negReacEnergy[ph] = 0.0;
        }
    }
}


void mqttManager::updateAccumulatedEnergy(const std::array<EnergyData,13> &data)
{
    mqttEnergy = data;
}
