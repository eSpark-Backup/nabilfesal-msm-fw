#include "awsmqttmanager.h"
#include <QFile>
#include "utils.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtNetwork/QSslKey>
#include "encryptionhelper.h"
#include <QProcessEnvironment>
#include <QtNetwork/QSslSocket>
#include <QtMqtt/QMqttSubscription>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>


#define AWS_MQTTS_PATH aws_path
std::array<EnergyData, 13> AwsMqttManager::awsEnergy = {};

#define AWS_LOG(msg) \
    do { \
        if (AwsMqttManager::debugEnabled()) { logDebug << msg; } \
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


AwsMqttManager::AwsMqttManager(Mode mode, QObject *parent) : clientMode(mode), QObject(parent)
{
    if(clientMode == Mode::Factory){
        tag = TAG_FACTORY;
        aws_path = "config/aws_mqtts_settings.json";
    }else{
        tag = TAG_ADMIN;
        caPath        = CERT_DIR_PATH + "admin/claim_certs/AmazonRootCA1.pem";
        claimCertPath = CERT_DIR_PATH + "admin/claim_certs/claim_cert.pem";
        claimKeyPath  = CERT_DIR_PATH + "admin/claim_certs/claim_private.key";
        provCertPath  = CERT_DIR_PATH + "admin/prov_certs/deviceCert.pem";
        provKeyPath   = CERT_DIR_PATH + "admin/prov_certs/privateKey.pem";
    }

    awsMqttsClient = new QMqttClient(this);
    publishTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);

    if (!loadAwsConfig(AWS_MQTTS_PATH)) {
        logWarn << "No valid config found. Skipping AWS MQTTS connection.";
    }

    reconnectTimer->setInterval(5 * 1000);  // 5 seconds
    reconnectTimer->setSingleShot(false);    // repeat until success

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected && !isConnecting) {
            reloadAwsMqttsSettings();
        }
    });

    // MQTT client state logging
    connect(awsMqttsClient, &QMqttClient::stateChanged, this, [this](QMqttClient::ClientState state) {
        AWS_LOG("[MQTTS] Client state:" << state);
    });

    connect(awsMqttsClient, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError err) {
        logWarn << "[MQTTS] Error:" << err;
    });

    // Timer: publish telemetry if connected
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            logWarn << "Skipped publish, client not connected.";
            return;
        }

        publishTelemetry();
    });

    connect(awsMqttsClient, &QMqttClient::connected, this, [this]() {
    isConnected  = true;
    isConnecting = false;
    logDebug << "[MQTTS] Connected to AWS IoT Core!";
    reconnectTimer->stop();

    if (intervalSec > 0) {
        Utils::startAlignedTimer( this,
            publishTimer, intervalSec,
            [this]() {if (isConnected) publishTelemetry();});
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
        isConnected  = false;
        isConnecting = false;
        logWarn << "[MQTTS] Disconnected from AWS IoT Core.";
        publishTimer->stop();

        if (!reconnectTimer->isActive()) {
            reconnectTimer->start();
            logWarn << "[MQTTS] Reconnection scheduled";
        }
    });

    if (!reconnectTimer->isActive()) reconnectTimer->start();
}

