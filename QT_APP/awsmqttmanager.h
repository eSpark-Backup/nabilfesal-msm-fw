#pragma once
#include <QDir>
#include <QObject>
#include <QTimer>
#include <QReadWriteLock>
#include <QtMqtt/QMqttClient>
#include <QCoreApplication>
#include "common_headers.h"


struct DeviceInfo {
    QString uuid;
    QString pass;
    QString ip;
    QString imei;
    QString date;
    QString ntpStatus;
    QString mqttStatus;
    QString sftpStatus;
    QString mqttsStatus;
    QString gsmStatus;
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
    explicit AwsMqttManager(Mode mode, QObject *parent = nullptr); // private ctor
    AwsMqttManager(const AwsMqttManager&) = delete;     // prevent copy
    bool loadAwsConfig(const QString &filePath);
    void reloadAwsMqttsSettings();
    void disconnectAwsClient();
    bool awsMqttsState();
    bool isConnected = false;

signals:
    //subscribe signals
    void rebootRequested();
    void ntpConfigReceived(const QJsonObject &payload);
    void mqttConfigReceived(const QJsonObject &payload);
    void mqttsConfigReceived(const QJsonObject &payload);
    void sftpConfigReceived(const QJsonObject &payload);
    void ctRatioReceived(const QJsonObject &payload);
    void vOffsetReceived(const QJsonObject &payload);
    void iOffsetReceived(const QJsonObject &payload);
    void otaReceived(const QString &payload);
    void updateAwsData();
    void settingsChanged();

public slots:

    //publish slots
    void processAwsTelemetry(int channel, const QJsonObject &payload);
    void processDeviceInfo(const DeviceInfo &info);
    void processMqttSettings(const QJsonObject &mqttSetting);
    void processSftpSettings(const QJsonObject &sftpSetting);
    void processNtpSettings(const NtpSettings &ntpSettings);
    void processMqttsSettings(const QJsonObject &mqttsSetting);
    void processReboot();
    void processCtRatio(const QJsonObject &ctRatio);
    void processVOffset(const QJsonObject &vOffset);
    void processIOffset(const QJsonObject &iOffset);
    void processOTA(const QJsonObject &otaData);
    void resetAccumulatedEnergy();
    void updateAccumulatedEnergy(const std::array<EnergyData,13> &energyData);
    
private:
    void publishMessage(const QString pubTopic, const QString &payload);
    void subscribeToTopic();
    void awsTopicsInit();
    void connectToBroker();
    void disconnectFromBroker();
    bool loadCertificates();
    static bool debugEnabled();
    QByteArray buildAwsTelemetryJson(int startIndex, int endIndex) const;
    void otaSend(const QJsonObject &obj);
    bool fileExists(const QString &path);

    DeviceInfo newDeviceInfo;
    QString configPath;
    QMqttClient *awsMqttsClient = nullptr;
    QString host;
    int port = 8883;
    QString clientId;
    int intervalSec = 10;
    bool isFirstTime = true;

    QString caPath        = CLAIM_PATH + "AmazonRootCA1.pem";
    QString claimCertPath = CLAIM_PATH + "claim_cert.pem";
    QString claimKeyPath  = CLAIM_PATH + "claim_private.key";
    QString provCertPath  = PROV_PATH  + "deviceCert.pem";
    QString provKeyPath   = PROV_PATH  + "privateKey.pem";


    QString topic_pub_telemetry;
    QString topic_pub_device_info;
    QString topic_pub_reboot;
    QString topic_pub_ntp;
    QString topic_pub_mqtt;
    QString topic_pub_mqtts;
    QString topic_pub_sftp;
    QString topic_pub_ctRatio;
    QString topic_pub_vOffset;
    QString topic_pub_iOffset;
    QString topic_pub_ota;

    QString topic_sub_reboot;
    QString topic_sub_ntp;
    QString topic_sub_mqtt;
    QString topic_sub_mqtts;
    QString topic_sub_sftp;
    QString topic_sub_ctRatio;
    QString topic_sub_vOffset;
    QString topic_sub_iOffset;
    QString topic_sub_ota;
    
    QTimer *publishTimer = nullptr;
    QSslConfiguration sslConf;
    QSslSocket *sslSocket = nullptr;
    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
    QTimer *reconnectTimer;

    // Running totals for energy
    static std::array<EnergyData, 13> awsEnergy;
    Mode clientMode;

public:
    QString tag;
    QString aws_path = "config/admin_settings/aws_mqtts_settings.json";
};

