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
    isConnected  = false;   // add as member variable (bool)

    // Timer: publish telemetry if connected
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "Skipped publish, client not connected.";
            return;
        }
        QByteArray awsPayload = buildAwsTelemetryJson();
        AWS_LOG("\n========================================================================");
        AWS_LOG("AwsTelemetryJson: " << awsPayload);
        AWS_LOG("\n========================================================================");
        publishMessage(awsPayload);
    });

    // Connected signal
    connect(awsMqttsClient, &QMqttClient::connected, this, [this]() {
        isConnected = true;
        qDebug() << "Connected to AWS IoT Core!";

        if (intervalSec > 0) {
            publishTimer->start(intervalSec*1000);
        }
    });

    // Disconnected signal
    connect(awsMqttsClient, &QMqttClient::disconnected, this, [this]() {
        isConnected = false;
        qDebug() << "Disconnected from AWS IoT Core.";
        publishTimer->stop();
    });
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
    host = obj.value("endpoint").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    // clientId = Utils::getUid();
    topic = obj.value("topic").toString();
    QString intervalSecStr = obj.value("interval").toString();
    intervalSec = intervalSecStr.toInt();

    // Log the values
    AWS_LOG("AWS MQTTS Settings Loaded:");
    AWS_LOG("  Host:" << host);
    AWS_LOG("  Port:" << port);
    AWS_LOG("  Client ID:" << clientId);
    AWS_LOG("  Topic:" << topic);
    AWS_LOG("  Interval (sec):" << intervalSec);

    return true;
}

void AwsMqttManager::reloadSettings()
{
    configPath = AWS_MQTTS_PATH;
    if (!loadConfig(configPath)) {
        qWarning() << "No valid config found. Skipping AWS MQTTS connection.";
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
    if (host.isEmpty() || clientId.isEmpty() || topic.isEmpty()) {
        qWarning() << "[MQTT] Settings incomplete.";
        return;
    }

    if (!loadCertificates()) {
        qWarning() << "[MQTT] Missing certificates. Cannot connect.";
        return;
    }
    awsMqttsClient->setClientId(clientId);
    awsMqttsClient->setCleanSession(true);
    awsMqttsClient->setKeepAlive(30);
    sslSocket->connectToHostEncrypted(host, port);
    AWS_LOG("[TLS] Initiating encrypted connect to" << host << ":" << port);
}


void AwsMqttManager::disconnectClient()
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

void AwsMqttManager::publishMessage(const QString &payload)
{
    if (awsMqttsClient->state() == QMqttClient::Connected) {
        auto id = awsMqttsClient->publish(topic, payload.toUtf8(), 1);
        if (id == -1)
            qWarning() << "Publish failed!";
    }
}

void AwsMqttManager::subscribeToTopic()
{
    if (awsMqttsClient->state() != QMqttClient::Connected)
        return;

    auto sub = awsMqttsClient->subscribe(topic, 1);
    if (!sub) {
        qWarning() << "Subscribe failed!";
        return;
    }

    connect(sub, &QMqttSubscription::messageReceived, this,
            [](const QMqttMessage &msg) {
                AWS_LOG( "Message received:" << msg.payload());
            });
}


QByteArray AwsMqttManager::buildAwsTelemetryJson() const
{
    QJsonObject root;

    // ISO8601 timestamp with offset
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["device_uuid"] = Utils::getUid();

    QJsonArray channelsArray;

    QReadLocker locker(&telemetryLock);
    for (auto it = channelTelemetry.begin(); it != channelTelemetry.end(); ++it) {
        channelsArray.append(it.value());
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
