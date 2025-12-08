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
#include "toast.h"

DeviceSettings::DeviceSettings(QWidget *parent)
    : settingsManager(parent)
    , ui(new Ui::DeviceSettings)
{
    ui->setupUi(this);
    setUiPtr(this);
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
    ui->mqtts->setText(Utils::getMqttsStatus());

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
    label->move(270, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 2", ui->server2);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(270, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 3", ui->server3);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();;
    label->move(270, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_IP", ui->mqtt_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(265, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_PASSWORD", ui->mqtt_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_USER", ui->mqtt_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(250, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_TOPIC", ui->mqtt_topic);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(245, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtt_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(115, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->mqtt_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(65, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_IP", ui->sftp_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(275, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(250, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_username);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(250, 1);
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
    label->move(72, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SCHEDULE(24hr)", ui->sftp_sch);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QFont font = label->font();
    font.setPointSize(6);
    label->setFont(font);
    label->adjustSize(); 
    label->move(45, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_ENDPOINT", ui->mqtts_endpoint);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(205, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("ID", ui->mqtts_client_id);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(190, 3);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtts_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(80, 3);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_INTERVAL", ui->mqtts_interval);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 120);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->server1, ui->server2, ui->server3,
        ui->mqtt_ip, ui->mqtt_port, ui->mqtt_user,
        ui->mqtt_pass, ui->mqtt_topic, ui->mqtt_int,
        ui->sftp_ip, ui->sftp_port, ui->sftp_username,
        ui->sftp_pass, ui->sftp_int, ui->sftp_sch,
        ui->mqtts_interval,  ui->mqtts_endpoint
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
    ui->gsm->setText(signal);
    QString date   = root.value("date").toString();
    QString time   = root.value("time").toString();
    ui->date_input->setText(date);
    ui->time_input->setText(time);
    QString ntpStatus = root.value("ntp_status").toString();
    ui->ntp->setText(ntpStatus);
}

void DeviceSettings::on_continue_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void DeviceSettings::on_dev_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
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
        if(msgBox) Toast::show("Please enter at least one NTP server.", this, Toast::Warning);
        return false;
    }
    
    bool success = false;
    if (newNtp != previousNtp) {
        success = Utils::setNtpServers(newNtp.servers);
        if (success) {
            if(msgBox) {
                Toast::show("NTP servers saved successfully.\nService restarted.", this, Toast::Info);
                emit ntpSettingsChanged(newNtp);
            }
            previousNtp = newNtp;
        } else {
            if(msgBox) Toast::show("Failed to save NTP servers.Make sure the app has root permissions.", this, Toast::Warning);
        }
    } else {
        if (msgBox) Toast::show("No NTP changes detected, settings are already up-to-date.", this, Toast::Info);
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

    if (hasEmptyField(mqttSettings)) {
        Toast::show("Please fill in all MQTT fields before saving.", this, Toast::Warning);
        return;
    }
    saveMqttSettings(mqttSettings);
}


void DeviceSettings::on_mqtt_load_btn_clicked(bool msgBox)
{
   load_mqtt_settings(msgBox);
}


void DeviceSettings::load_mqtt_settings(bool msgBox, bool reconnect)
{
    QJsonObject mqttSettings = loadMqttSettings(msgBox, reconnect);
    if (!mqttSettings.isEmpty()) apply_mqtt_Settings(mqttSettings, msgBox);
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

    saveMqttSettings(mqttSettings);
    apply_mqtt_Settings(mqttSettings);
}


void DeviceSettings::apply_mqtt_Settings(const QJsonObject &mqttSettings, bool msgBox)
{
    ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
    ui->mqtt_user->setText(mqttSettings.value("user").toString());
    ui->mqtt_port->setText(mqttSettings.value("port").toString());
    ui->mqtt_pass->setText(mqttSettings.value("password").toString());
    ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
    ui->mqtt_int->setText(mqttSettings.value("interval").toString());
    
    if(!msgBox)
        Toast::show("MQTT settings loaded successfully!", this, Toast::Info);
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

    if (hasEmptyField(sftpSettings)) {
        Toast::show("Please fill in all SFTP fields before saving.", this, Toast::Warning);
        return;
    }
    saveSftpSettings(sftpSettings);
}


void DeviceSettings::on_sftp_load_btn_clicked(bool msgBox)
{
    load_sftp_settings(msgBox);
}


void DeviceSettings::load_sftp_settings(bool msgBox, bool reconnect)
{
    QJsonObject sftpSettings = loadSftpSettings(msgBox, reconnect);
    if (!sftpSettings.isEmpty()) apply_sftp_Settings(sftpSettings, msgBox);
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
    
    saveSftpSettings(sftpSettings);
    apply_sftp_Settings(sftpSettings);
}


void DeviceSettings::apply_sftp_Settings(const QJsonObject &sftpSettings, bool msgBox)
{
    ui->sftp_ip->setText(sftpSettings.value("ip").toString());
    ui->sftp_username->setText(sftpSettings.value("username").toString());
    ui->sftp_pass->setText(sftpSettings.value("password").toString());
    ui->sftp_int->setText(sftpSettings.value("interval").toString());
    ui->sftp_sch->setText(sftpSettings.value("schedule").toString());
    
    if(!msgBox)
        Toast::show("SFTP settings loaded successfully!", this, Toast::Info);
}


void DeviceSettings::refreshInfo()
{
    curDevInfo.uuid       = Utils::getUid();
    curDevInfo.pass       = Utils::getDevPassword();
    curDevInfo.ip         = Utils::getDeviceIp();
    
    QString newImei = Utils::getDeviceImei();
    if (newImei != "UNKNOWN" && !newImei.isEmpty()) {
        curDevInfo.imei = newImei;
    } else {
        curDevInfo.imei = prvDevInfo.imei;
    }

    auto ntpData = Utils::getNtpStatus();
    curDevInfo.date       = ntpData.value("date").toString();
    curDevInfo.gsmStatus  = Utils::getSignalCategoryFromFile();

    curDevInfo.mqttStatus = Utils::getMqttStatus();
    curDevInfo.sftpStatus = Utils::getSftpStatus();
    curDevInfo.mqttsStatus =Utils::getMqttsStatus();
    curDevInfo.ntpStatus = ntpData.value("ntp_status").toString();

    ui->uuid->setText(curDevInfo.uuid);
    ui->ip->setText(curDevInfo.ip);
    ui->imei->setText(curDevInfo.imei);
    ui->date_input->setText(curDevInfo.date);
    ui->gsm->setText(curDevInfo.gsmStatus);
    ui->mqtt->setText(curDevInfo.mqttStatus);
    ui->sftp->setText(curDevInfo.sftpStatus);
    ui->mqtts->setText(curDevInfo.mqttsStatus);
    ui->ntp->setText(curDevInfo.ntpStatus);

    if (isDifferent(curDevInfo, prvDevInfo)) {
        emit deviceInfoChanged(curDevInfo);
        prvDevInfo = curDevInfo;
    }
    if(refreshTimer != nullptr) refreshTimer->start(10 * 1000);
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
        || a.sftpStatus != b.sftpStatus
        || a.mqttsStatus != b.mqttsStatus
        || a.gsmStatus != b.gsmStatus;
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

    QJsonObject mqttsSettings = loadMqttsSettings(true, false);
    if (!mqttsSettings.isEmpty())
        emit mqttsSettingsChanged(mqttsSettings);

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


void DeviceSettings::on_pushButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void DeviceSettings::on_pushButton_2_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}


void DeviceSettings::on_mqtts_load_btn_clicked(bool msgBox)
{
    QJsonObject mqttsSettings = loadMqttsSettings(msgBox);
    apply_mqtts_Settings(mqttsSettings, msgBox);
}


void DeviceSettings::parse_load_store_mqtts(const QJsonObject &payload)
{
    QMetaObject::invokeMethod(this, [this, payload]() {
        QJsonArray dataArray = payload.value("data").toArray();
        if (dataArray.isEmpty()) {
            qWarning() << "[MQTTS] 'data' array is incomplete!";
            return;
        }

        QJsonObject mqttsPayload = dataArray.at(0).toObject();
        QJsonObject mqttsSetting;
        mqttsSetting["endpoint"] = mqttsPayload.value("broker_ip").toString();
        mqttsSetting["port"]     = QString::number(mqttsPayload.value("broker_port").toInt());
        mqttsSetting["interval"] = QString::number(mqttsPayload.value("data_interval").toInt());

        saveMqttsSettings(mqttsSetting);
        apply_mqtts_Settings(mqttsSetting);
    }, Qt::QueuedConnection);
}


void DeviceSettings::apply_mqtts_Settings(const QJsonObject &mqttsSettings, bool msgBox)
{
    ui->mqtts_port->setText(mqttsSettings.value("port").toString());
    ui->mqtts_endpoint->setText(mqttsSettings.value("endpoint").toString());
    ui->mqtts_interval->setText(mqttsSettings.value("interval").toString());
    ui->mqtts_endpoint->setCursorPosition(0);
    ui->mqtts_client_id->setText(Utils::getUid());
    
    if(!msgBox)
        Toast::show("MQTTS settings loaded successfully!", this, Toast::Info);
}


void DeviceSettings::load_mqtts_settings(bool msgBox, bool reconnect)
{
    QJsonObject mqttsSettings = loadMqttsSettings(msgBox, reconnect);
    if (!mqttsSettings.isEmpty()) apply_mqtts_Settings(mqttsSettings, msgBox);
}


void DeviceSettings::on_mqtts_save_btn_clicked(bool msgBox)
{
    QString mqttEndpoint = ui->mqtts_endpoint->text().trimmed();
    QString mqttPort     = ui->mqtts_port->text().trimmed();
    QString mqttInterval = ui->mqtts_interval->text().trimmed();

    if (mqttPort != "8883") mqttPort = "8883";  //not editable

    QJsonObject mqttsSettings;
    mqttsSettings["endpoint"] = mqttEndpoint;
    mqttsSettings["port"]     = mqttPort;
    mqttsSettings["interval"] = mqttInterval;

    if (hasEmptyField(mqttsSettings)) {
        Toast::show("Please fill in all MQTTS fields before saving.", this, Toast::Warning);
        return;
    }
    saveMqttsSettings(mqttsSettings, true);
}


void DeviceSettings::on_resetEnergy_Btn_clicked()
{
    ui->resetEnergy_Btn->clearFocus();

    int ret = QMessageBox::question(
        this, "Reset Energy", "Do you want to reset energy?",QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) emit resetEnergy();
}

