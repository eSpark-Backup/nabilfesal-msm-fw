#include "factsetting.h"
#include "ui_factsetting.h"
#include <QDir>
#include <QFile>
#include <QScreen>
#include "utils.h"
#include "toast.h"
#include <QProcess>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include "encryptionhelper.h"


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

    on_ota_version_btn_clicked();

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
    if (selVersion.isEmpty()) {
        Toast::show("No OTA version selected.", this, Toast::Warning);
        return;
    }

    int ret = QMessageBox::question(
        this, "Over-The-Air", QString("Do you want to OTA to %1?").arg(selVersion), QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    startOTA(selVersion);
}


void FactSetting::startOTA(const QString &selectedVersion)
{
    QString updatingImagePath = ":/icons/updating.png";
    updatingLabel = Utils::showImageFullscreenFor(updatingImagePath, -1);

    QString otaFileName = QString("MSM_%1.zip").arg(selectedVersion);

    QFile file(sftpConfigPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open SFTP config:" << file.errorString();
        return;
    }

    EncryptionHelper::check_and_encrypt_files(sftpConfigPath(), "Fact SFTP", false, nullptr);

    QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(sftpConfigPath(), Utils::getEncryptionKey());
    if (decryptedData.isEmpty()) {
        qWarning() << "Failed to decrypt Fact SFTP config file (wrong password or corrupted file)";
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(decryptedData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid SFTP config JSON:" << parseError.errorString();
        return;
    }

    QJsonObject obj  = doc.object();
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


void FactSetting::on_ota_version_btn_clicked()
{
    if (m_isFetchingVersions)
        return;
    
    m_isFetchingVersions = true;
    ui->ota_version_btn->setEnabled(false);
    getAvailableVersionsFromSFTP();
}


void FactSetting::getAvailableVersionsFromSFTP()
{
    QString filePath = sftpConfigPath();
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "SFTP config file not found for OTA versions";
        return;
    }

    QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(filePath, Utils::getEncryptionKey());
    if (decryptedData.isEmpty()) {
        qWarning() << "Failed to decrypt SFTP config for versions";
        finishFetchingVersions({});
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
    if (!doc.isObject()) {
        qWarning() << "Invalid SFTP JSON";
        finishFetchingVersions({});
        return;
    }

    QJsonObject obj = doc.object();
    QString ip       = obj.value("ip").toString();
    QString username = obj.value("username").toString();
    QString password = obj.value("password").toString();

    if (ip.isEmpty() || username.isEmpty() || password.isEmpty()) {
        qWarning() << "Incomplete SFTP configuration";
        finishFetchingVersions({});
        return;
    }

    // --- Start sshpass + sftp asynchronously ---
    QProcess *process = new QProcess(this);
    m_sftpOutput.clear();

    QStringList args;
    args << "-p" << password
         << "sftp"
         << "-o" << "StrictHostKeyChecking=no"
         << "-o" << "UserKnownHostsFile=/dev/null"
         << QString("%1@%2").arg(username, ip);

    connect(process, &QProcess::started, this, [process]() {
        process->write("ls /ota-test\n");
        process->write("bye\n");
        process->closeWriteChannel();
    });

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        m_sftpOutput += process->readAllStandardOutput();
    });

    connect(process, &QProcess::readyReadStandardError, this, [process]() {
        qWarning() << process->readAllStandardError();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus status) {

        Q_UNUSED(exitCode)
        Q_UNUSED(status)

        QString output = m_sftpOutput;
        m_sftpOutput.clear();

        QStringList versionList;

        output.replace("sftp> ", "");
        QStringList tokens = output.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        for (QString f : tokens) {
            if (!f.endsWith(".zip")) continue;
            f = QFileInfo(f).fileName();
            if (!f.startsWith("MSM_")) continue;
            f.remove(0, 4);
            f.chop(4);
            versionList << f;
        }

        versionList.removeDuplicates();
        versionList.sort();

        finishFetchingVersions(versionList);
        process->deleteLater();
    });

    process->start("sshpass", args);
}


void FactSetting::finishFetchingVersions(const QStringList &versionList)
{
    m_isFetchingVersions = false;
    ui->ota_version_btn->setEnabled(true);

    ui->comboBox->clear();
    for (const QString &v : versionList)
        ui->comboBox->addItem(v);
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
