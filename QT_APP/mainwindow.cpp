#include "mainwindow.h"
#include <QTimer>
#include "ui_mainwindow.h"
#include "login.h"
#include "channels.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include "utils.h"
#include "factdialog.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    progressValue(0),
    login(nullptr),
    settings(nullptr),
    factorySettings(nullptr),
    channels(nullptr),
    registration(nullptr),
    measure(nullptr),
    factSettings(nullptr),
    dataExporter(nullptr)
{
    ui->setupUi(this);

    initConfig();

    if (hardwareVersion.isEmpty() || softwareVersion.isEmpty()) {
        hardwareVersion = "1.5.0";
        softwareVersion = "2.0.0";
    }

    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    // create login once
    login = new LoginWidget(hardwareVersion, softwareVersion);
    usbExporter = new UsbExporter(this);
    connectLoginSignals();

    ui->progressBar->setValue(progressValue);
    ui->process_info->setText("Initializing...");

    watcher = new UsbWatcher(this);

    connect(watcher, &UsbWatcher::usbMounted, this, [=](const QString &dev, const QString &path){
        m_mountPath = path.toStdString();
        usbExporter->setDeviceName(dev, path);
        if (isLogin) {
            usbExporter->show();
        }
    });

    connect(watcher, &UsbWatcher::usbUnmounted, this, [=](){
        m_mountPath.clear();
        usbExporter->removeDeviceName();
        if (dataExporter) {
            dataExporter->stop();
            dataExporter->deleteLater();
            dataExporter = nullptr;
        }
    });

    connect(usbExporter, &QDialog::accepted, this, [=](){
        int days = usbExporter->getSelectedIntervalDays();
        if (days > 0 && !m_mountPath.empty()) {
            if (dataExporter) {
                dataExporter->stop();
                dataExporter->deleteLater();
                dataExporter = nullptr;
            }
            QMutexLocker locker(&latestDataMutex);
            dataExporter = new UsbDataExporter(QString::fromStdString(m_mountPath), days, latestData, this);
        }
    });

    serialThread = new QThread(this);

    worker = new SerialWorker("/dev/ttyS1", 115200);
    worker->moveToThread(serialThread);

    connect(this, &MainWindow::requestChannelPoll, worker, &SerialWorker::requestChannel);
    connect(this, &MainWindow::requestSerialCommand, worker, &SerialWorker::sendRawCommand);

    connect(worker, &SerialWorker::channelDataReady, this,
            [=](int ch, const QJsonObject &obj) {
                QMutexLocker locker(&latestDataMutex);
                latestData[ch] = obj;
                emit receivedChannelData(ch, obj);
            });

    connect(worker, &SerialWorker::genericResponseReady, this,
            [this](const QString &resp){
                FactDialog dialog(this);
                dialog.setLabel(std::string(resp.toStdString()));
                dialog.exec();
                dialog.deleteLater();
            });

    serialThread->start();

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, [this] {
        static int ch = 1;
        emit requestChannelPoll(ch);
        ch = (ch % 12) + 1;
    });
    pollTimer->start(200);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    timer->start(50);

    Utils::mainSound();
    showFullScreen();
}

MainWindow::~MainWindow()
{
    serialThread->quit();
    serialThread->wait();
    pollTimer->stop();
    delete ui;
}

void MainWindow::initConfig()
{
    QDir dir("config");
    if (!dir.exists())
        dir.mkpath(".");

    QFile file("config/project.json");
    if (!file.exists()) {
        QJsonObject defaultSettings {
            {"uid", ""},
            {"admin", ""},
            {"factory", "msm123"}
        };

        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(defaultSettings).toJson(QJsonDocument::Indented));
            file.close();
        }
    }

    if (file.open(QIODevice::ReadOnly)) {
        const auto data = file.readAll();
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const auto obj = doc.object();
            m_uid = obj.value("uid").toString();
            m_adminPass = obj.value("admin").toString();
            m_factoryPass = obj.value("factory").toString();
        }
        file.close();
    }

    QFile hwFile("version.json");
    if (hwFile.open(QIODevice::ReadOnly)) {
        const auto data = hwFile.readAll();
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const auto obj = doc.object();
            hardwareVersion = obj.value("hardware_version").toString();
            softwareVersion = obj.value("software_version").toString();
        }
        hwFile.close();
    }
}

