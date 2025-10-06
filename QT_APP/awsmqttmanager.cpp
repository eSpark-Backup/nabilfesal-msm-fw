#include "awsmqttmanager.h"
#include <QFile>
#include <QDebug>
#include "utils.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMqtt/QMqttSubscription>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslConfiguration>
#include <QProcessEnvironment>

#define AWS_MQTTS_PATH "config/aws_mqtts_settings.json"


#define AWS_LOG(msg) \
    do { \
        if (AwsMqttManager::debugEnabled()) { qDebug() << msg; } \
    } while(0)


bool AwsMqttManager::debugEnabled()
{
    static bool enabled = []() {
        // Check once at startup
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        return env.contains("AWS_MQTT_DEBUG") && env.value("AWS_MQTT_DEBUG") == "1";
    }();
    return enabled;
}


AwsMqttManager& AwsMqttManager::instance()
{
    static AwsMqttManager _instance;  // created on first call, shared afterwards
    return _instance;
}

AwsMqttManager::AwsMqttManager(QObject *parent) : QObject(parent)
{
    awsMqttsClient = new QMqttClient(this);
    publishTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);

    reconnectTimer->setInterval(30 * 1000);  // 30 seconds
    reconnectTimer->setSingleShot(false);    // repeat until success

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "AWS Reconnection attempt...";
            connectToBroker();
        }
    });

    // Timer: publish telemetry if connected
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "Skipped publish, client not connected.";
            return;
        }

        int totalChannels = channelTelemetry.size();
        int mid = totalChannels / 2;

        QByteArray payload1 = buildAwsTelemetryJson(0, mid);
        AWS_LOG("\n========================================================================");
        AWS_LOG("Publishing first half of telemetry (" << 0 << "-" << mid-1 << ")");
        AWS_LOG("\n========================================================================");
        publishMessage(topic_pub_telemetry, payload1);
        // Publish second half
        QByteArray payload2 = buildAwsTelemetryJson(mid, totalChannels);
        AWS_LOG("\n========================================================================");
        AWS_LOG("Publishing second half of telemetry (" << mid << "-" << totalChannels-1 << ")");
        AWS_LOG("\n========================================================================");
        publishMessage(topic_pub_telemetry, payload2);

    });

    // Connected signal
    connect(awsMqttsClient, &QMqttClient::connected, this, [this]() {
        isConnected = true;
        qDebug() << "Connected to AWS IoT Core!";
        reconnectTimer->stop();

        if (intervalSec > 0) {
            publishTimer->start(intervalSec*1000);
        }
        awsTopicsInit();
        subscribeToTopic();
        if(isFirstTime) {
            isFirstTime = false;
            emit updateAwsData();
        }
    });

    // Disconnected signal
    connect(awsMqttsClient, &QMqttClient::disconnected, this, [this]() {
        isConnected = false;
        qDebug() << "Disconnected from AWS IoT Core.";
        publishTimer->stop();
        if (!reconnectTimer->isActive()) {
            reconnectTimer->start();  // retry every 1 min
        }
    });
}

bool AwsMqttManager::loadAwsConfig(const QString &filePath)
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
    host = obj.value("endpoint").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    clientId = Utils::getUid();
    QString intervalSecStr = obj.value("interval").toString();
    intervalSec = intervalSecStr.toInt();

    // Log the values
    AWS_LOG("AWS MQTTS Settings Loaded:");
    AWS_LOG("  Host:" << host);
    AWS_LOG("  Port:" << port);
    AWS_LOG("  Client ID:" << clientId);
    AWS_LOG("  Interval (sec):" << intervalSec);

    return true;
}

void AwsMqttManager::reloadAwsMqttsSettings()
{
    configPath = AWS_MQTTS_PATH;
    if (!loadAwsConfig(configPath)) {
        qWarning() << "No valid config found. Skipping AWS MQTTS connection.";
        return;
    }
    connectToBroker();
}


bool AwsMqttManager::awsMqttsState()
{
    return isConnected;
}


