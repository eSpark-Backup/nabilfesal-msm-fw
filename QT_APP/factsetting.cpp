#include "factsetting.h"
#include "ui_factsetting.h"
#include "rebootdialog.h"
#include "utils.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QScreen>
#include "toast.h"

FactSetting::FactSetting(QWidget *parent)
    : settingsManager(parent)
    , ui(new Ui::FactSetting)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    auto firmware = Utils::getFirmwareVersion();
    ui->ota_cur_version->setText(firmware);
    auto otaInitiated = Utils::getLastOTAInitiated();

    if (!otaInitiated.isEmpty()) {
        QDateTime dateTime = QDateTime::fromString(otaInitiated, Qt::ISODate);
        if (dateTime.isValid()) {
            ui->ota_last_date->setText(dateTime.date().toString("yyyy-MM-dd"));
            ui->ota_last_time->setText(dateTime.time().toString("HH:mm:ss"));
        } else {
            ui->ota_last_date->setText("N/A");
            ui->ota_last_time->setText("N/A");
        }
    } else {
        ui->ota_last_date->setText("N/A");
        ui->ota_last_time->setText("N/A");
    }

    auto versionList = Utils::getAvailableVersions();
    ui->comboBox->clear();

    for (const QString &version : versionList) {
        QString cleanVersion = version;
        if (cleanVersion.startsWith("MSM_"))
            cleanVersion.remove(0, 4);
        
        ui->comboBox->addItem(cleanVersion);
    }

    QLabel* label = new QLabel("MQTT_IP", ui->mqtt_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(260, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_PASSWORD", ui->mqtt_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(195, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_USER", ui->mqtt_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(237, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_TOPIC", ui->mqtt_topic);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(230, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtt_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(100, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->mqtt_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(60, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_IP", ui->sftp_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(285, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(260, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(260, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PASSWORD", ui->sftp_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(220, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->sftp_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(60, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SCHEDULE(24hr)", ui->sftp_sch);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QFont font = label->font();
    font.setPointSize(5);
    label->setFont(font);
    label->adjustSize(); 
    label->move(50, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_ENDPOINT", ui->mqtts_endpoint);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(215, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_PORT", ui->mqtts_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(240, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_INTERVAL", ui->mqtts_interval);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(220, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("CURRENT VERSION", ui->ota_cur_version);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("LAST UPDATE DATE", ui->ota_last_date);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("LAST UPDATE TIME", ui->ota_last_time);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 120);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->mqtt_ip, ui->mqtt_port, ui->mqtt_user,
        ui->mqtt_pass, ui->mqtt_topic, ui->mqtt_int,
        ui->sftp_ip, ui->sftp_port, ui->sftp_user,
        ui->sftp_pass, ui->sftp_int, ui->sftp_sch,
        ui->mqtts_interval, ui->mqtts_endpoint
    };

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);

        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            edit->clearFocus();
            keyboard->hide();
        });
    }
}

void FactSetting::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ui->exit_btn->setFocusPolicy(Qt::NoFocus);
    setFocus(Qt::OtherFocusReason);
    if (keyboard && keyboard->isVisible()) {
        keyboard->hide();
    }
}