bool AwsMqttManager::loadAwsConfig(const QString &filePath)
{
    configPath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        logWarn << "Config file not found:" << filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logWarn << "Failed to open MQTTS config file";
        return false;
    }

    EncryptionHelper::check_and_encrypt_files(filePath, "MQTTS", false, nullptr);

    QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(filePath, Utils::getEncryptionKey());
    if (decryptedData.isEmpty()) {
        logWarn << "Failed to decrypt MQTTS settings (wrong password or corrupted file)";
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(decryptedData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        logWarn << "Invalid MQTTS config file format.";
        return false;
    }

    QJsonObject obj = doc.object();
    host = obj.value("endpoint").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    clientId = Utils::getUid();
    clientId += ((clientMode == Mode::Factory) ? "_factory" : "");
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
        logWarn << "No valid config found. Skipping AWS MQTTS connection.";
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
    if (sslSocket) {
        sslSocket->disconnect(this);
        sslSocket->deleteLater();
        sslSocket = nullptr;
    }

    if (!QSslSocket::supportsSsl()) {
        logWarn << "[AWS TLS] SSL/TLS not supported on this system!";
        return false;
    }

    EncryptionHelper::check_and_encrypt_files(provCertPath, "Load Device Cert", true, nullptr);
    EncryptionHelper::check_and_encrypt_files(provKeyPath, "Load Device Key", true, nullptr);

    QFile caFile(caPath), certFile(provCertPath), keyFile(provKeyPath);
    if (!caFile.open(QIODevice::ReadOnly) ||
        !certFile.open(QIODevice::ReadOnly) ||
        !keyFile.open(QIODevice::ReadOnly)) {
        logWarn << "[AWS TLS] Failed to open cert/key files";
        return false;
    }

    QByteArray certDecryptedData = EncryptionHelper::readAndDecryptFile(provCertPath, Utils::getEncryptionKey());
    if (certDecryptedData.isEmpty()) {
        logWarn << "Failed to decrypt Device Cert File settings (wrong password or corrupted file)";
        return false;
    }

    QByteArray KeyDecryptedData = EncryptionHelper::readAndDecryptFile(provKeyPath, Utils::getEncryptionKey());
    if (KeyDecryptedData.isEmpty()) {
        logWarn << "Failed to decrypt Device Key File settings (wrong password or corrupted file)";
        return false;
    }

    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setProtocol(QSsl::TlsV1_2OrLater);
    conf.setCaCertificates(QSslCertificate::fromData(caFile.readAll(), QSsl::Pem));
    conf.setLocalCertificate(QSslCertificate(certDecryptedData, QSsl::Pem));
    conf.setPrivateKey(QSslKey(KeyDecryptedData, QSsl::Rsa, QSsl::Pem));

    if (conf.localCertificate().isNull() || conf.privateKey().isNull() || conf.caCertificates().isEmpty()) {
        logWarn << "[AWS TLS] Invalid certificate/key/CA chain";
        return false;
    }

    sslSocket = new QSslSocket(this);
    sslSocket->setSslConfiguration(conf);
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslSocket->setPeerVerifyName(host);

    // TLS signals
    connect(sslSocket, &QSslSocket::encrypted, this, [this]() {
        AWS_LOG("[AWS TLS] Handshake complete (encrypted). Connecting MQTT client...");
        awsMqttsClient->connectToHost();
    });

    connect(sslSocket,
            static_cast<void(QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors),
            this,
            [this](const QList<QSslError> &errs) {
                for (const auto &e : errs)
                    logWarn << "[TLS] SSL error:" << e.errorString();
            });

    connect(sslSocket, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError err) {
        logWarn << "[TLS] Socket error:" << err;
        isConnected  = false;
        isConnecting = false;
    });

    awsMqttsClient->setTransport(sslSocket, QMqttClient::IODevice);
    return true;
}

void AwsMqttManager::connectToBroker()
{
    if (isConnected || isConnecting) {
        return;
    }

    isConnecting = true;

    if (!sslSocket) {
        if (!loadCertificates()) {
            logWarn << "[AWS MQTTS] TLS setup failed";
            isConnecting = false;
            return;
        }
    }

    awsMqttsClient->setClientId(clientId);
    awsMqttsClient->setCleanSession(true);
    awsMqttsClient->setKeepAlive(30);

    AWS_LOG("[TLS] Initiating encrypted connect to " << host << ":" << port);
    sslSocket->connectToHostEncrypted(host, port);
}


void AwsMqttManager::disconnectAwsClient()
{
    disconnectFromBroker();
}


void AwsMqttManager::disconnectFromBroker()
{
    if (!awsMqttsClient) return;

    AWS_LOG("[MQTTS] Disconnecting...");

    publishTimer->stop();
   
    if (awsMqttsClient->state() != QMqttClient::Disconnected)
        awsMqttsClient->disconnectFromHost();

    if (sslSocket) {
        sslSocket->disconnectFromHost();
        sslSocket->close();
    }
}


