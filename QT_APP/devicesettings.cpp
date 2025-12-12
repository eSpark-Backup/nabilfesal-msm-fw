#include "devicesettings.h"
#include "ui_devicesettings.h"
#include "utils.h"
#include <QJsonDocument>
#include <QJsonObject>
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
    setCertPath(CERT_DIR_PATH + "admin/claim_certs/", CERT_DIR_PATH + "admin/prov_certs/");
    auto uid = Utils::getUid();
    ui->uuid->setText(uid);
    auto currentIp = Utils::getDeviceIp();
    ui->ip->setText(currentIp);
    auto currentImei = Utils::getDeviceImei();
    ui->imei->setText(currentImei);
    auto ntpData = Utils::getNtpStatus();
    parseNtpJson(ntpData);
    auto mqttData = Utils::getMqttStatus(Mode::Admin);
    ui->mqtt->setText(mqttData);
    auto sftpData = Utils::getSftpStatus(Mode::Admin);
    ui->sftp->setText(sftpData);
    ui->mqtts->setText(Utils::getMqttsStatus(Mode::Admin));

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [=]() {
        QString time = QTime::currentTime().toString("HH:mm:ss");
        ui->time_input->setText(time);
    });
    timer->start(1000);

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

    label = new QLabel("SFTP_USER", ui->sftp_user);
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

    label = new QLabel("INTERVAL(s)", ui->sftp_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(60, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SCHEDULE(24hr)", ui->sftp_sch);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize(); 
    label->move(60, 1);
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
    label->move(180, 3);
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
        ui->sftp_ip, ui->sftp_port, ui->sftp_user,
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
    QJsonObject mqttSettings = getMqttSettings();
    if(!mqttSettings.isEmpty()) saveMqttSettings(mqttSettings);
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


void DeviceSettings::on_sftp_save_btn_clicked()
{
    QJsonObject sftpSettings = getSftpSettings();
    if(!sftpSettings.isEmpty()) saveSftpSettings(sftpSettings);
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


void DeviceSettings::load_mqtts_settings(bool msgBox, bool reconnect)
{
    QJsonObject mqttsSettings = loadMqttsSettings(msgBox, reconnect);
    if (!mqttsSettings.isEmpty()) apply_mqtts_Settings(mqttsSettings, msgBox);
}


void DeviceSettings::on_mqtts_save_btn_clicked(bool msgBox)
{
    QJsonObject mqttsSettings = getMqttsSettings();
    if(!mqttsSettings.isEmpty()) saveMqttsSettings(mqttsSettings, true);
}


void DeviceSettings::on_resetEnergy_Btn_clicked()
{
    ui->resetEnergy_Btn->clearFocus();

    int ret = QMessageBox::question(
        this, "Reset Energy", "Do you want to reset energy?",QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) emit resetEnergy();
}


QString DeviceSettings::configJsonPath() const
{
#if defined(BUILD_X86)
    return QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/admin_config.json";
#elif defined(BUILD_V3S)
    return "/MSM/potential-pancake/MSM/bin/fleet_provisioner/admin_config.json";
#endif
}


void DeviceSettings::on_fetch_claim_btn_clicked()
{
    fetch_claim_certs();
}


void DeviceSettings::on_provision_btn_clicked()
{
    QString endpoint = ui->mqtts_endpoint->text();
    if (endpoint.length() < 5) {
        Toast::show("[Invalid Input] Endpoint must be at least 5 characters long.", this, Toast::Error);
        return;
    }
    ui->provision_btn->setEnabled(false);
    emit runFleetProvisioning();
}


void DeviceSettings::updateInfo(const DeviceInfo &a)
{
    ui->ip->setText(a.ip);
    ui->gsm->setText(a.gsmStatus);
    ui->ntp->setText(a.ntpStatus);
    ui->uuid->setText(a.uuid);
    ui->imei->setText(a.imei);
    ui->date_input->setText(a.date);
    ui->sftp->setText(a.conn.mqttStatus);
    ui->mqtt->setText(a.conn.sftpStatus);
    ui->mqtts->setText(a.conn.mqttsStatus);
}
