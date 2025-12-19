#include "factsetting.h"
#include "ui_factsetting.h"
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
    setUiPtr(this);
    setCertPath(CLAIM_PATH, PROV_PATH);
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
    label->move(55, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_ENDPOINT", ui->mqtts_endpoint);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(215, 1);
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
    emit rebootMCM();
}


void FactSetting::on_mqtt_load_btn_clicked(bool msgBox)
{
    load_mqtt_settings(msgBox);
}


void FactSetting::load_mqtt_settings(bool msgBox, bool reconnect)
{
    QJsonObject mqttSettings = loadMqttSettings(msgBox, reconnect);
    if (!mqttSettings.isEmpty()) apply_mqtt_Settings(mqttSettings, msgBox);
}


void FactSetting::on_mqtt_save_btn_clicked()
{
    QJsonObject mqttSettings = getMqttSettings();
    if(!mqttSettings.isEmpty()) saveMqttSettings(mqttSettings);
}


void FactSetting::on_sftp_load_btn_clicked(bool msgBox)
{
    load_sftp_settings(msgBox);
}


void FactSetting::load_sftp_settings(bool msgBox, bool reconnect)
{
    QJsonObject sftpSettings = loadSftpSettings(msgBox, reconnect);
    if (!sftpSettings.isEmpty()) apply_sftp_Settings(sftpSettings, msgBox);
}


void FactSetting::on_sftp_save_btn_clicked()
{
    QJsonObject sftpSettings = getSftpSettings();
    if(!sftpSettings.isEmpty()) saveSftpSettings(sftpSettings);
}


void FactSetting::on_OTA_pushButton_clicked()
{
    QString selVersion = ui->comboBox->currentText();
    ui->OTA_pushButton->clearFocus();
    startOTA(selVersion);
}

void FactSetting::startOTA(const QString &selectedVersion)
{
    if (selectedVersion.isEmpty()) {
        Toast::show("No OTA version selected.", this, Toast::Warning);
        return;
    }

    int ret = QMessageBox::question(
        this, "Over-The-Air", QString("Do you want to OTA to %1?").arg(selectedVersion), QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

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
    QJsonObject mqttsSettings = loadMqttsSettings(msgBox, reconnect);
    if (!mqttsSettings.isEmpty()) apply_mqtts_Settings(mqttsSettings, msgBox);
}


void FactSetting::on_mqtts_save_btn_clicked(bool msgBox)
{
    QJsonObject mqttsSettings = getMqttsSettings();
    if(!mqttsSettings.isEmpty()) saveMqttsSettings(mqttsSettings, true);
}


void FactSetting::on_provision_btn_clicked()
{
    QString endpoint = ui->mqtts_endpoint->text();
    if (endpoint.length() < 5) {
        Toast::show("[Invalid Input] Endpoint must be at least 5 characters long.", this, Toast::Error);
        return;
    }
    ui->provision_btn->setEnabled(false);
    emit runFleetProvisioning();
}


void FactSetting::on_fetch_claim_btn_clicked()
{
    fetch_claim_certs();
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
    // QString jsonString = QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact));
    // qDebug().noquote() << "OTA JSON:" << QJsonDocument(json).toJson(QJsonDocument::Indented);

}


QString FactSetting::configJsonPath() const
{
#if defined(BUILD_X86)
    return QDir::homePath() + "/Downloads/aws-iot-device-sdk-embedded-C/build/bin/config.json";
#elif defined(BUILD_V3S)
    return "/MSM/potential-pancake/MSM/bin/fleet_provisioner/config.json";
#endif
}


void FactSetting::send_aws_updated_data()
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
}


void FactSetting::updateInfo(const DeviceInfo &f)
{
    auto setStatusColor = [](QLabel* label, const QString& status) {
        QString color = (status == "Connected") ? "green" : "red";
        label->setStyleSheet(QString("background-color: %1; border-radius: 10px;").arg(color));
    };

    setStatusColor(ui->fact_sftp_con_status,  f.conn.sftpStatus);
    setStatusColor(ui->fact_mqtt_con_status,  f.conn.mqttStatus);
    setStatusColor(ui->fact_mqtts_con_status, f.conn.mqttsStatus);
}