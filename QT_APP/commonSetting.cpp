#include "commonSetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include "toast.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>


QJsonObject settingsManager::loadMqttSettings(bool showMessages, bool reloadSettings)
{
    QFile file(mqttConfigPath());
    if (!file.exists()) {
        if(!showMessages) Toast::show("No MQTT settings Found!", this, Toast::Warning);
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (!showMessages) Toast::show("Failed to open MQTT setting file!", this, Toast::Error);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (!showMessages) Toast::show("[MQTT] Invalid JSON format: " + parseError.errorString(), this, Toast::Error);
        return {};
    }

    QJsonObject mqttSettings = doc.object();
    if(reloadSettings) emit mqtt_load();
    return mqttSettings;
}


void settingsManager::saveMqttSettings(QJsonObject mqttSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(mqttSetting, mqttConfigPath());
    if (!isJsonDiff) {
        if (showMessages) Toast::show("[MQTT] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }

    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(mqttConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if(showMessages) Toast::show("Failed to save MQTT settings!", this, Toast::Error);
        return;
    }

    QJsonDocument doc(mqttSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if(showMessages) 
    {
        Toast::show("MQTT settings saved successfully!", this, Toast::Info);
        emit mqttSettingsChanged(mqttSetting);
    }
    emit mqtt_load();
}


void settingsManager::saveSftpSettings(QJsonObject sftpSetting, bool showMessages)
{
    bool isJsonDiff = jsonDifferent(sftpSetting, sftpConfigPath());
    if (!isJsonDiff) {
        if (showMessages) Toast::show("[SFTP] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }
    
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/factory_sftp_settings.json
    QFile file(sftpConfigPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (showMessages) Toast::show("Failed to save SFTP settings!", this, Toast::Error);
        return;
    }

    QJsonDocument doc(sftpSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (showMessages) {
        Toast::show("SFTP settings saved successfully!", this, Toast::Info);
        emit sftpSettingsChanged(sftpSetting);
    }
    emit sftp_load();    
}


QJsonObject settingsManager::loadSftpSettings(bool showMessages, bool reloadSettings)
{
    QFile file(sftpConfigPath());
    if (!file.exists()) {
        if (!showMessages) Toast::show("No SFTP settings found!", this, Toast::Warning);
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (!showMessages) Toast::show("Failed to open SFTP setting file!", this, Toast::Error);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        if (!showMessages) Toast::show("[SFTP] Invalid JSON format: " + parseError.errorString(), this, Toast::Error);
        return {};
    }

    QJsonObject sftpSettings = doc.object();

    if(reloadSettings) emit sftp_load();
    return sftpSettings;
}


QJsonObject settingsManager::loadMqttsSettings(bool showMessages, bool reloadSettings)
{
    QString filePath = mqttsConfigPath();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (!showMessages)
            Toast::show("Failed to open MQTTS configuration file.", this, Toast::Error);
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (!showMessages)
            Toast::show("Invalid MQTTS configuration file format.", this, Toast::Warning);
        return {};
    }

    QJsonObject obj = doc.object();
    if(reloadSettings) emit aws_mqtts_load();
    return obj;
}


void settingsManager::saveMqttsSettings(QJsonObject mqttsSetting, bool showMessages)
{
    QString MQTTS_CONFIG_PATH = mqttsConfigPath();
    bool isJsonDiff = jsonDifferent(mqttsSetting, MQTTS_CONFIG_PATH);
    if (!isJsonDiff) {
        if (showMessages) Toast::show("[MQTTS] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }

    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(MQTTS_CONFIG_PATH);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if(showMessages) Toast::show("Failed to save MQTTS settings!", this, Toast::Error);
        return;
    }

    QJsonDocument doc(mqttsSetting);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if(showMessages) 
    {
        Toast::show("MQTTS settings saved successfully!", this, Toast::Info);
        emit mqttsSettingsChanged(mqttsSetting);
    }
    emit aws_mqtts_load();
}


bool settingsManager::jsonDifferent(const QJsonObject &a, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return true; // return empty object if not found
    }

    QJsonDocument old = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject b = old.object();

    if (a.keys() != b.keys())
        return true;

    for (const QString &key : a.keys()) {
        if (a.value(key) != b.value(key)) {
            return true; // mismatch found
        }
    }
    return false; // all fields same
}

bool settingsManager::hasEmptyField(const QJsonObject &obj) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.value().toString().trimmed().isEmpty())
            return true;
    }
    return false;
}


void settingsManager::runFleetProvisionerAsync()
{
    auto mqtts_endpoint = uiBase->findChild<QLineEdit*>("mqtts_endpoint");
    auto provision_btn = uiBase->findChild<QPushButton*>("provision_btn");
    QString mqttsEp = mqtts_endpoint->text();
    Utils::updateSerialInConfig(mqttsEp);
    QString exe_path;
    #if defined(BUILD_X86)
        exe_path = QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin";
    #elif defined(BUILD_V3S)
        exe_path = "/MSM/potential-pancake/MSM/bin/fleet_provisioner";
    #endif

    QString program = exe_path + "/msm_fleet_provisioner";

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setWorkingDirectory(exe_path);

    QString *logBuffer = new QString();

    QObject::connect(process, &QProcess::errorOccurred, [this, process](QProcess::ProcessError err) {
        qWarning() << "[Fleet ERROR] QProcess error:" << err
                   << "Details:" << process->errorString();
    });

    QObject::connect(process, &QProcess::readyReadStandardOutput, [this, process, logBuffer]() {
        QByteArray output = process->readAllStandardOutput();
        logBuffer->append(QString::fromUtf8(output));
        qDebug().noquote() << "[Fleet STDOUT]" << output;
    });

    QObject::connect(process, &QProcess::readyReadStandardError, [this, process]() {
        QByteArray error = process->readAllStandardError();
        qWarning().noquote() << "[Fleet STDERR]" << error;
    });

    QObject::connect(process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [this, process, logBuffer, provision_btn](int exitCode, QProcess::ExitStatus exitStatus) {
        bool exitedNormally = (exitStatus == QProcess::NormalExit && exitCode == 0);
        bool demoSuccess = logBuffer->contains("Demo completed successfully.");

        qDebug() << "[Fleet] Finished with code:" << exitCode
                 << "status:" << (exitStatus == QProcess::NormalExit ? "Normal" : "Crashed")
                 << "success:" << (demoSuccess && exitedNormally);

        if (demoSuccess && exitedNormally) {
            Toast::show("[MQTTS] Fleet Provisioning Success!", this, Toast::Info);
            checkCertificates();
            emit aws_mqtts_load();
        } else {
            qWarning() << "[Fleet] Provisioning failed.";
            Toast::show("[MQTTS] Fleet Provisioning Failed!", this, Toast::Error);
        }
        provision_btn->setEnabled(true);
        delete logBuffer;
        process->deleteLater();
    });
    Toast::show("[MQTTS] Fleet Provisioning Started", this, Toast::Info);
    process->start();

    // 🔹 immediate check
    if (!process->waitForStarted(3000)) {
        qWarning() << "[Fleet ERROR] Failed to start process:" << program
                   << "Reason:" << process->errorString();
        Toast::show("[MQTTS] Failed to start Fleet Provisioning!", this, Toast::Error);
        provision_btn->setEnabled(true);
        delete logBuffer;
        process->deleteLater();
    }
}


void settingsManager::fetch_claim_certs()
{
    auto fetch_claim_btn = uiBase->findChild<QPushButton*>("fetch_claim_btn");
    Toast::show("[MQTTS] Starting Fetching Claim credentials", this, Toast::Info);
    fetch_claim_btn->setEnabled(false);
    QString claimDir = "/MSM/potential-pancake/MSM/creds/";
#if defined(BUILD_X86)
    claimDir = QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/certificates/";
#endif
    QString configPath = "config/sftp_settings.json";

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Toast::show("Failed to open SFTP config file.", this, Toast::Warning);
        qWarning() << "Failed to open SFTP config:" << file.errorString();
        fetch_claim_btn->setEnabled(true);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        Toast::show("Invalid SFTP configuration file format.", this, Toast::Warning);
        fetch_claim_btn->setEnabled(true);
        return;
    }

    QJsonObject obj = doc.object();
    QString ip       = obj.value("ip").toString();
    QString username = obj.value("username").toString();
    QString password = obj.value("password").toString();
    QString port     = obj.value("port").toString();

    if (ip.isEmpty() || username.isEmpty() || password.isEmpty() || port.isEmpty()) {
        Toast::show("Incomplete SFTP configuration.", this, Toast::Warning);
        fetch_claim_btn->setEnabled(true);
        return;
    }

    QDir dir(claimDir);
    if (!dir.exists()) {
        dir.mkpath(claimDir);
    } else {
        QStringList files = dir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            QFile::remove(claimDir + fileName);
        }
    }

    QString remotePath = "/aws_iot_core/claim_certs/";
    QString localPath  = claimDir;

    // Use full paths to binaries
    QString sshpassPath = "/usr/bin/sshpass";
    QString scpPath     = "/usr/bin/scp";

    QStringList args = {
        "-p", password,
        scpPath,
        "-r",
        "-P", port,
        "-o", "StrictHostKeyChecking=no",
        QString("%1@%2:%3").arg(username, ip, remotePath),
        localPath
    };

    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::readyReadStandardOutput, [process]() {
        qDebug().noquote() << process->readAllStandardOutput();
    });

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning().noquote() << process->readAllStandardError();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, fetch_claim_btn](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);
        if (exitCode == 0) {
            Toast::show("Claim certificates downloaded successfully!", this, Toast::Info);
            checkCertificates();
        } else {
            Toast::show("Failed to download claim certificates. Please check SFTP connection or credentials.", this, Toast::Error);
        }
        fetch_claim_btn->setEnabled(true);
        process->deleteLater();
    });

    process->start(sshpassPath, args);

    if (!process->waitForStarted()) {
        Toast::show("Failed to start SFTP download process.", this, Toast::Warning);
        process->deleteLater();
        fetch_claim_btn->setEnabled(true);
        return;
    }
}