bool FactSetting::eventFilter(QObject *obj, QEvent *event)
{
    // Then: handle QLineEdit focus/keyboard logic
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
    if (lineEdit) {
        if (event->type() == QEvent::FocusIn) {
            keyboard->setTarget(lineEdit);
            Utils::keySound();
            keyboard->show();
            lineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                keyboard->hide();
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

FactSetting::~FactSetting()
{
    delete ui;
}

void FactSetting::on_next_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void FactSetting::on_fact_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void FactSetting::on_exp_btn_clicked()
{
    emit usbExport();
}


void FactSetting::on_exit_btn_clicked()
{
    emit factoryExit();
}


void FactSetting::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}


void FactSetting::on_mqtt_load_btn_clicked(bool msgBox)
{
    load_mqtt_settings(msgBox);
}


void FactSetting::load_mqtt_settings(bool msgBox, bool reconnect)
{
    QJsonObject mqttSettings = loadMqttSettings(msgBox, reconnect);
    if (!mqttSettings.isEmpty())
    {
        ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
        ui->mqtt_user->setText(mqttSettings.value("user").toString());
        ui->mqtt_pass->setText(mqttSettings.value("password").toString());
        ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
        ui->mqtt_int->setText(mqttSettings.value("interval").toString());
        if(!msgBox) Toast::show("MQTT settings loaded successfully!", this, Toast::Info);
    }
}


void FactSetting::on_mqtt_save_btn_clicked()
{
    QString mqttIp      = ui->mqtt_ip->text().trimmed();
    QString mqttPort    = ui->mqtt_port->text().trimmed();
    QString mqttUser    = ui->mqtt_user->text().trimmed();
    QString mqttPass    = ui->mqtt_pass->text().trimmed();
    QString mqttTopic   = ui->mqtt_topic->text().trimmed();
    QString mqttInterval = ui->mqtt_int->text().trimmed();

    // 1. Create JSON object
    QJsonObject mqttSettings;
    mqttSettings["ip"]       = mqttIp;
    mqttSettings["port"]     = mqttPort;
    mqttSettings["user"]     = mqttUser;
    mqttSettings["password"] = mqttPass;
    mqttSettings["topic"]    = mqttTopic;
    mqttSettings["interval"] = mqttInterval;

    if (hasEmptyField(mqttSettings)) {
        Toast::show("Please fill in all MQTT fields before saving.", this, Toast::Warning);
        return;
    }
    saveMqttSettings(mqttSettings);
}


void FactSetting::on_sftp_load_btn_clicked(bool msgBox)
{
    load_sftp_settings(msgBox);
}


void FactSetting::load_sftp_settings(bool msgBox, bool reconnect)
{
    QJsonObject sftpSettings = loadSftpSettings(msgBox, reconnect);

    if (!sftpSettings.isEmpty())
    {
        ui->sftp_ip->setText(sftpSettings.value("ip").toString());
        ui->sftp_pass->setText(sftpSettings.value("password").toString());
        ui->sftp_int->setText(sftpSettings.value("interval").toString());
        ui->sftp_sch->setText(sftpSettings.value("schedule").toString());
        ui->sftp_user->setText(sftpSettings.value("username").toString());
        if(!msgBox) Toast::show("SFTP settings loaded successfully!", this, Toast::Info);
    }
}


void FactSetting::on_sftp_save_btn_clicked()
{
    QString sftpIp       = ui->sftp_ip->text().trimmed();
    QString sftpPort     = ui->sftp_port->text().trimmed();
    QString sftpUser     = ui->sftp_user->text().trimmed();
    QString sftpPass     = ui->sftp_pass->text().trimmed();
    QString sftpInterval = ui->sftp_int->text().trimmed();
    QString sftpSchedule = ui->sftp_sch->text().trimmed();

    // 1. Create JSON object
    QJsonObject sftpSettings;
    sftpSettings["ip"]       = sftpIp;
    sftpSettings["port"]     = sftpPort;
    sftpSettings["username"] = sftpUser;
    sftpSettings["password"] = sftpPass;
    sftpSettings["interval"] = sftpInterval;
    sftpSettings["schedule"] = sftpSchedule;

    if (hasEmptyField(sftpSettings)) {
        Toast::show("Please fill in all SFTP fields before saving.", this, Toast::Warning);
        return;
    }
    saveSftpSettings(sftpSettings);
}


void FactSetting::on_OTA_pushButton_clicked()
{
    QString selVersion = ui->comboBox->currentText();
    startOTA(selVersion);
}

void FactSetting::startOTA(const QString &selectedVersion)
{    
    if (selectedVersion.isEmpty()) {
        Toast::show("No OTA version selected.", this, Toast::Warning);
        return;
    }
    QString updatingImagePath = ":/icons/updating.png";
    updatingLabel = Utils::showImageFullscreenFor(updatingImagePath, -1);

    QString otaFileName = QString("MSM_%1.zip").arg(selectedVersion);

    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open SFTP config:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid SFTP config JSON:" << parseError.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    QString ip       = obj.value("ip").toString();
    QString username = obj.value("username").toString();
    QString password = obj.value("password").toString();

    if (ip.isEmpty() || username.isEmpty() || password.isEmpty()) {
        qWarning() << "Incomplete SFTP configuration.";
        return;
    }

    QString scriptPath = "/MSM/potential-pancake/MSM/linux_configs/misc-scripts/ota.sh";

    #if defined(BUILD_X86)
    scriptPath = "/etc/ota.sh";
    #endif

    QStringList args;
    args << otaFileName << ip << username << password;

    QProcess *process = new QProcess(this);
    process->setProgram(scriptPath);
    process->setArguments(args);

    connect(process, &QProcess::readyReadStandardOutput, [process]() {
        qDebug().noquote() << process->readAllStandardOutput();
    });

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning().noquote() << process->readAllStandardError();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);

         if (updatingLabel) {
                updatingLabel->close();   // hides and deletes the window
                delete updatingLabel;
                updatingLabel = nullptr;
            }

        if (exitCode == 0) {
            sendOTAData(exitCode);
            Utils::setLastOTAInitiated();
            Utils::rebootSystem();
        } else {

            QMessageBox::information(nullptr, "OTA Update Failed", 
                                     "The OTA update process failed. Please try again.");
        }

        process->deleteLater();
    });

    process->start();

    if (!process->waitForStarted()) {
        QMessageBox::warning(nullptr, "Error", "Failed to start OTA update script.");
        if (updatingLabel) {
            updatingLabel->close();   // hides and deletes the window
            delete updatingLabel;
            updatingLabel = nullptr;
        }
        return;
    }
}


