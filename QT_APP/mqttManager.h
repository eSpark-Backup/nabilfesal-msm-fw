#pragma once
#include <QObject>
#include <QMqttClient>
#include <QTimer>
#include <QReadWriteLock>
#include "common_headers.h"

class mqttManager : public QObject
{
    Q_OBJECT
public:
    explicit mqttManager(Mode mode, QObject *parent = nullptr); // private ctor
    mqttManager(const mqttManager&) = delete;     // prevent copy
    bool loadConfig(const QString &filePath);
    void reloadSettings();
    void disconnectClient();
    void publishMessage(const QString &payload);
    void subscribeToTopic();
    bool mqttState();
    bool isConnected = false;

public slots:
    void processIncomingJson(int channel, const QJsonObject &payload);
    void resetAccumulatedEnergy();
    void updateAccumulatedEnergy(const std::array<EnergyData,13> &energyData);

private:
    void initializeTelemetryJson();
    void connectToBroker();
    void disconnectFromBroker();
    bool loadCertificates();
    QByteArray buildTelemetryJson() const;

    QString configPath;
    QMqttClient *mqttClient = nullptr;

    // MQTT settings from JSON
    QString host;
    QString username;
    QString password;
    int port = 1883;
    QString clientId;
    QString topic;
    int intervalSec = 0;

    // System certificate paths (fixed)
    QString caPath = "/etc/certs/AmazonRootCA1.pem";
    QString certPath = "/etc/certs/deviceCert.pem";
    QString keyPath = "/etc/certs/privateKey.pem";

    QTimer *publishTimer = nullptr;
    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
    QTimer *reconnectTimer;

    // Assume channel numbers start at 1
    std::array<EnergyData, 13> mqttEnergy;
    Mode clientMode;

public:
    QString tag;
};
