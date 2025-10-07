#include "devicesettings.h"
#include "ui_devicesettings.h"
#include "utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QKeyEvent>


DeviceSettings::DeviceSettings(QWidget *parent)
    : settingsManager(parent)
    , ui(new Ui::DeviceSettings)
{
    ui->setupUi(this);
    auto uid = Utils::getUid();
    ui->uuid->setText(uid);
    auto currentIp = Utils::getDeviceIp();
    ui->ip->setText(currentIp);
    auto currentImei = Utils::getDeviceImei();
    ui->imei->setText(currentImei);
    auto ntpData = Utils::getNtpStatus();
    parseNtpJson(ntpData);
    auto mqttData = Utils::getMqttStatus();
    ui->mqtt->setText(mqttData);
    auto sftpData = Utils::getSftpStatus();
    ui->sftp->setText(sftpData);

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [=]() {
        QString time = QTime::currentTime().toString("HH:mm:ss");
        ui->time_input->setText(time);
    });
    timer->start(1000);

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &DeviceSettings::refreshInfo);
    refreshTimer->start(10 * 1000); // every 10 seconds

    ui->stackedWidget->setCurrentIndex(0);
    QLabel* label = new QLabel("server 1", ui->server1);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 2", ui->server2);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 3", ui->server3);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();;
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_IP", ui->mqtt_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(215, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_PASSWORD", ui->mqtt_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(150, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_USER", ui->mqtt_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(190, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_TOPIC", ui->mqtt_topic);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(185, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtt_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(80, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->mqtt_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(45, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_IP", ui->sftp_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(215, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_username);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PASSWORD", ui->sftp_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(150, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->sftp_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(42, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SCHEDULE", ui->sftp_sch);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(55, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 120);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->server1, ui->server2, ui->server3,
        ui->mqtt_ip, ui->mqtt_port, ui->mqtt_user,
        ui->mqtt_pass, ui->mqtt_topic, ui->mqtt_int,
        ui->sftp_ip, ui->sftp_port, ui->sftp_username,
        ui->sftp_pass, ui->sftp_int, ui->sftp_sch
    };

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);

        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            edit->clearFocus();
            keyboard->hide();
        });
    }
    this->installEventFilter(this);
    on_ntp_load_btn_clicked();
}

DeviceSettings::~DeviceSettings()
{
    delete ui;
}

bool DeviceSettings::eventFilter(QObject *obj, QEvent *event)
{
    // First: handle key events globally
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Home) {
            on_channels_btn_clicked();
            return true;  // stop further processing
        }
    }

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


void DeviceSettings::on_channels_btn_clicked()
{
    emit channels_view();
}

void DeviceSettings::parseNtpJson(const QJsonObject& root)
{
    QString signal = Utils::getSignalCategoryFromFile();
    ui->ntp->setText(signal);
    QString date   = root.value("date").toString();
    QString time   = root.value("time").toString();
    ui->date_input->setText(date);
    ui->time_input->setText(time);
}

void DeviceSettings::on_continue_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void DeviceSettings::on_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void DeviceSettings::on_ntp_load_btn_clicked()
{
    auto ntpData = Utils::getNtpStatus();
    QString date   = ntpData.value("date").toString();
    ui->date_input->setText(date);

    // Extract servers
    if (ntpData.contains("ntp_servers") && ntpData["ntp_servers"].isArray()) {
        QJsonArray servers = ntpData["ntp_servers"].toArray();
        ui->server1->setText(servers[0].toString());
        ui->server2->setText(servers[1].toString());
        ui->server3->setText(servers[2].toString());
    }

}


void DeviceSettings::on_ntp_save_btn_clicked()
{
    server1 = ui->server1->text().trimmed();
    server2 = ui->server2->text().trimmed();
    server3 = ui->server3->text().trimmed();

    apply_ntp_Settings();
}


void DeviceSettings::parse_load_store_ntp(const QJsonObject &payload)
{
    QJsonArray dataArray = payload.value("data").toArray();
    if (dataArray.isEmpty()) {
        qWarning() << "[NTP] 'data' array is incomplete";
        return;
    }

    QJsonObject ntpPayload = dataArray.at(0).toObject();
    server1 = ntpPayload.value("server_1").toString();
    server2 = ntpPayload.value("server_2").toString();
    server3 = ntpPayload.value("server_3").toString();

    if(apply_ntp_Settings(true))
    {   
        ui->server1->setText(server1);
        ui->server2->setText(server2);
        ui->server3->setText(server3);
    }
}