void FactSetting::on_pushButton_5_clicked()
{
    auto versionList = Utils::getAvailableVersions();
    ui->comboBox->clear();

    for (const QString &version : versionList) {
        QString cleanVersion = version;
        if (cleanVersion.startsWith("MSM_"))
            cleanVersion.remove(0, 4);

        ui->comboBox->addItem(cleanVersion);
    }
}


void FactSetting::on_mqtts_load_btn_clicked(bool msgBox)
{
    load_mqtts_settings(msgBox);
}


void FactSetting::load_mqtts_settings(bool msgBox, bool reconnect)
{
    QString filePath = "config/aws_mqtts_settings.json";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (!msgBox)
            Toast::show("Failed to open MQTTs configuration file.", this, Toast::Warning);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (!msgBox)
            Toast::show("Invalid MQTTs configuration file format.", this, Toast::Error);
        return;
    }

    QJsonObject obj = doc.object();

    // Fill UI fields (port skipped)
    ui->mqtts_endpoint->setText(obj.value("endpoint").toString());
    ui->mqtts_endpoint->setCursorPosition(0);
    ui->mqtts_interval->setText(obj.value("interval").toString());
    // Port is constant and not editable

    // ---- Check for certificate files ----
    #if defined(BUILD_X86)
        QString basePath = QCoreApplication::applicationDirPath() + "/data/certs/";
        QString caPath   = basePath + "AmazonRootCA1.pem";
    #elif defined(BUILD_V3S)
        QString basePath = "/MSM/potential-pancake/MSM/creds/prov_certs/";
        QString caPath = "/MSM/potential-pancake/MSM/creds/claim_certs/AmazonRootCA1.pem"; //v3s
    #endif

    QString certPath = basePath + "deviceCert.pem";
    QString keyPath  = basePath + "privateKey.pem";

    checkCertificates();
            
    if (!msgBox) Toast::show("MQTT settings loaded successfully!", this, Toast::Info);
    if(reconnect) emit aws_mqtts_load();
}



void FactSetting::on_mqtts_save_btn_clicked(bool msgBox)
{
    QString filePath = "config/aws_mqtts_settings.json";

    // Ensure directory exists
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QFile file(filePath);
    QJsonObject obj;

    // Read existing JSON if available
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject())
            obj = doc.object();
    }

    // Update relevant fields
    obj["endpoint"] = ui->mqtts_endpoint->text();
    obj["interval"] = ui->mqtts_interval->text();

    // Keep or restore other fields
    if (!obj.contains("port"))
        obj["port"] = "8883";

    if (hasEmptyField(obj)) {
        Toast::show("Please fill in all MQTTS fields before saving.", this, Toast::Warning);
        return;
    }

    if (!jsonDifferent(obj, filePath)) {
        if (!msgBox) Toast::show("[MQTTS] Settings unchanged, not saving into file.", this, Toast::Warning);
        return;
    }

    // Write updated JSON back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (!msgBox) Toast::show("Failed to save MQTT configuration.", this, Toast::Warning);
        return;
    }

    QJsonDocument newDoc(obj);
    file.write(newDoc.toJson(QJsonDocument::Indented));
    file.close();

    if (!msgBox) Toast::show("MQTTS settings saved successfully!", this, Toast::Info);
    emit aws_mqtts_load();
}