void MainWindow::startProgress()
{
    ui->progressBar->setValue(progressValue);
    ui->process_info->setText("Initializing...");

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    timer->start(500);
}

void MainWindow::updateProgress()
{
    if (progressValue > 100)
        return;

    ui->progressBar->setValue(progressValue);

    if (progressValue < 20)
        ui->process_info->setText("Initializing...");
    else if (progressValue < 40)
        ui->process_info->setText("Detecting...");
    else if (progressValue < 60)
        ui->process_info->setText("Processing...");
    else if (progressValue < 100)
        ui->process_info->setText("Registration...");

    if (progressValue == 100) {
        QTimer::singleShot(500, this, &MainWindow::handlePostProgress);
    }

    progressValue += 1;
}

void MainWindow::handlePostProgress()
{
    if (m_uid.isEmpty() || m_adminPass.isEmpty()) {
        if (!registration) {
            registration = new Registration(hardwareVersion, softwareVersion);
            connect(registration, &Registration::registrationComplete, this, [this]() {
                if (!channels)
                    channels = new ChannelsHome(hardwareVersion, softwareVersion);

                connect(channels, &ChannelsHome::channelsExit, this, [this]() {
                    login->showFullScreen();
                    channels->hide();
                });
                connect(channels, &ChannelsHome::deviceInfo, this, [this]() {
                    if (isLogin && settings) {
                        settings->showFullScreen();
                        channels->hide();
                    }
                    else{
                        QMessageBox::warning(this, "Access Denied", "Please login as admin to access device settings.");                
                    }
                });
                
                connect(this, &MainWindow::receivedChannelData,
                    channels, &ChannelsHome::handleChannelData);

                channels->showFullScreen();
                registration->hide();
            });
        }
        registration->showFullScreen();
        hide();
    } else {
        if (!settings) {
            settings = new DeviceSettings();
        }
        settings->on_mqtt_load_btn_clicked(false);
        settings->on_sftp_load_btn_clicked(false);
        settings->loadAwsMqttsSettings(false);
        if (!factSettings){
            factSettings = new FactSetting;
        }
        factSettings->on_mqtt_load_btn_clicked(false);
        factSettings->on_sftp_load_btn_clicked(false);

        if (!channels)
            channels = new ChannelsHome(hardwareVersion, softwareVersion);

        connect(channels, &ChannelsHome::channelsExit, this, [this]() {
            login->showFullScreen();
            channels->hide();
        });

        connect(channels, &ChannelsHome::deviceInfo, this, [this]() {
            if (isLogin && settings) {
                settings->showFullScreen();
                channels->hide();
            }
            else{
                QMessageBox::warning(this, "Access Denied", "Please login as admin to access device settings.");                
            }
        });

        connect(this, &MainWindow::receivedChannelData,
                    channels, &ChannelsHome::handleChannelData);

        channels->showFullScreen();
        hide();
    }
}

