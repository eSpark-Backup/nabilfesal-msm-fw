#pragma once
#include <QObject>
#include <QMqttClient>
#include <QTimer>
#include <QReadWriteLock>

class AwsMqttManager : public QObject
{
    Q_OBJECT
public:
    static AwsMqttManager& instance();

    bool loadConfig(const QString &filePath);
    void reloadSettings();
    void disconnectClient();
    void publishMessage(const QString &payload);
    void subscribeToTopic();
    bool mqttState();

public slots:
    void processIncomingJson(int channel, const QJsonObject &payload);

private:
    explicit AwsMqttManager(QObject *parent = nullptr); // private ctor
    AwsMqttManager(const AwsMqttManager&) = delete;     // prevent copy
    AwsMqttManager& operator=(const AwsMqttManager&) = delete; // prevent assignment
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
    bool isConnected = false;

    // System certificate paths (fixed)
    QString caPath = "/etc/certs/AmazonRootCA1.pem";
    QString certPath = "/etc/certs/deviceCert.pem";
    QString keyPath = "/etc/certs/privateKey.pem";

    QTimer *publishTimer = nullptr;
    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
};