void AwsMqttManager::publishMessage(const QString pubTopic, const QString &payload)
{
    if (awsMqttsClient->state() == QMqttClient::Connected) {
        auto id = awsMqttsClient->publish(pubTopic, payload.toUtf8(), 1);
        if (id == -1)
            logWarn << "Publish on [" << pubTopic << "] failed!";
    }
}


void AwsMqttManager::subscribeToTopic()
{
    QStringList topics = {
        topic_sub_reboot,
        topic_sub_ntp,
        topic_sub_mqtt,
        topic_sub_mqtts,
        topic_sub_sftp
    };

    if (clientMode == Mode::Factory) {
        topics << topic_sub_ctRatio
               << topic_sub_vOffset
               << topic_sub_iOffset
               << topic_sub_ota;
    }

    for (const QString &t : topics) {
        auto *sub = awsMqttsClient->subscribe(t, 1);
        if (!sub) {
            logWarn << "[MQTT] Subscribe failed for topic:" << t;
            continue;
        }

    connect(sub, &QMqttSubscription::messageReceived, this,
        [this](const QMqttMessage &msg) {
            const QString topic = msg.topic().name();
            QByteArray rawPayload = msg.payload();
            QJsonDocument doc = QJsonDocument::fromJson(rawPayload);
            QJsonObject obj;
            if (!doc.isNull() && doc.isObject()) {
                QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
                obj = doc.object();
                logDebug << "\n[" << timestamp << "] [AWS] Topic:" << topic
                         << "Message received: Parsed JSON:\n"
                         << QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
            }
            emit settingsChanged();

            if (topic == topic_sub_reboot) {
                emit rebootRequested();
            } else if (topic == topic_sub_ntp) {
                emit ntpConfigReceived(obj);
            } else if (topic == topic_sub_mqtt) {
                emit mqttConfigReceived(obj);
            } else if (topic == topic_sub_mqtts) {
                emit mqttsConfigReceived(obj);
            } else if (topic == topic_sub_sftp) {
                emit sftpConfigReceived(obj);
            } else if (topic == topic_sub_ctRatio) {
                emit ctRatioReceived(obj);
            } else if (topic == topic_sub_vOffset) {
                emit vOffsetReceived(obj);
            } else if (topic == topic_sub_iOffset) {
                emit iOffsetReceived(obj);
            } else if (topic == topic_sub_ota) {
                otaSend(obj);
            } else {
                logWarn << "[MQTT] Unknown topic:" << topic;
            }
        });
    }
}


void AwsMqttManager::awsTopicsInit()
{
    QString base_pub_topic = "msm/" + clientId + "/pub"; //clientId to be replaced
    QString base_sub_topic = "msm/" + clientId + "/sub";
    
    topic_pub_telemetry   = base_pub_topic + "/telemetry";
    topic_pub_device_info = base_pub_topic + "/device_info";
    topic_pub_reboot      = base_pub_topic + "/reboot";
    topic_pub_ntp         = base_pub_topic + "/ntp";
    topic_pub_mqtt        = base_pub_topic + "/mqtt";
    topic_pub_mqtts       = base_pub_topic + "/mqtts";
    topic_pub_sftp        = base_pub_topic + "/sftp";

    topic_sub_reboot      = base_sub_topic + "/reboot";
    topic_sub_ntp         = base_sub_topic + "/ntp";
    topic_sub_mqtt        = base_sub_topic + "/mqtt";
    topic_sub_mqtts       = base_sub_topic + "/mqtts";
    topic_sub_sftp        = base_sub_topic + "/sftp";

    if (clientMode == Mode::Factory)
    {
        topic_pub_ctRatio  = base_pub_topic + "/ct_ratio";
        topic_pub_vOffset  = base_pub_topic + "/v_offset";
        topic_pub_iOffset  = base_pub_topic + "/i_offset";
        topic_pub_ota      = base_pub_topic + "/ota_info";

        topic_sub_ctRatio  = base_sub_topic + "/ct_ratio";
        topic_sub_vOffset  = base_sub_topic + "/v_offset";
        topic_sub_iOffset  = base_sub_topic + "/i_offset";
        topic_sub_ota      = base_sub_topic + "/ota_trigger";
    }
}


