#pragma once
#include <QObject>
#include <QMqttClient>
#include <QTimer>
#include <QReadWriteLock>
#include <QCoreApplication>

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
    int port = 8883;
    QString clientId = "fleet_prov_shadow_test_1";
    QString topic;
    int intervalSec = 10;
    bool isConnected = false;

    // // System certificate paths (fixed)
    // QString caPath = "/etc/certs/AmazonRootCA1.pem";
    // QString certPath = "/etc/certs/deviceCert.pem";
    // QString keyPath = "/etc/certs/privateKey.pem";

    // x86 System certificate paths (fixed)
    QString basePath = QCoreApplication::applicationDirPath() + "/data/certs/";
    QString caPath   = basePath + "AmazonRootCA1.pem";
    QString certPath = basePath + "deviceCert.pem";
    QString keyPath  = basePath + "privateKey.pem";


    QTimer *publishTimer = nullptr;
    QSslConfiguration sslConf;
    QSslSocket *sslSocket = nullptr;
    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
};

