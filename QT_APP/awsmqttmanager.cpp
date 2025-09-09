#include "awsmqttmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QSslSocket>
#include <QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>

AwsMqttManager::AwsMqttManager(QObject *parent) : QObject(parent)
{
    mqttClient = new QMqttClient(this);
    publishTimer = new QTimer(this);

    connect(mqttClient, &QMqttClient::connected, []() {
        qDebug() << "Connected to AWS IoT Core!";
    });

    connect(mqttClient, &QMqttClient::disconnected, []() {
        qDebug() << "Disconnected from AWS IoT.";
    });

    connect(mqttClient, &QMqttClient::stateChanged, [](QMqttClient::ClientState state) {
        qDebug() << "MQTT state changed:" << state;
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
    host = obj.value("ip").toString();
    port = obj.value("port").toInt(8883);
    clientId = obj.value("clientId").toString();
    topic = obj.value("topic").toString();
    intervalMs = obj.value("interval").toInt(0);

    return true;
}

void AwsMqttManager::reloadSettings()
{
    disconnectFromBroker();
    if (!loadConfig(configPath)) {
        qWarning() << "No valid config found. Skipping MQTT connection.";
        return;
    }
    connectToBroker();
}

bool AwsMqttManager::loadCertificates()
{
    QFile caFile("/etc/ssl/certs/AmazonRootCA1.pem");
    QFile certFile("/etc/ssl/certs/deviceCert.crt");
    QFile keyFile("/etc/ssl/certs/privateKey.key");

    if (!caFile.open(QIODevice::ReadOnly) ||
        !certFile.open(QIODevice::ReadOnly) ||
        !keyFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open cert files";
        return false;
    }

    QSslConfiguration sslConf;
    sslConf.addCaCertificate(QSslCertificate(&caFile));
    sslConf.setLocalCertificate(QSslCertificate(&certFile));
    sslConf.setPrivateKey(QSslKey(&keyFile, QSsl::Rsa, QSsl::Pem));

    // secure socket
    QSslSocket *sslSocket = new QSslSocket(this);
    sslSocket->setSslConfiguration(sslConf);

    // bind socket to mqtt client
    mqttClient->setTransport(sslSocket, QMqttClient::IODevice);

    return true;
}

void AwsMqttManager::connectToBroker()
{
    if (host.isEmpty() || clientId.isEmpty() || topic.isEmpty()) {
        qWarning() << "MQTT settings incomplete.";
        return;
    }

    if (!loadCertificates()) {
        qWarning() << "Missing certificates. Cannot connect.";
        return;
    }

    mqttClient->setHostname(host);
    mqttClient->setPort(port);
    mqttClient->setClientId(clientId);

    // Use the transport socket set in loadCertificates()
    mqttClient->connectToHost();

    if (intervalMs > 0) {
        publishTimer->start(intervalMs);
    }
}

void AwsMqttManager::disconnectFromBroker()
{
    if (mqttClient->state() == QMqttClient::Connected)
        mqttClient->disconnectFromHost();

    publishTimer->stop();
}

void AwsMqttManager::publishMessage(const QString &payload)
{
    if (mqttClient->state() == QMqttClient::Connected) {
        auto id = mqttClient->publish(topic, payload.toUtf8(), 1);
        if (id == -1)
            qWarning() << "Publish failed!";
    }
}

void AwsMqttManager::subscribeToTopic()
{
    if (mqttClient->state() != QMqttClient::Connected)
        return;

    auto sub = mqttClient->subscribe(topic, 1);
    if (!sub) {
        qWarning() << "Subscribe failed!";
        return;
    }

    connect(sub, &QMqttSubscription::messageReceived, this,
            [](const QMqttMessage &msg) {
                qDebug() << "Message received:" << msg.payload();
            });
}