bool DeviceSettings::apply_ntp_Settings(bool msgBox)
{
    newNtp.servers.clear();
    if (!server1.isEmpty()) newNtp.servers << server1;
    if (!server2.isEmpty()) newNtp.servers << server2;
    if (!server3.isEmpty()) newNtp.servers << server3;

    if (newNtp.servers.isEmpty()) {
        if(msgBox) QMessageBox::warning(this, "NTP Settings", "Please enter at least one NTP server.");
        return false;
    }
    
    bool success = false;
    if (newNtp != previousNtp) {
        success = Utils::setNtpServers(newNtp.servers);
        if (success) {
            if(msgBox) {
                QMessageBox::information(this, "NTP Settings", "NTP servers saved successfully.\nService restarted.");
                emit ntpSettingsChanged(newNtp);
            }
            previousNtp = newNtp;
        } else {
            if(msgBox) QMessageBox::critical(this, "NTP Settings", "Failed to save NTP servers.\nMake sure the app has root permissions.");
        }
    } else {
        if (msgBox) QMessageBox::information(this, "NTP Settings", "No changes detected, settings are already up-to-date.");
        qInfo() << "[NTP] No changes detected, skipping save.";
    }
    return success;
}


void DeviceSettings::on_mqtt_save_btn_clicked()
{
    QString mqttIp      = ui->mqtt_ip->text().trimmed();
    QString mqttPort    = ui->mqtt_port->text().trimmed();
    QString mqttUser    = ui->mqtt_user->text().trimmed();
    QString mqttPass    = ui->mqtt_pass->text().trimmed();
    QString mqttTopic   = ui->mqtt_topic->text().trimmed();
    QString mqttInterval = ui->mqtt_int->text().trimmed();

    QJsonObject mqttSettings;
    mqttSettings["ip"]       = mqttIp;
    mqttSettings["port"]     = mqttPort;
    mqttSettings["user"]     = mqttUser;
    mqttSettings["password"] = mqttPass;
    mqttSettings["topic"]    = mqttTopic;
    mqttSettings["interval"] = mqttInterval;
    saveMqttSettings(mqttSettings);
}


void DeviceSettings::on_mqtt_load_btn_clicked(bool msgBox)
{
    QJsonObject mqttSettings = loadMqttSettings(msgBox);

    if (!mqttSettings.isEmpty())
    {
        apply_mqtt_Settings(mqttSettings, msgBox);
    }
}


void DeviceSettings::parse_load_store_mqtt(const QJsonObject &payload)
{
    QJsonArray dataArray = payload.value("data").toArray();
    if (dataArray.isEmpty()) {
        qWarning() << "[MQTT] 'data' array is incomplete!";
        return;
    }

    QJsonObject mqttPayload = dataArray.at(0).toObject();
    QJsonObject mqttSettings;
    mqttSettings["ip"]       = mqttPayload.value("broker_ip").toString();
    mqttSettings["port"]     = QString::number(mqttPayload.value("broker_port").toInt());
    mqttSettings["user"]     = mqttPayload.value("broker_user").toString();
    mqttSettings["password"] = mqttPayload.value("broker_pass").toString();
    mqttSettings["topic"]    = mqttPayload.value("mqtt_topic").toString();
    mqttSettings["interval"] = QString::number(mqttPayload.value("data_interval").toInt());

    saveMqttSettings(mqttSettings, false);
    apply_mqtt_Settings(mqttSettings);
}


void DeviceSettings::apply_mqtt_Settings(const QJsonObject &mqttSettings, bool msgBox)
{
    ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
    ui->mqtt_port->setText(mqttSettings.value("port").toString());
    ui->mqtt_user->setText(mqttSettings.value("user").toString());
    ui->mqtt_pass->setText(mqttSettings.value("password").toString());
    ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
    ui->mqtt_int->setText(mqttSettings.value("interval").toString());
    
    if(msgBox)
        QMessageBox::information(this, "MQTT Settings", "MQTT settings loaded successfully!");
}


