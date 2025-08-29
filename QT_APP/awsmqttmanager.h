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
    void publishMessage(const QString &payload);
    void subscribeToTopic();

private:
    void connectToBroker();
    void disconnectFromBroker();
    bool loadCertificates();

    QString configPath;
    QMqttClient *mqttClient = nullptr;

    // MQTT settings from JSON
    QString host;
    int port = 8883;
    QString clientId;
    QString topic;
    int intervalMs = 0;

    // System certificate paths (fixed)
    QString caPath = "/etc/certs/AmazonRootCA1.pem";
    QString certPath = "/etc/certs/deviceCert.pem";
    QString keyPath = "/etc/certs/privateKey.pem";

    QTimer *publishTimer = nullptr;
};

