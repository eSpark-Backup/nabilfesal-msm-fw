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

FactSetting::FactSetting(QWidget *parent)
    : settingsManager(parent)
    , ui(new Ui::FactSetting)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    auto firmware = Utils::getFirmwareVersion();
    ui->ota_cur_version->setText(firmware);
    auto otaInitiated = Utils::getLastOTAInitiated();
    ui->ota_last_update->setText(otaInitiated);

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
    label->move(260, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PORT", ui->sftp_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(237, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_USER", ui->sftp_user);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(237, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("SFTP_PASSWORD", ui->sftp_pass);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(200, 1);
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
    label->move(45, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_ENDPOINT", ui->mqtts_endpoint);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(185, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_PORT", ui->mqtts_port);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(210, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("MQTTs_INTERVAL", ui->mqtts_interval);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(190, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("CURRENT VERSION", ui->ota_cur_version);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(180, 1);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

    label = new QLabel("LAST UPDATE", ui->ota_last_update);
    label->setStyleSheet("QLabel { color: gray; padding-left: 0px; padding-right: 0px; border: 0px solid gray;}");
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    label->adjustSize();
    label->move(220, 1);
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


void FactSetting::on_mqtt_load_btn_clicked(bool msgBox)
{
    QJsonObject mqttSettings = loadMqttSettings(msgBox);

    if (!mqttSettings.isEmpty())
    {
        ui->mqtt_ip->setText(mqttSettings.value("ip").toString());
        ui->mqtt_user->setText(mqttSettings.value("user").toString());
        ui->mqtt_pass->setText(mqttSettings.value("password").toString());
        ui->mqtt_topic->setText(mqttSettings.value("topic").toString());
        ui->mqtt_int->setText(mqttSettings.value("interval").toString());
        if(msgBox) QMessageBox::information(this, "MQTT Settings", "MQTT settings loaded successfully!");
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
    saveMqttSettings(mqttSettings);
}


void FactSetting::on_sftp_load_btn_clicked(bool msgBox)
{
    QJsonObject sftpSettings = loadSftpSettings(msgBox);

    if (!sftpSettings.isEmpty())
    {
        ui->sftp_ip->setText(sftpSettings.value("ip").toString());
        ui->sftp_pass->setText(sftpSettings.value("password").toString());
        ui->sftp_int->setText(sftpSettings.value("interval").toString());
        ui->sftp_sch->setText(sftpSettings.value("schedule").toString());
        ui->sftp_user->setText(sftpSettings.value("username").toString());
        if(msgBox) QMessageBox::information(this, "SFTP Settings", "SFTP settings saved successfully!");
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
    saveSftpSettings(sftpSettings);
}


void FactSetting::loadAwsMqttsSettings(bool showMessages)
{
    // QFile file("config/aws_mqtts_settings.json");
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
    emit aws_mqtts_load();
}
void FactSetting::on_pushButton_4_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void FactSetting::on_pushButton_clicked()
{
    QString selectedVersion = ui->comboBox->currentText();
    if (selectedVersion.isEmpty()) {
        qWarning() << "No version selected.";
        return;
    }

    QString updatingImagePath = "/MSM/potential-pancake/MSM/images/update/updating.bmp";
    QPixmap pixmap(updatingImagePath);
    if (!pixmap.isNull()) {
        QLabel *updatingLabel = new QLabel();
        updatingLabel->setPixmap(pixmap.scaled(qApp->primaryScreen()->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        updatingLabel->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        updatingLabel->setAlignment(Qt::AlignCenter);
        updatingLabel->showFullScreen();
        qApp->processEvents(); // Force UI to update immediately
    } else {
        qWarning() << "Failed to load updating image:" << updatingImagePath;
    }

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
            this, [process, updatingImagePath](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);

        // Remove OTA image overlay if shown
        for (QWidget *w : QApplication::topLevelWidgets()) {
            QLabel *label = qobject_cast<QLabel*>(w);
            if (label && label->pixmap() && label->pixmap()->cacheKey() == QPixmap(updatingImagePath).cacheKey()) {
                label->close();
                label->deleteLater();
            }
        }

        if (exitCode == 0) {
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


void FactSetting::on_mqtts_load_btn_clicked()
{
    QString filePath = "config/aws_mqtts_settings.json";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open MQTT configuration file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid MQTT configuration file format.");
        return;
    }

    QJsonObject obj = doc.object();

    // Fill UI fields (port skipped)
    ui->mqtts_endpoint->setText(obj.value("endpoint").toString());
    ui->mqtts_interval->setText(obj.value("interval").toString());
    // Port is constant and not editable

    // ---- Check for certificate files ----
    QString basePath = "/MSM/potential-pancake/MSM/creds/prov_certs/";
    QString caPath   = "/MSM/potential-pancake/MSM/creds/claim_certs/AmazonRootCA1.pem";
    QString certPath = basePath + "deviceCert.pem";
    QString keyPath  = basePath + "privateKey.pem";

    ui->ca1->setChecked(QFile::exists(caPath));
    ui->claim->setChecked(QFile::exists(certPath));
    ui->key->setChecked(QFile::exists(keyPath));

}



void FactSetting::on_mqtts_save_btn_clicked()
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
    if (!obj.contains("topic"))
        obj["topic"] = "$aws/things/fleet_prov_shadow_test_1/shadow/update";

    // Write updated JSON back
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to save MQTT configuration.");
        return;
    }

    QJsonDocument newDoc(obj);
    file.write(newDoc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "Saved", "MQTTS configuration saved successfully.");
}

void FactSetting::on_pushButton_3_clicked()
{
        emit runFleetProvisioning();
}


void FactSetting::on_pushButton_2_clicked()
{
    QString claimDir = "/MSM/potential-pancake/MSM/creds/claim_certs/";
    QString configPath = "config/sftp_settings.json";

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to open SFTP config file.");
        qWarning() << "Failed to open SFTP config:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, "Error", "Invalid SFTP configuration file format.");
        return;
    }

    QJsonObject obj = doc.object();
    QString ip       = obj.value("ip").toString();
    QString username = obj.value("username").toString();
    QString password = obj.value("password").toString();
    QString port     = obj.value("port").toString();

    if (ip.isEmpty() || username.isEmpty() || password.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "Error", "Incomplete SFTP configuration.");
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

    QString remotePath = "/aws_iot_core/claim_certs/*";
    QString localPath  = claimDir;

    QString command = QString("sshpass -p '%1' scp -P %2 -o StrictHostKeyChecking=no %3@%4:%5 %6")
                      .arg(password)
                      .arg(port)
                      .arg(username)
                      .arg(ip)
                      .arg(remotePath)
                      .arg(localPath);


    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::readyReadStandardOutput, [process]() {
        qDebug().noquote() << process->readAllStandardOutput();
    });

    connect(process, &QProcess::readyReadStandardError, [process]() {
        qWarning().noquote() << process->readAllStandardError();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [process, claimDir](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);

        if (exitCode == 0) {
            QMessageBox::information(nullptr, "Success", "Claim certificates downloaded successfully!");
        } else {
            QMessageBox::warning(nullptr, "Error", "Failed to download claim certificates. Please check SFTP connection or credentials.");
        }

        process->deleteLater();
    });

    process->start("bash", QStringList() << "-c" << command);

    if (!process->waitForStarted()) {
        QMessageBox::warning(this, "Error", "Failed to start SFTP download process.");
        process->deleteLater();
        return;
    }
}
