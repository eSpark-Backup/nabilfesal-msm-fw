// MainWindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QMessageBox>

#include "utils.h"
#include "factdialog.h"

// -----------------------------
// Constructor / Destructor
// -----------------------------
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
      dataExporter(nullptr),
      usbExporter(nullptr),
      watcher(nullptr),
      serialThread(nullptr),
      worker(nullptr),
      pollTimer(nullptr),
      usbDetected(false)
{
    ui->setupUi(this);

    initConfig();

    if (hardwareVersion.isEmpty() || softwareVersion.isEmpty()) {
        hardwareVersion = "1.5.0";
        softwareVersion = "2.0.0";
    }

    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    // Setup segmented responsibilities
    setupLogin();
    setupUsb();
    setupSerial();

    // Important: set connections between settings/managers
    connectLoginSignals();

    // UI progress and polling
    setupProgressBar();
    setupPolling();

    channelDataWatchdog = new QTimer(this);
    channelDataWatchdog->setSingleShot(true);
    connect(channelDataWatchdog, &QTimer::timeout, this, &MainWindow::onChannelDataWatchdogTimeout);
    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);

    Utils::mainSound();
    showFullScreen();

    // Send CT/I/V ratio and offsets at startup (if config present)
    startApp = true;
    sendRatioSettings();
    QTimer::singleShot(20000, this, [this]() { startApp = false; });
}

MainWindow::~MainWindow()
{
    if (serialThread) {
        serialThread->quit();
        serialThread->wait();
    }
    if (pollTimer) {
        pollTimer->stop();
    }
    delete ui;
}

// -----------------------------
// Config / Init
// -----------------------------
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

// -----------------------------
// Setup: login / USB / Serial
// -----------------------------
void MainWindow::setupLogin()
{
    login = new LoginWidget(hardwareVersion, softwareVersion);

    // simple routing: handlers implemented as slots
    connect(login, &LoginWidget::loadChannels, this, &MainWindow::onLoadChannels);
    connect(login, &LoginWidget::adminLoginSuccess, this, &MainWindow::onAdminLogin);
    connect(login, &LoginWidget::factoryLoginSuccess, this, &MainWindow::onFactoryLogin);
}

void MainWindow::setupUsb()
{
    usbExporter = new UsbExporter(this);
    watcher = new UsbWatcher(this);

    connect(watcher, &UsbWatcher::usbMounted, this, &MainWindow::onUsbMounted);
    connect(watcher, &UsbWatcher::usbUnmounted, this, &MainWindow::onUsbUnmounted);
    connect(usbExporter, &QDialog::accepted, this, &MainWindow::onUsbExportAccepted);
}

void MainWindow::setupSerial()
{
    serialThread = new QThread(this);
    worker = new SerialWorker("/dev/ttyS1", 115200);
    worker->moveToThread(serialThread);

    connect(this, &MainWindow::requestChannelPoll, worker, &SerialWorker::requestChannel);
    connect(this, &MainWindow::requestSerialCommand, worker, &SerialWorker::sendRawCommand);

    connect(worker, &SerialWorker::channelDataReady, this,
            &MainWindow::onChannelDataReady);

    connect(worker, &SerialWorker::genericResponseReady, this,
            &MainWindow::onGenericResponseReady);

    serialThread->start();
}

// -----------------------------
// Progress & Polling setup
// -----------------------------
void MainWindow::setupProgressBar()
{
    ui->progressBar->setValue(progressValue);
    ui->process_info->setText("Initializing...");

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    timer->start(50);
}

void MainWindow::setupPolling()
{
    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, [this] {
        static int ch = 1;
        emit requestChannelPoll(ch);
        ch = (ch % 12) + 1;
    });
    pollTimer->start(200);
}

// -----------------------------
// Common helper connectors
// -----------------------------
void MainWindow::connectChannels(ChannelsHome *ch)
{
    connect(ch, &ChannelsHome::channelsExit, this, &MainWindow::onChannelsExit);
    connect(this, &MainWindow::receivedChannelData, ch, &ChannelsHome::handleChannelData);
}

void MainWindow::connectFactSettings(FactSetting *fs)
{
    // Keep configuration reload handling the same as DeviceSettings
    connect(fs, &FactSetting::sftp_load, this, [this]() {
        sftpHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { sftpHandler->reloadSettings(); });
    });
    connect(fs, &FactSetting::mqtt_load, this, [this]() {
        mqttHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { mqttHandler->reloadSettings(); });
    });
    connect(fs, &FactSetting::aws_mqtts_load, this, [this]() {
        awsMqttsHandler->disconnectAwsClient();
        QTimer::singleShot(1000, [this]() { awsMqttsHandler->reloadAwsMqttsSettings(); });
    });

    connect(fs, &FactSetting::usbExport, this, [this]() { usbExporter->show(); });

    connect(fs, &FactSetting::factoryExit, this, [this, fs]() {
        if (factorySettings) {
            factorySettings->showFullScreen();
            fs->hide();
        }
    });
}