bool AwsMqttManager::loadCertificates()
{
    if (!QSslSocket::supportsSsl()) {
        qWarning() << "[TLS] SSL/TLS not supported on this system!";
        return false;
    }

    QFile caFile(caPath), certFile(certPath), keyFile(keyPath);
    if (!caFile.open(QIODevice::ReadOnly) ||
        !certFile.open(QIODevice::ReadOnly) ||
        !keyFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[TLS] Failed to open cert/key files";
        return false;
    }

    QList<QSslCertificate> caCerts = QSslCertificate::fromData(caFile.readAll(), QSsl::Pem);
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setCaCertificates(caCerts);
    conf.setLocalCertificate(QSslCertificate(certFile.readAll()));
    conf.setPrivateKey(QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem));
    conf.setProtocol(QSsl::TlsV1_2OrLater);

    // --- Debug TLS config ---
    AWS_LOG("=== TLS Config ===");
    AWS_LOG("Subject:" << conf.localCertificate().subjectInfo(QSslCertificate::CommonName));
    AWS_LOG("Private key valid?" << !conf.privateKey().isNull());
    AWS_LOG("CA count:" << conf.caCertificates().size());
    AWS_LOG("Protocol:" << conf.protocol());
    AWS_LOG("===================");

    if (conf.localCertificate().isNull() || conf.privateKey().isNull() || conf.caCertificates().isEmpty()) {
        qWarning() << "[TLS] Invalid certificate, key, or CA chain.";
        return false;
    }

    sslSocket = new QSslSocket();
    sslSocket->setSslConfiguration(conf);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslSocket->setPeerVerifyName(host);

    connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
        AWS_LOG("[TLS] Handshake complete (encrypted)");
        awsMqttsClient->connectToHost();
    });
    connect(sslSocket,
            static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors),
            this,
            [](const QList<QSslError> &errs) {
                for (const auto &e : errs)
                    qWarning() << "[TLS] SSL error:" << e.errorString();
            });

    connect(sslSocket, &QSslSocket::stateChanged, this, [](QAbstractSocket::SocketState state) {
        AWS_LOG( "[TLS] Socket state:" << state);
    });
    connect(sslSocket, &QSslSocket::errorOccurred, this, [](QAbstractSocket::SocketError err) {
        qWarning() << "[TLS] Socket error:" << err;
    });
    awsMqttsClient->setTransport(sslSocket, QMqttClient::IODevice);
    return true;
}


void AwsMqttManager::connectToBroker()
{
    if (host.isEmpty() || clientId.isEmpty()) {
        qWarning() << "[AWS MQTT] Settings incomplete.";
        return;
    }

    if (!loadCertificates()) {
        qWarning() << "[AWS MQTT] Missing certificates. Cannot connect.";
        return;
    }

    awsMqttsClient->setClientId(clientId);
    awsMqttsClient->setCleanSession(true);
    awsMqttsClient->setKeepAlive(30);
    sslSocket->connectToHostEncrypted(host, port);
    AWS_LOG("[TLS] Initiating encrypted connect to" << host << ":" << port);
}


void AwsMqttManager::disconnectAwsClient()
{
    disconnectFromBroker();
}


void AwsMqttManager::disconnectFromBroker()
{
    if (!awsMqttsClient) return;

    AWS_LOG("[MQTT] Disconnecting...");

    publishTimer->stop();

    if (awsMqttsClient->state() != QMqttClient::Disconnected)
        awsMqttsClient->disconnectFromHost();

    if (sslSocket) {
        sslSocket->disconnectFromHost();
        sslSocket->close();
        sslSocket->deleteLater();
        sslSocket = nullptr;
    }
}


void AwsMqttManager::publishMessage(const QString pubTopic, const QString &payload)
{
    if (awsMqttsClient->state() == QMqttClient::Connected) {
        auto id = awsMqttsClient->publish(pubTopic, payload.toUtf8(), 1);
        if (id == -1)
            qWarning() << "Publish on [" << pubTopic << "] failed!";
    }
}


void AwsMqttManager::subscribeToTopic()
{
    QStringList topics = {
        topic_sub_reboot,
        topic_sub_ntp,
        topic_sub_mqtt,
        topic_sub_sftp
    };

    for (const QString &t : topics) {
        auto *sub = awsMqttsClient->subscribe(t, 1);
        if (!sub) {
            qWarning() << "[MQTT] Subscribe failed for topic:" << t;
            continue;
        }

    connect(sub, &QMqttSubscription::messageReceived, this,
        [this](const QMqttMessage &msg) {
            const QString topic = msg.topic().name();
            QByteArray rawPayload = msg.payload();
            QJsonDocument doc = QJsonDocument::fromJson(rawPayload);
            QJsonObject obj;
            if (!doc.isNull() && doc.isObject()) {
                obj = doc.object();
                qDebug().noquote() << "\n[AWS] Topic: " << msg.topic() << "Message received:" << "Parsed JSON:" << QJsonDocument(obj).toJson(QJsonDocument::Indented);
            }

            if (topic == topic_sub_reboot) {
                emit rebootRequested();
            } else if (topic == topic_sub_ntp) {
                emit ntpConfigReceived(obj);
            } else if (topic == topic_sub_mqtt) {
                emit mqttConfigReceived(obj);
            } else if (topic == topic_sub_sftp) {
                emit sftpConfigReceived(obj);
            } else {
                qWarning() << "[MQTT] Unknown topic:" << topic;
            }
        });
    }
}


