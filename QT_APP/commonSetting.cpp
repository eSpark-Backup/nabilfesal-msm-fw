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
#include "devicesettings.h"
#include "factsetting.h"

QJsonObject settingsManager::getMqttSettings()
{
    auto Ip    = uiBase->findChild<QLineEdit*>("mqtt_ip"); 
    auto Int   = uiBase->findChild<QLineEdit*>("mqtt_int");
    auto Port  = uiBase->findChild<QLineEdit*>("mqtt_port");
    auto User  = uiBase->findChild<QLineEdit*>("mqtt_user");
    auto Pass  = uiBase->findChild<QLineEdit*>("mqtt_pass");
    auto Topic = uiBase->findChild<QLineEdit*>("mqtt_topic");

    QString mqttIp       = Ip->text().trimmed();
    QString mqttPort     = Port->text().trimmed();
    QString mqttUser     = User->text().trimmed();
    QString mqttPass     = Pass->text().trimmed();
    QString mqttTopic    = Topic->text().trimmed();
    QString mqttInterval = Int->text().trimmed();

    QJsonObject mqttSettings = getMqttSettings();
    mqttSettings["ip"]       = mqttIp;
    mqttSettings["port"]     = mqttPort;
    mqttSettings["user"]     = mqttUser;
    mqttSettings["password"] = mqttPass;
    mqttSettings["topic"]    = mqttTopic;
    mqttSettings["interval"] = mqttInterval;

    if (hasEmptyField(mqttSettings)) {
        Toast::show("Please fill in all MQTT fields before saving.", this, Toast::Warning);
        return {};
    }

    return mqttSettings;
}


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


