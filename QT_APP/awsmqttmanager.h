#pragma once
#include <QObject>
#include <QTimer>
#include <QReadWriteLock>
#include <QtMqtt/QMqttClient>
#include <QCoreApplication>


struct DeviceInfo {
    QString uuid;
    QString pass;
    QString ip;
    QString imei;
    QString date;
    QString ntpStatus;
    QString mqttStatus;
    QString sftpStatus;
};


struct NtpSettings {
    QStringList servers;  // up to 3 servers

    bool operator!=(const NtpSettings &other) const {
        return servers != other.servers;
    }
};


class AwsMqttManager : public QObject
{
    Q_OBJECT
public:
    static AwsMqttManager& instance();

    bool loadAwsConfig(const QString &filePath);
    void reloadAwsMqttsSettings();
    void disconnectAwsClient();
    bool awsMqttsState();

signals:
    //subscribe signals
    void rebootRequested();
    void ntpConfigReceived(const QJsonObject &payload);
    void mqttConfigReceived(const QJsonObject &payload);
    void sftpConfigReceived(const QJsonObject &payload);
    void updateAwsData();

public slots:

    //publish slots
    void processAwsTelemetry(int channel, const QJsonObject &payload);
    void processDeviceInfo(const DeviceInfo &info);
    void processMqttSettings(const QJsonObject &mqttSetting);
    void processSftpSettings(const QJsonObject &sftpSetting);
    void processNtpSettings(const NtpSettings &ntpSettings);
    void processReboot();
    
private:
    explicit AwsMqttManager(QObject *parent = nullptr); // private ctor
    AwsMqttManager(const AwsMqttManager&) = delete;     // prevent copy
    AwsMqttManager& operator=(const AwsMqttManager&) = delete; // prevent assignment
    void publishMessage(const QString pubTopic, const QString &payload);
    void subscribeToTopic();
    void awsTopicsInit();
    void connectToBroker();
    void disconnectFromBroker();
    bool loadCertificates();
    static bool debugEnabled();
    QByteArray buildAwsTelemetryJson(int startIndex, int endIndex) const;

    DeviceInfo newDeviceInfo;
    QString configPath;
    QMqttClient *awsMqttsClient = nullptr;
    QString host;
    int port = 8883;
    QString clientId;
    int intervalSec = 10;
    bool isConnected = false;
    bool isFirstTime = true;

    #if defined(BUILD_X86)
        QString basePath = QCoreApplication::applicationDirPath() + "/data/certs/";
        QString caPath   = basePath + "AmazonRootCA1.pem";
    #elif defined(BUILD_V3S)
        QString basePath = "/MSM/potential-pancake/MSM/creds/prov_certs/";
        QString caPath = "/MSM/potential-pancake/MSM/creds/claim_certs/AmazonRootCA1.pem"; //v3s
    #endif

    QString certPath = basePath + "deviceCert.pem";
    QString keyPath  = basePath + "privateKey.pem";

    QString topic_pub_telemetry;
    QString topic_pub_device_info;
    QString topic_pub_reboot;
    QString topic_pub_ntp;
    QString topic_pub_mqtt;
    QString topic_pub_sftp;
    QString topic_sub_reboot;
    QString topic_sub_ntp;
    QString topic_sub_mqtt;
    QString topic_sub_sftp;

    QTimer *publishTimer = nullptr;
    QSslConfiguration sslConf;
    QSslSocket *sslSocket = nullptr;
    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
    QTimer *reconnectTimer;
};

