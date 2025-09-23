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

FactSetting::FactSetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FactSetting)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    ui->stackedWidget->setCurrentIndex(0);

    QLabel* label = new QLabel("MQTT_IP", ui->mqtt_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(272, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_PASSWORD", ui->mqtt_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(212, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_USER", ui->mqtt_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(247, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTT_TOPIC", ui->mqtt_topic);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(242, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("PORT", ui->mqtt_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(114, 2);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->mqtt_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(80, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_IP", ui->sftp_ip);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(272, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(257, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(257, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PASSWORD", ui->sftp_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(217, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("INTERVAL (s)", ui->sftp_int);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(80, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SCHEDULE (24hr)", ui->sftp_sch);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(55, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 180);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->lan_address, ui->lan_netmast, ui->dns_server, ui->lan_server,
        ui->mqtt_ip, ui->mqtt_port, ui->mqtt_user,
        ui->mqtt_pass, ui->mqtt_topic, ui->mqtt_int,
        ui->sftp_ip, ui->sftp_port, ui->sftp_user,
        ui->sftp_pass, ui->sftp_int, ui->sftp_sch
    };

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);

        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            edit->clearFocus();
            keyboard->hide();
        });
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


void FactSetting::on_back_btn_clicked()
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


void FactSetting::on_pushButton_7_clicked()
{
    ui->pushButton_7->setStyleSheet(buttonClickedStyle);
    ui->pushButton_6->setStyleSheet(buttonNormalStyle);
    isDhcpEnabled = true;
    isStaticEnabled = false;
}


void FactSetting::on_pushButton_6_clicked()
{
    ui->pushButton_6->setStyleSheet(buttonClickedStyle);
    ui->pushButton_7->setStyleSheet(buttonNormalStyle);
    isDhcpEnabled = false;
    isStaticEnabled = true;
}


void FactSetting::on_lan_load_btn_clicked()
{
    QFile file("config/lan_settings.json");
    if (!file.exists()) {
        QMessageBox::warning(this, "LAN Settings", "No saved settings found.");
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "LAN Settings", "Failed to open settings file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "LAN Settings", "Invalid JSON format: " + parseError.errorString());
        return;
    }

    QJsonObject lanSettings = doc.object();

    isDhcpEnabled = lanSettings.value("isDhcpEnabled").toBool();
    isStaticEnabled = lanSettings.value("isStaticEnabled").toBool();

    ui->lan_address->setText(lanSettings.value("ip").toString());
    ui->lan_server->setText(lanSettings.value("server").toString());
    ui->lan_netmast->setText(lanSettings.value("netmask").toString());
    ui->dns_server->setText(lanSettings.value("dns_server").toString());

    if (isStaticEnabled)
    {
        ui->pushButton_6->setStyleSheet(buttonClickedStyle);
        ui->pushButton_7->setStyleSheet(buttonNormalStyle);
    }
    else
    {
        ui->pushButton_7->setStyleSheet(buttonClickedStyle);
        ui->pushButton_6->setStyleSheet(buttonNormalStyle);
    }


    QMessageBox::information(this, "LAN Settings", "LAN settings loaded successfully!");
}


void FactSetting::on_lan_save_btn_clicked()
{
    QString lanIp       = ui->lan_address->text().trimmed();
    QString lanServer     = ui->lan_server->text().trimmed();
    QString lanNetMask     = ui->lan_netmast->text().trimmed();
    QString dnsServer     = ui->dns_server->text().trimmed();

    // 1. Create JSON object
    QJsonObject lanSettings;
    lanSettings["ip"]       = lanIp;
    lanSettings["server"]     = lanServer;
    lanSettings["netmask"] = lanNetMask;
    lanSettings["dns_server"] = dnsServer;
    lanSettings["isDhcpEnabled"] = isDhcpEnabled;
    lanSettings["isStaticEnabled"] = isStaticEnabled;

    // 2. Ensure config directory exists
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/lan_settings.json
    QFile file("config/lan_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "LAN Settings", "Failed to save LAN settings.");
        return;
    }

    QJsonDocument doc(lanSettings);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "LAN Settings", "LAN settings saved successfully!");
}


void FactSetting::on_mqtt_load_btn_clicked()
{
    AwsMqttManager *mqttHandler = &AwsMqttManager::instance();
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

    QTimer::singleShot(1000, [this, mqttHandler](){
        mqttHandler->reloadSettings();
    });
}


void FactSetting::on_mqtt_save_btn_clicked()
{
    AwsMqttManager *mqttHandler = &AwsMqttManager::instance();
    mqttHandler->disconnectClient();

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

    QTimer::singleShot(1000, [this, mqttHandler](){
        mqttHandler->reloadSettings();
    });
}


void FactSetting::on_sftp_load_btn_clicked()
{
    sftpManager *sftpHandler = &sftpManager::instance();
    sftpHandler->disconnectClient();

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
    ui->sftp_user->setText(sftpSettings.value("username").toString());
    ui->sftp_pass->setText(sftpSettings.value("password").toString());
    ui->sftp_int->setText(sftpSettings.value("interval").toString());
    ui->sftp_sch->setText(sftpSettings.value("schedule").toString());

    QMessageBox::information(this, "SFTP Settings", "SFTP settings loaded successfully!");

    QTimer::singleShot(1000, [this, sftpHandler](){
        sftpHandler->reloadSettings();
    });
}


void FactSetting::on_sftp_save_btn_clicked()
{
    sftpManager *sftpHandler = &sftpManager::instance();
    sftpHandler->disconnectClient();
    
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

    // 2. Ensure config directory exists
    QDir dir("config");
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 3. Write to config/factory_sftp_settings.json
    QFile file("config/sftp_settings.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "SFTP Settings", "Failed to save SFTP settings.");
        return;
    }

    QJsonDocument doc(sftpSettings);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "SFTP Settings", "SFTP settings saved successfully!");
    
    QTimer::singleShot(1000, [this, sftpHandler](){
        sftpHandler->reloadSettings();
    });
}