QByteArray AwsMqttManager::buildAwsTelemetryJson(int startIndex, int endIndex) const
{
    QJsonObject root;

    // ISO8601 timestamp with offset
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["device_uuid"] = clientId;

    QJsonArray channelsArray;

    QReadLocker locker(&telemetryLock);
    int i = 0;
    for (auto it = channelTelemetry.begin(); it != channelTelemetry.end(); ++it, ++i) {
        if (i >= startIndex && i < endIndex) {
            channelsArray.append(it.value());
        }
    }
    locker.unlock();
    if (channelsArray.isEmpty())    return {};
    root["channels"] = channelsArray;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}


void AwsMqttManager::publishTelemetry()
{
    int totalChannels = channelTelemetry.size();
    int mid = totalChannels / 2;

    QByteArray payload1 = buildAwsTelemetryJson(0, mid);
    if (!payload1.isEmpty())
        publishMessage(topic_pub_telemetry, payload1);

    QByteArray payload2 = buildAwsTelemetryJson(mid, totalChannels);
    if (!payload2.isEmpty())
        publishMessage(topic_pub_telemetry, payload2);
}


void AwsMqttManager::processAwsTelemetry(int channel, const QJsonObject &payload)
{
    QJsonObject cleanPayload = payload;
    QJsonObject dataObj = cleanPayload["data"].toObject();

    for (const QString &phase : {"phase_a", "phase_b", "phase_c"}) {
        int phaseNum = (phase == "phase_a") ? 0 : (phase == "phase_b") ? 1 : 2;
        if (dataObj.contains(phase)) {
            QJsonObject phaseObj = dataObj[phase].toObject();
            phaseObj.remove("avg_power");
            phaseObj.remove("avg_energy");
            phaseObj.remove("thd");

             // Get energy object
            QJsonObject energyObj = phaseObj["energy"].toObject();
            QJsonObject active   = energyObj["active"].toObject();
            QJsonObject reactive = energyObj["reactive"].toObject();

            if(channel >= 1 && channel <= 12)
            {
                QMutexLocker locker(&mqttsMutex);
                // Accumulate into class variables
                awsEnergy[channel].posActEnergy[phaseNum] += active["positive"].toDouble();
                awsEnergy[channel].negActEnergy[phaseNum] += active["negative"].toDouble();
                awsEnergy[channel].posReacEnergy[phaseNum] += reactive["positive"].toDouble();
                awsEnergy[channel].negReacEnergy[phaseNum] += reactive["negative"].toDouble();

                // Update JSON with accumulated totals
                active["positive"] = awsEnergy[channel].posActEnergy[phaseNum];
                active["negative"] = awsEnergy[channel].negActEnergy[phaseNum];
                reactive["positive"] = awsEnergy[channel].posReacEnergy[phaseNum];
                reactive["negative"] = awsEnergy[channel].negReacEnergy[phaseNum];

                energyObj["active"] = active;
                energyObj["reactive"] = reactive;
                phaseObj["energy"] = energyObj;
            }
            
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
    deviceInfoData["mqtt_status"] = info.conn.mqttStatus == "Connected" ? true : false;
    deviceInfoData["sftp_status"] = info.conn.sftpStatus == "Connected" ? true : false;
    deviceInfoData["ntp_status"] = info.ntpStatus;
    deviceInfoData["gsm_status"] = info.gsmStatus;
    deviceInfoData["Firmware Version"] = Utils::getFirmwareVersion();
    deviceInfoData["Hardware Version"] = Utils::getHardwareVersion();


    QJsonArray dataArray;
    dataArray.append(deviceInfoData);

    QJsonObject deviceInfoPayload;
    deviceInfoPayload["data"] = dataArray;
    deviceInfoPayload["device_uuid"] = clientId;

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
    payload["device_uuid"] = clientId;
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
    payload["device_uuid"] = clientId;
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
    payload["device_uuid"] = clientId;
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_ntp, jsonString);
}


void AwsMqttManager::processMqttsSettings(const QJsonObject &mqttsSetting)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject credentials;
    credentials["root_ca"]    = fileExists(caPath);
    credentials["claim_cert"] = fileExists(claimCertPath);
    credentials["claim_key"]  = fileExists(claimKeyPath);

    QJsonObject awsData;
    awsData["broker_ip"]     = mqttsSetting.value("endpoint").toString();
    awsData["broker_port"]   = mqttsSetting.value("port").toString().toInt();
    awsData["data_interval"] = mqttsSetting.value("interval").toString().toInt();
    awsData["credentials"]   = credentials;

    QJsonArray dataArray;
    dataArray.append(awsData);

    QJsonObject payload;
    payload["device_uuid"] = clientId;
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_mqtts, jsonString);
}