void DeviceSettings::on_sftp_save_btn_clicked()
{
    QString sftpIp       = ui->sftp_ip->text().trimmed();
    QString sftpPort     = ui->sftp_port->text().trimmed();
    QString sftpUser     = ui->sftp_username->text().trimmed();
    QString sftpPass     = ui->sftp_pass->text().trimmed();
    QString sftpInterval = ui->sftp_int->text().trimmed();
    QString sftpSchedule = ui->sftp_sch->text().trimmed();

    QJsonObject sftpSettings;
    sftpSettings["ip"]       = sftpIp;
    sftpSettings["port"]     = sftpPort;
    sftpSettings["username"] = sftpUser;
    sftpSettings["password"] = sftpPass;
    sftpSettings["interval"] = sftpInterval;
    sftpSettings["schedule"] = sftpSchedule;
    saveSftpSettings(sftpSettings);
}


void DeviceSettings::on_sftp_load_btn_clicked(bool msgBox)
{
    QJsonObject sftpSettings = loadSftpSettings(msgBox);

    if (!sftpSettings.isEmpty())
    {
        apply_sftp_Settings(sftpSettings, msgBox);
    }
}


void DeviceSettings::parse_load_store_sftp(const QJsonObject &payload)
{
    QJsonArray dataArray = payload.value("data").toArray();
    if (dataArray.isEmpty()) {
        qWarning() << "[SFTP] 'data' array is incomplete";
        return;
    }

    QJsonObject sftpPayload = dataArray.at(0).toObject();
    QJsonObject sftpSettings;
    sftpSettings["ip"]       = sftpPayload.value("server_ip").toString();
    sftpSettings["port"]     = QString::number(sftpPayload.value("server_port").toInt());
    sftpSettings["username"] = sftpPayload.value("username").toString();
    sftpSettings["password"] = sftpPayload.value("password").toString();
    sftpSettings["interval"] = QString::number(sftpPayload.value("data_interval").toInt());
    sftpSettings["schedule"] = ui->sftp_sch->text().trimmed();
    
    saveSftpSettings(sftpSettings, false);
    apply_sftp_Settings(sftpSettings);
}


void DeviceSettings::apply_sftp_Settings(const QJsonObject &sftpSettings, bool msgBox)
{
    ui->sftp_ip->setText(sftpSettings.value("ip").toString());
    ui->sftp_port->setText(sftpSettings.value("port").toString());
    ui->sftp_username->setText(sftpSettings.value("username").toString());
    ui->sftp_pass->setText(sftpSettings.value("password").toString());
    ui->sftp_int->setText(sftpSettings.value("interval").toString());
    ui->sftp_sch->setText(sftpSettings.value("schedule").toString());
    
    if(msgBox)
        QMessageBox::information(this, "SFTP Settings", "SFTP settings loaded successfully!");
}


void DeviceSettings::loadAwsMqttsSettings(bool showMessages)
{
    QString certs_path;
    #if defined(BUILD_X86)
        certs_path = QCoreApplication::applicationDirPath() + "/data/certs/";
    #elif defined(BUILD_V3S)
        certs_path = "/MSM/potential-pancake/MSM/creds/prov_certs/";
    #endif

    QString certPath = certs_path + "deviceCert.pem";
    QString keyPath  = certs_path + "privateKey.pem";

    if ((!QFile::exists(certPath) || !QFile::exists(keyPath)) && !Utils::getUid().isEmpty()) {
        qDebug() << "One or more certificate files are missing!";
        QFile::remove(certPath); QFile::remove(keyPath);
        
        if(Utils::updateSerialInConfig())
        {
            runFleetProvisionerAsync();
        }
    }
    else
        emit aws_mqtts_load();

    // file("config/aws_mqtts_settings.json");
    // if (!file.exists()) {
    //     if (showMessages)
    //         QMessageBox::warning(this, "AWS MQTTS Settings", "No saved settings found.");
    //     return;
    // }

    // if (!file.open(QIODevice::ReadOnly)) {
    //     if (showMessages)
    //         QMessageBox::critical(this, "AWS MQTTS Settings", "Failed to open settings file.");
    //     return;
    // }

    // QByteArray data = file.readAll();
    // file.close();

    // QJsonParseError parseError;
    // QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    // if (doc.isNull() || !doc.isObject()) {
    //     if (showMessages)
    //         QMessageBox::critical(this, "AWS MQTTS Settings", "Invalid JSON format: " + parseError.errorString());
    //     return;
    // }

    // QJsonObject mqttSettings = doc.object();
    // emit aws_mqtts_load();
}