void settingsManager::apply_mqtt_Settings(const QJsonObject &mqttSettings, bool msgBox)
{
    auto mqtt_ip    = uiBase->findChild<QLineEdit*>("mqtt_ip");
    auto mqtt_user  = uiBase->findChild<QLineEdit*>("mqtt_user");
    auto mqtt_port  = uiBase->findChild<QLineEdit*>("mqtt_port");
    auto mqtt_pass  = uiBase->findChild<QLineEdit*>("mqtt_pass");
    auto mqtt_int   = uiBase->findChild<QLineEdit*>("mqtt_int");
    auto mqtt_topic = uiBase->findChild<QLineEdit*>("mqtt_topic");

    mqtt_ip->setText(mqttSettings.value("ip").toString());
    mqtt_user->setText(mqttSettings.value("user").toString());
    mqtt_port->setText(mqttSettings.value("port").toString());
    mqtt_pass->setText(mqttSettings.value("password").toString());
    mqtt_topic->setText(mqttSettings.value("topic").toString());
    mqtt_int->setText(mqttSettings.value("interval").toString());
    
    if(!msgBox) Toast::show("MQTT settings loaded successfully!", this, Toast::Info);
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
    
    QDir dir("config/admin_settings/");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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

QJsonObject settingsManager::getSftpSettings()
{
    auto Ip   = uiBase->findChild<QLineEdit*>("sftp_ip"); 
    auto Int  = uiBase->findChild<QLineEdit*>("sftp_int");
    auto Sch  = uiBase->findChild<QLineEdit*>("sftp_sch");
    auto Port = uiBase->findChild<QLineEdit*>("sftp_port");
    auto User = uiBase->findChild<QLineEdit*>("sftp_user");
    auto Pass = uiBase->findChild<QLineEdit*>("sftp_pass");

    QString sftpIp       = Ip->text().trimmed();
    QString sftpPort     = Port->text().trimmed();
    QString sftpUser     = User->text().trimmed();
    QString sftpPass     = Pass->text().trimmed();
    QString sftpInterval = Int->text().trimmed();
    QString sftpSchedule = Sch->text().trimmed();

    QJsonObject sftpSettings;
    sftpSettings["ip"]       = sftpIp;
    sftpSettings["port"]     = sftpPort;
    sftpSettings["username"] = sftpUser;
    sftpSettings["password"] = sftpPass;
    sftpSettings["interval"] = sftpInterval;
    sftpSettings["schedule"] = sftpSchedule;

    if (hasEmptyField(sftpSettings)) {
        Toast::show("Please fill in all SFTP fields before saving.", this, Toast::Warning);
        return {};
    }
    
    return sftpSettings;
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


void settingsManager::apply_sftp_Settings(const QJsonObject &sftpSettings, bool msgBox)
{
    auto sftp_ip    = uiBase->findChild<QLineEdit*>("sftp_ip");
    auto sftp_user  = uiBase->findChild<QLineEdit*>("sftp_user");
    auto sftp_pass  = uiBase->findChild<QLineEdit*>("sftp_pass");
    auto sftp_int   = uiBase->findChild<QLineEdit*>("sftp_int");
    auto sftp_sch   = uiBase->findChild<QLineEdit*>("sftp_sch");

    sftp_ip->setText(sftpSettings.value("ip").toString());
    sftp_user->setText(sftpSettings.value("username").toString());
    sftp_pass->setText(sftpSettings.value("password").toString());
    sftp_int->setText(sftpSettings.value("interval").toString());
    sftp_sch->setText(sftpSettings.value("schedule").toString());
    
    if(!msgBox) Toast::show("SFTP settings loaded successfully!", this, Toast::Info);
}


QJsonObject settingsManager::getMqttsSettings()
{
    auto Port     = uiBase->findChild<QLineEdit*>("mqtts_port");
    auto Endpoint = uiBase->findChild<QLineEdit*>("mqtts_endpoint");
    auto Interval = uiBase->findChild<QLineEdit*>("mqtts_interval");

    QString mqttsPort     = Port->text().trimmed();
    QString mqttsEndpoint = Endpoint->text().trimmed();
    QString mqttsInterval = Interval->text().trimmed();

    if (mqttsPort != "8883") mqttsPort = "8883";  //not editable

    QJsonObject mqttsSettings;
    mqttsSettings["endpoint"] = mqttsEndpoint;
    mqttsSettings["port"]     = mqttsPort;
    mqttsSettings["interval"] = mqttsInterval;

    if (hasEmptyField(mqttsSettings)) {
        Toast::show("Please fill in all MQTTS fields before saving.", this, Toast::Warning);
        return {};
    }

    return mqttsSettings;
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


void settingsManager::apply_mqtts_Settings(const QJsonObject &mqttsSettings, bool msgBox)
{
    auto mqtts_port      = uiBase->findChild<QLineEdit*>("mqtts_port");
    auto mqtts_endpoint  = uiBase->findChild<QLineEdit*>("mqtts_endpoint");
    auto mqtts_interval  = uiBase->findChild<QLineEdit*>("mqtts_interval");
    auto mqtts_client_id = uiBase->findChild<QLineEdit*>("mqtts_client_id");

    mqtts_port->setText(mqttsSettings.value("port").toString());
    mqtts_endpoint->setText(mqttsSettings.value("endpoint").toString());
    mqtts_interval->setText(mqttsSettings.value("interval").toString());
    mqtts_endpoint->setCursorPosition(0);
    mqtts_client_id->setText(Utils::getUid());

    checkCertificates();
    
    if(!msgBox) Toast::show("MQTTS settings loaded successfully!", this, Toast::Info);
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
    Utils::updateSerialInConfig(mqttsEp, configJsonPath());

    QString exe_path = FLEET_PROV_SCRIPT;
    QString config_file;
    config_file = qobject_cast<DeviceSettings*>(this) ? "admin_config.json": "config.json";
    QString program = exe_path + "/msm_fleet_provisioner";
    QStringList arguments = { config_file };

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);
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
    QString claimDir = CERT_DIR_PATH;

    if (qobject_cast<DeviceSettings*>(this)) {claimDir += "admin/";}
    
    QString configPath = sftpConfigPath();
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
    QString caPath      = claimBasePath + "AmazonRootCA1.pem";
    QString certPath    = claimBasePath + "claim_cert.pem";
    QString keyPath     = claimBasePath + "claim_private.key";
    QString prvCertPath = provBasePath  + "deviceCert.pem";
    QString prvkeyPath  = provBasePath  + "privateKey.pem";

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