void settingsManager::checkCertificates()
{
    #if defined(BUILD_X86)
        QString claimBasePath = QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/certificates/claim_certs/";
        QString basePath = QCoreApplication::applicationDirPath() + "/data/certs/";
    #elif defined(BUILD_V3S)
        QString claimBasePath = "/MSM/potential-pancake/MSM/creds/claim_certs/";
        QString basePath = "/MSM/potential-pancake/MSM/creds/prov_certs/";
    #endif
    
    QString caPath      = claimBasePath + "AmazonRootCA1.pem";
    QString certPath    = claimBasePath + "claim_cert.pem";
    QString keyPath     = claimBasePath + "claim_private.key";
    QString prvCertPath = basePath + "deviceCert.pem";
    QString prvkeyPath  = basePath + "privateKey.pem";

    auto ca1        = uiBase->findChild<QCheckBox*>("ca1");
    auto claim      = uiBase->findChild<QCheckBox*>("claim");
    auto key        = uiBase->findChild<QCheckBox*>("key");
    auto devCert    = uiBase->findChild<QCheckBox*>("devCertChkBox");
    auto devKey     = uiBase->findChild<QCheckBox*>("devKeyChkBox");

    if (!ca1 || !claim || !key || !devCert || !devKey) {
        qWarning() << "UI element not found in child UI!";
        return;
    }

    ca1->setChecked(QFile::exists(caPath));
    claim->setChecked(QFile::exists(certPath));
    key->setChecked(QFile::exists(keyPath));
    devCert->setChecked(QFile::exists(prvCertPath));
    devKey->setChecked(QFile::exists(prvkeyPath));
}