void FactSetting::on_pushButton_3_clicked()
{
    QString endpoint = ui->mqtts_endpoint->text();
    if (endpoint.length() < 5) {
        Toast::show("[Invalid Input] Endpoint must be at least 5 characters long.", this, Toast::Error);
        return;
    }
    ui->pushButton_3->setEnabled(false);
    emit runFleetProvisioning();
}


void FactSetting::runFleetProvisionerAsync()
{
    QString mqttsEp = ui->mqtts_endpoint->text();
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
                     [this, process, logBuffer](int exitCode, QProcess::ExitStatus exitStatus) {
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
        ui->pushButton_3->setEnabled(true);
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
        ui->pushButton_3->setEnabled(true);
        delete logBuffer;
        process->deleteLater();
    }
}


void FactSetting::on_pushButton_2_clicked()
{
    Toast::show("[MQTTS] Starting Fetching Claim credentials", this, Toast::Info);
    ui->pushButton_2->setEnabled(false);
    QString claimDir = "/MSM/potential-pancake/MSM/creds/";
#if defined(BUILD_X86)
    claimDir = QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/certificates/";
#endif
    QString configPath = "config/sftp_settings.json";

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Toast::show("Failed to open SFTP config file.", this, Toast::Warning);
        qWarning() << "Failed to open SFTP config:" << file.errorString();
        ui->pushButton_2->setEnabled(true);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        Toast::show("Invalid SFTP configuration file format.", this, Toast::Warning);
        ui->pushButton_2->setEnabled(true);
        return;
    }

    QJsonObject obj = doc.object();
    QString ip       = obj.value("ip").toString();
    QString username = obj.value("username").toString();
    QString password = obj.value("password").toString();
    QString port     = obj.value("port").toString();

    if (ip.isEmpty() || username.isEmpty() || password.isEmpty() || port.isEmpty()) {
        Toast::show("Incomplete SFTP configuration.", this, Toast::Warning);
        ui->pushButton_2->setEnabled(true);
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
            this, [this, process](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);
        if (exitCode == 0) {
            Toast::show("Claim certificates downloaded successfully!", this, Toast::Info);
            checkCertificates();
        } else {
            Toast::show("Failed to download claim certificates. Please check SFTP connection or credentials.", this, Toast::Error);
        }
        ui->pushButton_2->setEnabled(true);
        process->deleteLater();
    });

    process->start(sshpassPath, args);

    if (!process->waitForStarted()) {
        Toast::show("Failed to start SFTP download process.", this, Toast::Warning);
        process->deleteLater();
        ui->pushButton_2->setEnabled(true);
        return;
    }
}

void FactSetting::checkCertificates()
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

    ui->ca1->setChecked(QFile::exists(caPath));
    ui->claim->setChecked(QFile::exists(certPath));
    ui->key->setChecked(QFile::exists(keyPath));
    ui->devCertChkBox->setChecked(QFile::exists(prvCertPath));
    ui->devKeyChkBox->setChecked(QFile::exists(prvkeyPath));
}


void FactSetting::sendOTAData(int status)
{
    QString firmware = Utils::getFirmwareVersion();
    QString otaInitiated = Utils::getLastOTAInitiated();
    QJsonObject json;
    json["current_version"] = firmware;

    QJsonObject lastUpdate;
    if (!otaInitiated.isEmpty()) {
        QDateTime dateTime = QDateTime::fromString(otaInitiated, Qt::ISODate);
        if (dateTime.isValid()) {
            lastUpdate["date"] = dateTime.date().toString("yyyy-MM-dd");
            lastUpdate["time"] = dateTime.time().toString("HH:mm:ss");
        } else {
            lastUpdate["date"] = "N/A";
            lastUpdate["time"] = "N/A";
        }
    } else {
        lastUpdate["date"] = "N/A";
        lastUpdate["time"] = "N/A";
    }

    json["last_update"] = lastUpdate;

    emit OTA_status(json);
    QString jsonString = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    qDebug().noquote() << "OTA JSON:" << QJsonDocument(json).toJson(QJsonDocument::Indented);

}