void DeviceSettings::runFleetProvisionerAsync()
{
    QString exe_path;
    #if defined(BUILD_X86)
        exe_path = "/home/nehalespark/Downloads/aws-iot-device-sdk-embedded-C/build/bin";
    #elif defined(BUILD_V3S)
        exe_path = "/MSM/potential-pancake/MSM/bin/fleet_provisioner";
    #endif

    QString program = exe_path + "/msm_fleet_provisioner";

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setWorkingDirectory(exe_path);

    QString *logBuffer = new QString();

    QObject::connect(process, &QProcess::errorOccurred, [process](QProcess::ProcessError err) {
        qWarning() << "[Fleet ERROR] QProcess error:" << err
                   << "Details:" << process->errorString();
    });

    QObject::connect(process, &QProcess::readyReadStandardOutput, [process, logBuffer]() {
        QByteArray output = process->readAllStandardOutput();
        logBuffer->append(QString::fromUtf8(output));
        qDebug().noquote() << "[Fleet STDOUT]" << output;
    });

    QObject::connect(process, &QProcess::readyReadStandardError, [process]() {
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
            emit aws_mqtts_load();
        } else {
            qWarning() << "[Fleet] Provisioning failed.";
        }

        delete logBuffer;
        process->deleteLater();
    });
    qDebug() << "Process start called here";
    process->start();

    // 🔹 immediate check
    if (!process->waitForStarted(3000)) {
        qWarning() << "[Fleet ERROR] Failed to start process:" << program
                   << "Reason:" << process->errorString();
        delete logBuffer;
        process->deleteLater();
    }
}



void DeviceSettings::refreshInfo()
{
    curDevInfo.uuid       = Utils::getUid();
    curDevInfo.pass       = Utils::getDevPassword();
    curDevInfo.ip         = Utils::getDeviceIp();
    curDevInfo.imei       = Utils::getDeviceImei();

    auto ntpData = Utils::getNtpStatus();
    curDevInfo.date       = ntpData.value("date").toString();
    curDevInfo.ntpStatus  = Utils::getSignalCategoryFromFile();

    curDevInfo.mqttStatus = Utils::getMqttStatus();
    curDevInfo.sftpStatus = Utils::getSftpStatus();

    ui->uuid->setText(curDevInfo.uuid);
    ui->ip->setText(curDevInfo.ip);
    ui->imei->setText(curDevInfo.imei);
    ui->date_input->setText(curDevInfo.date);
    ui->ntp->setText(curDevInfo.ntpStatus);
    ui->mqtt->setText(curDevInfo.mqttStatus);
    ui->sftp->setText(curDevInfo.sftpStatus);

    if (isDifferent(curDevInfo, prvDevInfo)) {
        emit deviceInfoChanged(curDevInfo);
        prvDevInfo = curDevInfo;
    }
}


bool DeviceSettings::isDifferent(const DeviceInfo &a, const DeviceInfo &b)
{
    return a.uuid       != b.uuid
        || a.pass       != b.pass
        || a.ip         != b.ip
        || a.imei       != b.imei
        || a.date       != b.date
        || a.ntpStatus  != b.ntpStatus
        || a.mqttStatus != b.mqttStatus
        || a.sftpStatus != b.sftpStatus;
}


void DeviceSettings::on_export_btn_clicked()
{
    emit usbExport();
}


void DeviceSettings::send_aws_updated_data()
{
    QJsonObject mqttSettings = loadMqttSettings(false, false);
    if (!mqttSettings.isEmpty())
        emit mqttSettingsChanged(mqttSettings);

    QJsonObject sftpSettings = loadSftpSettings(false, false);
    if (!sftpSettings.isEmpty())
        emit sftpSettingsChanged(sftpSettings);

    NtpSettings ntp;
    auto ntpData = Utils::getNtpStatus();
    if (ntpData.contains("ntp_servers") && ntpData["ntp_servers"].isArray()) {
        QJsonArray serverArray = ntpData["ntp_servers"].toArray();
        for (const auto &val : serverArray) {
        QString server = val.toString().trimmed();
        if (!server.isEmpty())
            ntp.servers.append(server);
    }

    if (!ntp.servers.isEmpty())
        emit ntpSettingsChanged(ntp);
    }
}