bool AwsMqttManager::fileExists(const QString &path)
{
    return QFile::exists(path);
}


void AwsMqttManager::processReboot()
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject rebootData;
    rebootData["command"] = true;
    
    QJsonArray dataArray;
    dataArray.append(rebootData);

    QJsonObject payload;
    payload["device_uuid"] = clientId;
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_reboot, jsonString);
}


void AwsMqttManager::processCtRatio(const QJsonObject &ctRatio)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject payload;
    payload["device_uuid"] = clientId;
    QJsonArray dataArray;
    dataArray.append(ctRatio);
    payload["data"] = dataArray;
    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_ctRatio, jsonString);
}


void AwsMqttManager::processVOffset(const QJsonObject &vOffset)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;
    
    QJsonObject payload;
    payload["device_uuid"] = clientId;
    QJsonArray dataArray;
    dataArray.append(vOffset);
    payload["data"] = dataArray;
    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_vOffset, jsonString);
}


void AwsMqttManager::processIOffset(const QJsonObject &iOffset)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject payload;
    payload["device_uuid"] = clientId;
    QJsonArray dataArray;
    dataArray.append(iOffset);
    payload["data"] = dataArray;
    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_iOffset, jsonString);
}


void AwsMqttManager::processOTA(const QJsonObject &otaData)
{
    if (awsMqttsClient->state() == QMqttClient::Disconnected) return;

    QJsonObject payload;
    payload["device_uuid"] = clientId;
    QJsonArray dataArray;
    dataArray.append(otaData);
    payload["data"] = dataArray;

    QJsonDocument doc(payload);
    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    publishMessage(topic_pub_ota, jsonString);
}


void AwsMqttManager::otaSend(const QJsonObject &obj)
{
    QJsonArray dataArray = obj.value("data").toArray();
    if (!dataArray.isEmpty()) {
        QJsonObject firstObj = dataArray.first().toObject();
        QString otaVersion = firstObj.value("ota_version").toString();
        emit otaReceived(otaVersion);
    } else {
        logWarn << "[MQTT] OTA message received but 'data' array is empty.";
    }
}

void AwsMqttManager::resetAccumulatedEnergy()
{
    for(int ch = 1; ch <= 12; ++ch) {
        for(int ph = 0; ph < 3; ph++)
        {
            awsEnergy[ch].posActEnergy[ph] = 0.0;
            awsEnergy[ch].negActEnergy[ph] = 0.0;
            awsEnergy[ch].posReacEnergy[ph] = 0.0;
            awsEnergy[ch].negReacEnergy[ph] = 0.0;
        }
    }
}

void AwsMqttManager::updateAccumulatedEnergy(const std::array<EnergyData,13> &data)
{
    QMutexLocker locker(&mqttsMutex);
    awsEnergy = data;
}
