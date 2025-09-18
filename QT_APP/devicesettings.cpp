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
    : QWidget(parent)
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
    label->move(230, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 2", ui->server2);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(230, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("server 3", ui->server3);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();;
    label->move(230, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_IP", ui->mqtt_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(225, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_PASSWORD", ui->mqtt_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(165, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_USER", ui->mqtt_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_TOPIC", ui->mqtt_topic);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(195, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtt_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(90, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL", ui->mqtt_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(70, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_IP", ui->sftp_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(225, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_username);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PASSWORD", ui->sftp_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(170, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_INTERVAL", ui->sftp_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(185, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 180);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->server1, ui->server2, ui->server3,
        ui->mqtt_ip, ui->mqtt_port, ui->mqtt_user,
        ui->mqtt_pass, ui->mqtt_topic, ui->mqtt_int,
        ui->sftp_ip, ui->sftp_port, ui->sftp_username,
        ui->sftp_pass, ui->sftp_int
    };

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);

        connect(edit, &QLineEdit::editingFinished, this, [this]() {
            keyboard->hide();
        });
    }
    this->installEventFilter(this);

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
    QString status = root.value("ntp_status").toString();
    ui->ntp->setText(status);
    QString date   = root.value("date").toString();
    QString time   = root.value("time").toString();
    ui->date_input->setText(date);
    ui->time_input->setText(time);
}

void DeviceSettings::on_continue_btn_clicked()
{
    load_and_start_mqtt_data();
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
    QString server1 = ui->server1->text().trimmed();
    QString server2 = ui->server2->text().trimmed();
    QString server3 = ui->server3->text().trimmed();

    QStringList servers;
    if (!server1.isEmpty()) servers << server1;
    if (!server2.isEmpty()) servers << server2;
    if (!server3.isEmpty()) servers << server3;

    if (servers.isEmpty()) {
        QMessageBox::warning(this, "NTP Settings", "Please enter at least one NTP server.");
        return;
    }

    bool success = Utils::setNtpServers(servers);

    if (success) {
        QMessageBox::information(this, "NTP Settings", "NTP servers saved successfully.\nService restarted.");
    } else {
        QMessageBox::critical(this, "NTP Settings", "Failed to save NTP servers.\nMake sure the app has root permissions.");
    }
}


void DeviceSettings::on_mqtt_save_btn_clicked()
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

    // 2. Ensure config directory exists
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/mqtt_settings.json
    QFile file("config/mqtt_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "MQTT Settings", "Failed to save MQTT settings.");
        return;
    }

    QJsonDocument doc(mqttSettings);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "MQTT Settings", "MQTT settings saved successfully!");
}

void DeviceSettings::load_and_start_mqtt_data()
{
    // qInfo() << "load_and_start_mqtt_data";
    if (mqttHandler == nullptr) {
        mqttHandler = new AwsMqttManager(this);
    }

    QFile file("config/mqtt_settings.json");
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) return;
    QJsonObject mqttSettings = doc.object();

    ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
    ui->mqtt_port->setText(mqttSettings.value("port").toString());
    ui->mqtt_user->setText(mqttSettings.value("user").toString());
    ui->mqtt_pass->setText(mqttSettings.value("password").toString());
    ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
    ui->mqtt_int->setText(mqttSettings.value("interval").toString());
    
    if (!mqttHandler->mqttState())
    {
        mqttHandler->reloadSettings();
    }
}


void DeviceSettings::on_mqtt_load_btn_clicked()
{
    if (mqttHandler == nullptr) {
        mqttHandler = new AwsMqttManager(this);
    }
    mqttHandler->disconnectClient();

    QFile file("config/mqtt_settings.json");
    if (!file.exists()) {
        QMessageBox::warning(this, "MQTT Settings", "No saved settings found.");
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "MQTT Settings", "Failed to open settings file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "MQTT Settings", "Invalid JSON format: " + parseError.errorString());
        return;
    }

    QJsonObject mqttSettings = doc.object();

    ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
    ui->mqtt_port->setText(mqttSettings.value("port").toString());
    ui->mqtt_user->setText(mqttSettings.value("user").toString());
    ui->mqtt_pass->setText(mqttSettings.value("password").toString());
    ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
    ui->mqtt_int->setText(mqttSettings.value("interval").toString());

    QMessageBox::information(this, "MQTT Settings", "MQTT settings loaded successfully!");
    
    QTimer::singleShot(1000, [this](){
        mqttHandler->reloadSettings();
    });
}


void DeviceSettings::on_sftp_save_btn_clicked()
{
    QString sftpIp       = ui->sftp_ip->text().trimmed();
    QString sftpPort     = ui->sftp_port->text().trimmed();
    QString sftpUser     = ui->sftp_username->text().trimmed();
    QString sftpPass     = ui->sftp_pass->text().trimmed();
    QString sftpInterval = ui->sftp_int->text().trimmed();

    // 1. Create JSON object
    QJsonObject sftpSettings;
    sftpSettings["ip"]       = sftpIp;
    sftpSettings["port"]     = sftpPort;
    sftpSettings["username"] = sftpUser;
    sftpSettings["password"] = sftpPass;
    sftpSettings["interval"] = sftpInterval;

    // 2. Ensure config directory exists
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/sftp_settings.json
    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "SFTP Settings", "Failed to save SFTP settings.");
        return;
    }

    QJsonDocument doc(sftpSettings);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "SFTP Settings", "SFTP settings saved successfully!");
}


void DeviceSettings::on_sftp_load_btn_clicked()
{
    QFile file("config/sftp_settings.json");
    if (!file.exists()) {
        QMessageBox::warning(this, "SFTP Settings", "No saved settings found.");
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "SFTP Settings", "Failed to open settings file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "SFTP Settings", "Invalid JSON format: " + parseError.errorString());
        return;
    }

    QJsonObject sftpSettings = doc.object();

    // Update UI line edits safely
    ui->sftp_ip->setText(sftpSettings.value("ip").toString());
    ui->sftp_port->setText(sftpSettings.value("port").toString());
    ui->sftp_username->setText(sftpSettings.value("username").toString());
    ui->sftp_pass->setText(sftpSettings.value("password").toString());
    ui->sftp_int->setText(sftpSettings.value("interval").toString());

    QMessageBox::information(this, "SFTP Settings", "SFTP settings loaded successfully!");
}


void DeviceSettings::refreshInfo()
{
    auto uid = Utils::getUid();
    ui->uuid->setText(uid);
    auto currentIp = Utils::getDeviceIp();
    ui->ip->setText(currentIp);
    auto currentImei = Utils::getDeviceImei();
    ui->imei->setText(currentImei);
    auto ntpData = Utils::getNtpStatus();
    QString date   = ntpData.value("date").toString();
    ui->date_input->setText(date);
    QString status = ntpData.value("ntp_status").toString();
    ui->ntp->setText(status);
}


void DeviceSettings::on_export_btn_clicked()
{
    emit usbExport();
}