// -----------------------------
// connectLoginSignals (restored & cleaned)
// -----------------------------
void MainWindow::connectLoginSignals()
{
    // Ensure settings exists so manager <-> settings connections are present early.
    if (!settings) {
        settings = new DeviceSettings;
    }

    // When DeviceSettings says "reload SFTP", disconnect and reload after a short delay.
    connect(settings, &DeviceSettings::sftp_load, this, [this]() {
        sftpHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { sftpHandler->reloadSettings(); });
    });

    // When DeviceSettings says "reload MQTT", disconnect and reload.
    connect(settings, &DeviceSettings::mqtt_load, this, [this]() {
        mqttHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { mqttHandler->reloadSettings(); });
    });

    // AWS MQTTS reload request
    connect(settings, &DeviceSettings::aws_mqtts_load, this, [this]() {
        awsMqttsHandler->disconnectAwsClient();
        QTimer::singleShot(5000, [this]() { awsMqttsHandler->reloadAwsMqttsSettings(); });
    });

    // AWS manager may ask for reboot
    connect(awsMqttsHandler, &AwsMqttManager::rebootRequested, this, [this]() {
        this->close();
        Utils::rebootSystem();
    });

    // If AWS sends back stored configs, populate DeviceSettings fields
    connect(awsMqttsHandler, &AwsMqttManager::ntpConfigReceived,
            settings, &DeviceSettings::parse_load_store_ntp);
    connect(awsMqttsHandler, &AwsMqttManager::sftpConfigReceived,
            settings, &DeviceSettings::parse_load_store_sftp);
    connect(awsMqttsHandler, &AwsMqttManager::mqttConfigReceived,
            settings, &DeviceSettings::parse_load_store_mqtt);

    // When settings change, notify AWS manager to process them
    connect(settings, &DeviceSettings::deviceInfoChanged,
            awsMqttsHandler, &AwsMqttManager::processDeviceInfo);
    connect(settings, &DeviceSettings::mqttSettingsChanged,
            awsMqttsHandler, &AwsMqttManager::processMqttSettings);
    connect(settings, &DeviceSettings::sftpSettingsChanged,
            awsMqttsHandler, &AwsMqttManager::processSftpSettings);
    connect(settings, &DeviceSettings::ntpSettingsChanged,
            awsMqttsHandler, &AwsMqttManager::processNtpSettings);

    // Wire telemetry -> managers
    connect(this, &MainWindow::receivedChannelData,
            mqttHandler, &mqttManager::processIncomingJson);
    connect(this, &MainWindow::receivedChannelData,
            sftpHandler, &sftpManager::processIncomingJson);
    connect(this, &MainWindow::receivedChannelData,
            awsMqttsHandler, &AwsMqttManager::processAwsTelemetry);

    // Note:
    // Navigation-related connections (showing/hiding settings/channels/usbExport)
    // are handled where UI is shown (onAdminLogin / onFactoryLogin) to avoid double
    // connecting repeated lambdas.
}

// -----------------------------
// USB slots
// -----------------------------
void MainWindow::onUsbMounted(const QString &dev, const QString &path)
{
    m_mountPath = path.toStdString();
    usbExporter->setDeviceName(dev, path);

    if (!isLogin) {
        usbDetected = true;
        login->showFullScreen();
    } else {
        usbExporter->show();
    }
}

void MainWindow::onUsbUnmounted()
{
    m_mountPath.clear();
    usbExporter->removeDeviceName();
    if (dataExporter) {
        dataExporter->stop();
        dataExporter->deleteLater();
        dataExporter = nullptr;
    }
}

void MainWindow::onUsbExportAccepted()
{
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
}

// -----------------------------
// Serial slots
// -----------------------------
void MainWindow::onChannelDataReady(int ch, const QJsonObject &obj)
{
    QMutexLocker locker(&latestDataMutex);
    latestData[ch] = obj;
    emit receivedChannelData(ch, obj);
    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);
}

void MainWindow::onGenericResponseReady(const QString &resp)
{
    if (!startApp) {
        FactDialog dialog(this);
        dialog.setLabel(resp.toStdString());
        dialog.exec();
        dialog.deleteLater();
    }
}

// -----------------------------
// Login / Navigation slots
// -----------------------------
void MainWindow::onLoadChannels()
{
    if (!channels)
        channels = new ChannelsHome(hardwareVersion, softwareVersion);

    connectChannels(channels);
    channels->showFullScreen();
    login->hide();
}

