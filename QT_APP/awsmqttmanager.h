#pragma once
#include <QObject>
#include <QMqttClient>
#include <QTimer>

class AwsMqttManager : public QObject
{
    Q_OBJECT
public:
    explicit AwsMqttManager(QObject *parent = nullptr);

    bool loadConfig(const QString &filePath);
    void reloadSettings();
    void disconnectClient();
    void publishMessage(const QString &payload);
    void subscribeToTopic();
    bool mqttState();

public slots:
    void processIncomingJson(int channel, const QJsonObject &payload);

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
    bool isConnected = false;

    // System certificate paths (fixed)
    QString caPath = "/etc/certs/AmazonRootCA1.pem";
    QString certPath = "/etc/certs/deviceCert.pem";
    QString keyPath = "/etc/certs/privateKey.pem";

    QTimer *publishTimer = nullptr;
};