void AwsMqttManager::awsTopicsInit()
{
    QString base_pub_topic = "msm/" + clientId + "/pub"; //clientId to be replace with Utils::getUid()
    QString base_sub_topic = "msm/" + clientId + "/sub";
    
    topic_pub_telemetry     = base_pub_topic + "/telemetry";
    topic_pub_device_info   = base_pub_topic + "/device_info";
    topic_pub_reboot        = base_pub_topic + "/reboot";
    topic_pub_ntp           = base_pub_topic + "/ntp";
    topic_pub_mqtt          = base_pub_topic + "/mqtt";
    topic_pub_sftp          = base_pub_topic + "/sftp";

    topic_sub_reboot        = base_sub_topic + "/reboot";
    topic_sub_ntp           = base_sub_topic + "/ntp";
    topic_sub_mqtt          = base_sub_topic + "/mqtt";
    topic_sub_sftp          = base_sub_topic + "/sftp";
}


QByteArray AwsMqttManager::buildAwsTelemetryJson(int startIndex, int endIndex) const
{
    QJsonObject root;

    // ISO8601 timestamp with offset
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["device_uuid"] = Utils::getUid();

    QJsonArray channelsArray;

    QReadLocker locker(&telemetryLock);
    int i = 0;
    for (auto it = channelTelemetry.begin(); it != channelTelemetry.end(); ++it, ++i) {
        if (i >= startIndex && i < endIndex) {
            channelsArray.append(it.value());
        }
    }

    root["channels"] = channelsArray;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}


void AwsMqttManager::processAwsTelemetry(int channel, const QJsonObject &payload)
{
    QJsonObject cleanPayload = payload;
    QJsonObject dataObj = cleanPayload["data"].toObject();

    for (const QString &phase : {"phase_a", "phase_b", "phase_c"}) {
        if (dataObj.contains(phase)) {
            QJsonObject phaseObj = dataObj[phase].toObject();
            phaseObj.remove("avg_power");
            phaseObj.remove("avg_energy");
            phaseObj.remove("thd");
            dataObj[phase] = phaseObj;
        }
    }

    cleanPayload["data"] = dataObj;

    QWriteLocker locker(&telemetryLock);
    channelTelemetry[channel] = cleanPayload;
}


void AwsMqttManager::processDeviceInfo(const DeviceInfo &info)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject deviceInfoData;
    deviceInfoData["device_ip"] = info.ip;
    deviceInfoData["device_pass"] = info.pass;
    deviceInfoData["device_imei"] = info.imei;
    deviceInfoData["mqtt_status"] = info.mqttStatus == "Connected" ? true : false;
    deviceInfoData["sftp_status"] = info.sftpStatus == "Connected" ? true : false;

    QJsonArray dataArray;
    dataArray.append(deviceInfoData);

    QJsonObject deviceInfoPayload;
    deviceInfoPayload["data"] = dataArray;
    deviceInfoPayload["device_uuid"] = info.uuid;

    QJsonDocument doc(deviceInfoPayload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_device_info, jsonString);
}


void AwsMqttManager::processMqttSettings(const QJsonObject &mqttSetting)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject brokerData;
    brokerData["broker_ip"]     = mqttSetting.value("ip").toString();
    brokerData["broker_port"]   = mqttSetting.value("port").toString().toInt();
    brokerData["broker_user"]   = mqttSetting.value("user").toString();
    brokerData["broker_pass"]   = mqttSetting.value("password").toString();
    brokerData["data_interval"] = mqttSetting.value("interval").toString().toInt();
    brokerData["mqtt_topic"]    = mqttSetting.value("topic").toString();

    QJsonArray dataArray;
    dataArray.append(brokerData);

    QJsonObject payload;
    payload["device_uuid"] = Utils::getUid();
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_mqtt, jsonString);
}


void AwsMqttManager::processSftpSettings(const QJsonObject &sftpSetting)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject serverData;
    serverData["server_ip"]     = sftpSetting.value("ip").toString();
    serverData["server_port"]   = sftpSetting.value("port").toString().toInt();
    serverData["username"]   = sftpSetting.value("username").toString();
    serverData["password"]   = sftpSetting.value("password").toString();
    serverData["data_interval"] = sftpSetting.value("interval").toString().toInt();

    QJsonArray dataArray;
    dataArray.append(serverData);

    QJsonObject payload;
    payload["device_uuid"] = Utils::getUid();
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_sftp, jsonString);
}


void AwsMqttManager::processNtpSettings(const NtpSettings &ntpSettings)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject ntpData;
    ntpData["server_1"] = ntpSettings.servers.size() > 0 ? ntpSettings.servers.at(0) : "";
    ntpData["server_2"] = ntpSettings.servers.size() > 1 ? ntpSettings.servers.at(1) : "";
    ntpData["server_3"] = ntpSettings.servers.size() > 2 ? ntpSettings.servers.at(2) : "";

    QJsonArray dataArray;
    dataArray.append(ntpData);

    QJsonObject payload;
    payload["device_uuid"] = Utils::getUid();
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_ntp, jsonString);
}


void AwsMqttManager::processReboot()
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject rebootData;
    rebootData["command"] = true;
    
    QJsonArray dataArray;
    dataArray.append(rebootData);

    QJsonObject payload;
    payload["device_uuid"] = Utils::getUid();
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_reboot, jsonString);
}