void MainWindow::onAdminLogin()
{
    isLogin = true;
    if (!settings)
        settings = new DeviceSettings;

    // navigation actions for settings
    // -> when settings asks to view channels
    connect(settings, &DeviceSettings::channels_view, this, [this]() {
        isLogin = false;
        if (!channels) channels = new ChannelsHome(hardwareVersion, softwareVersion);
        connectChannels(channels);
        channels->showFullScreen();
        settings->hide();
    });

    // -> when settings requests USB export
    connect(settings, &DeviceSettings::usbExport, this, [this]() {
        if (usbExporter) usbExporter->show();
    });

    settings->showFullScreen();
    login->hide();

    if (usbDetected) {
        usbExporter->show();
        usbDetected = false;
    }
}

void MainWindow::onFactoryLogin()
{
    isLogin = true;
    if (!factorySettings)
        factorySettings = new FactorySettings(hardwareVersion, softwareVersion);

    // exit back to login
    connect(factorySettings, &FactorySettings::factoryExit, this, [this]() {
        isLogin = false;
        login->showFullScreen();
        factorySettings->hide();
    });

    // load fact settings (factory -> factSettings)
    connect(factorySettings, &FactorySettings::loadFactSettings, this, [this]() {
        if (!factSettings)
            factSettings = new FactSetting;
        connectFactSettings(factSettings);
        factSettings->showFullScreen();
        factorySettings->hide();
    });

    // load measurements
    connect(factorySettings, &FactorySettings::loadMeasurements, this, [this]() {
        if (!measure)
            measure = new Measurements;
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

    if (usbDetected) {
        usbExporter->show();
        usbDetected = false;
    }
}

void MainWindow::onChannelsExit()
{
    isLogin = false;
    if (login) login->showFullScreen();
    if (channels) channels->hide();
}

// -----------------------------
// Progress management
// -----------------------------
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
    // If first setup not done, show registration UI
    if (m_uid.isEmpty() || m_adminPass.isEmpty()) {
        if (!registration) {
            registration = new Registration(hardwareVersion, softwareVersion);

            connect(registration, &Registration::registrationComplete, this, [this]() {
                if (!channels)
                    channels = new ChannelsHome(hardwareVersion, softwareVersion);
                connectChannels(channels);
                channels->showFullScreen();
                registration->hide();
            });
        }

        registration->showFullScreen();
        hide();
        return;
    }

    // Normal flow when system already configured
    if (!settings) {
        settings = new DeviceSettings();
    }

    // load preset settings into DeviceSettings and FactSetting (original behavior)
    settings->on_mqtt_load_btn_clicked(false);
    settings->on_sftp_load_btn_clicked(false);
    settings->loadAwsMqttsSettings(false);

    if (!factSettings) {
        factSettings = new FactSetting;
    }
    factSettings->on_mqtt_load_btn_clicked(false);
    factSettings->on_sftp_load_btn_clicked(false);

    if (!channels)
        channels = new ChannelsHome(hardwareVersion, softwareVersion);

    connectChannels(channels);
    channels->showFullScreen();
    hide();
}

// -----------------------------
// File-based ratio/offset send
// -----------------------------
void MainWindow::sendRatioSettings()
{
    QFile ratioFile("config/ct_ratio_settings.json");
    if (ratioFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = ratioFile.readAll();
        ratioFile.close();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            emit requestSerialCommand(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
        }
    }

    QFile iOffsetFile("config/i_offset_settings.json");
    if (iOffsetFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = iOffsetFile.readAll();
        iOffsetFile.close();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            emit requestSerialCommand(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
        }
    }

    QFile vOffsetFile("config/v_offset_settings.json");
    if (vOffsetFile.open(QIODevice::ReadOnly)) {
        QByteArray jsonData = vOffsetFile.readAll();
        vOffsetFile.close();
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            emit requestSerialCommand(QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact)));
        }
    }
}

void MainWindow::onChannelDataWatchdogTimeout()
{
    
    // Stop Polling
    if (pollTimer && pollTimer->isActive()) {
        pollTimer->stop();
    }

    QProcess::execute("sh", QStringList() << "-c" << "echo 0 > /sys/class/gpio/gpio37/value");
    QThread::sleep(1);
    QProcess::execute("sh", QStringList() << "-c" << "echo 1 > /sys/class/gpio/gpio37/value");


    if (pollTimer) {
        pollTimer->start(200);
    }

    startApp = true;
    sendRatioSettings();
    QTimer::singleShot(20000, this, [this]() { 
        startApp = false; 
    });

    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);
}