void MainWindow::connectLoginSignals()
{
    if (!settings) {
        settings = new DeviceSettings;
    }

    connect(settings, &DeviceSettings::sftp_load, this, [this]() {
        sftpHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() {
            sftpHandler->reloadSettings();
        });
    });
    connect(settings, &DeviceSettings::mqtt_load, this, [this]() {
        mqttHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() {
            mqttHandler->reloadSettings();
        });
    });
    connect(settings, &DeviceSettings::aws_mqtts_load, this, [this]() {
        awsMqttsHandler->disconnectAwsClient();
        QTimer::singleShot(1000, [this](){
            awsMqttsHandler->reloadAwsMqttsSettings();
        });
    });
    connect(awsMqttsHandler, &AwsMqttManager::rebootRequested, this, [this] {
        this->close();
        Utils::rebootSystem();
    });
    
    connect(awsMqttsHandler, &AwsMqttManager::ntpConfigReceived,
        settings, &DeviceSettings::parse_load_store_ntp);
    connect(awsMqttsHandler, &AwsMqttManager::sftpConfigReceived,
        settings, &DeviceSettings::parse_load_store_sftp);
    connect(awsMqttsHandler, &AwsMqttManager::mqttConfigReceived,
        settings, &DeviceSettings::parse_load_store_mqtt);
    connect(settings, &DeviceSettings::deviceInfoChanged,
        awsMqttsHandler, &AwsMqttManager::processDeviceInfo);
    connect(settings, &DeviceSettings::mqttSettingsChanged,
        awsMqttsHandler, &AwsMqttManager::processMqttSettings);
    connect(settings, &DeviceSettings::sftpSettingsChanged,
        awsMqttsHandler, &AwsMqttManager::processSftpSettings);
    connect(settings, &DeviceSettings::ntpSettingsChanged,
        awsMqttsHandler, &AwsMqttManager::processNtpSettings);
    connect(this, &MainWindow::receivedChannelData,
        mqttHandler, &mqttManager::processIncomingJson);
    connect(this, &MainWindow::receivedChannelData,
        sftpHandler, &sftpManager::processIncomingJson);
    connect(this, &MainWindow::receivedChannelData,
        awsMqttsHandler, &AwsMqttManager::processAwsTelemetry);
    connect(login, &LoginWidget::adminLoginSuccess, this, [this]() {
        isLogin = true;
        connect(settings, &DeviceSettings::channels_view, this, [this]() {
            channels->showFullScreen();
            settings->hide();
        });
        connect(settings, &DeviceSettings::usbExport, this, [this]() {
            usbExporter->show();
        });
        settings->showFullScreen();
        login->hide();
    });

    // Factory login -> FactorySettings
    connect(login, &LoginWidget::factoryLoginSuccess, this, [this]() {
        isLogin = true;
        if (!factorySettings) {
            factorySettings = new FactorySettings(hardwareVersion, softwareVersion);
        }
        connect(factorySettings, &FactorySettings::factoryExit, this, [this]() {
            isLogin = false;
            login->showFullScreen();
            factorySettings->hide();
        });
        connect(factorySettings, &FactorySettings::loadFactSettings, this, [this]() {
            if (!factSettings) {
                factSettings = new FactSetting;
            }
            connect(factSettings, &FactSetting::sftp_load, this, [this]() {
                sftpHandler->disconnectClient();
                QTimer::singleShot(1000, [this]() {
                    sftpHandler->reloadSettings();
                });
            });
            connect(factSettings, &FactSetting::mqtt_load, this, [this]() {
                mqttHandler->disconnectClient();
                QTimer::singleShot(1000, [this]() {
                    mqttHandler->reloadSettings();
                });
            });
            connect(factSettings, &FactSetting::aws_mqtts_load, this, [this]() {
                awsMqttsHandler->disconnectAwsClient();
                QTimer::singleShot(1000, [this](){
                    awsMqttsHandler->reloadAwsMqttsSettings();
                });
            });
            connect(factSettings, &FactSetting::usbExport, this, [this](){
                usbExporter->show();
            });
            connect(factSettings, &FactSetting::factoryExit, this, [this]() {
                factorySettings->showFullScreen();
                factSettings->hide();
            });
            factSettings->showFullScreen();
            factorySettings->hide();
        });
        connect(factorySettings, &FactorySettings::loadMeasurements, this, [this]() {
            if (!measure) {
                measure = new Measurements;
            }
            connect(measure, &Measurements::measureExit, this, [this]() {
                factorySettings->showFullScreen();
                measure->hide();
            });
            connect(measure, &Measurements::requestSerialCommand, this, [this](const QString &cmd) {
                emit requestSerialCommand(cmd);
            });

            measure->showFullScreen();
            factorySettings->hide();
        });
        factorySettings->showFullScreen();
        login->hide();
    });
}
