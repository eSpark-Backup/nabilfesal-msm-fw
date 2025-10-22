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
#include <QKeyEvent>
#include <QApplication>
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
    dataExporter = new SystemDataExporter(this);
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

    connect(this, &MainWindow::homeDetected, this, &MainWindow::showChannelsHome);

    Utils::mainSound();
    showFullScreen();

    // Send CT/I/V ratio and offsets at startup (if config present)
    startApp = true;
    sendRatioSettings();
    QTimer::singleShot(20000, this, [this]() { startApp = false; });

    qApp->installEventFilter(this);
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
            {"factory", "admin@441322@"}
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
    connect(fs, &FactSetting::aws_mqtts_load, this, [fs, this]() {
        awsMqttsHandler->disconnectAwsClient();
        fs->checkCertificates();
        QTimer::singleShot(1000, [this]() { awsMqttsHandler->reloadAwsMqttsSettings(); });
    });

    connect(fs, &FactSetting::usbExport, this, [this]() { usbExporter->show(); });

    connect(fs, &FactSetting::factoryExit, this, [this, fs]() {
        if (factorySettings) {
            factorySettings->showFullScreen();
            fs->hide();
        }
    });

    connect(fs, &FactSetting::runFleetProvisioning, this, [this]() { factSettings->runFleetProvisionerAsync(); });
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
        if(factSettings != nullptr) factSettings->checkCertificates();
        QTimer::singleShot(1000, [this]() { awsMqttsHandler->reloadAwsMqttsSettings(); });
    });

    // AWS manager may ask for reboot
    connect(awsMqttsHandler, &AwsMqttManager::rebootRequested, this, [this]() {
        this->close();
        awsMqttsHandler->disconnectAwsClient();
        qDebug() << "disconnect reboot called";
        Utils::rebootSystem();
    },Qt::QueuedConnection);

    // If AWS sends back stored configs, populate DeviceSettings fields
    connect(awsMqttsHandler, &AwsMqttManager::ntpConfigReceived,
            settings, &DeviceSettings::parse_load_store_ntp);
    connect(awsMqttsHandler, &AwsMqttManager::sftpConfigReceived,
            settings, &DeviceSettings::parse_load_store_sftp);
    connect(awsMqttsHandler, &AwsMqttManager::mqttConfigReceived,
            settings, &DeviceSettings::parse_load_store_mqtt);
    connect(awsMqttsHandler, &AwsMqttManager::updateAwsData,
            settings, &DeviceSettings::send_aws_updated_data);

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
    startPingTimer();
}

// -----------------------------
// USB slots
// -----------------------------
void MainWindow::onUsbMounted(const QString &dev, const QString &path)
{
    m_mountPath = path.toStdString();
    usbExporter->setDeviceName(dev, path);

    if (!startApp)
    {
        if (!isLogin) {
            usbDetected = true;
            login->showFullScreen();
        } else {
            usbExporter->show();
        }
    }
}

void MainWindow::onUsbUnmounted()
{
    m_mountPath.clear();
    usbExporter->removeDeviceName();
}

void MainWindow::onUsbExportAccepted()
{
    int days = usbExporter->getSelectedIntervalDays();
    if (days > 0 && !m_mountPath.empty()) {

        QString sysFilePath = dataExporter->getFilePath();

        QFile inFile(sysFilePath);
        if (!inFile.exists()) {
            QMessageBox::warning(nullptr, "Export Error", "System CSV file not found!");
            return ;
        }

        if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(nullptr, "Export Error", "Unable to open system log file!");
            return ;
        }

        QTextStream in(&inFile);
        in.setCodec("UTF-8");

        QStringList lines;
        while (!in.atEnd())
            lines << in.readLine();
        inFile.close();

        if (lines.size() <= 1) {
            QMessageBox::warning(nullptr, "Export Error", "No data available for export.");
            return ;
        }

        QString header = lines.first();
        QStringList filtered;
        filtered << header;

        QDate cutoff = QDate::currentDate().addDays(-days);
        QDate startDate, endDate = QDate::currentDate();

        for (int i = 1; i < lines.size(); ++i) {
            QStringList parts = lines[i].split(",");
            if (parts.isEmpty()) continue;
            QDate date = QDate::fromString(parts[0].remove("\""), "dd/MM/yyyy");
            if (date >= cutoff) {
                filtered << lines[i];
                if (!startDate.isValid() || date < startDate)
                    startDate = date;
            }
        }

        if (filtered.size() <= 1) {
            QMessageBox::information(nullptr, "Export Info", "No data found in selected range.");
            return ;
        }

        QString exportFileName = QString("%1_%2_to_%3.csv")
                                    .arg(m_uid)
                                    .arg(startDate.toString("yyyyMMdd"))
                                    .arg(endDate.toString("yyyyMMdd"));

        auto exportedFilePath = QString::fromStdString(m_mountPath) + "/" + exportFileName;

        QFile outFile(exportedFilePath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(nullptr, "Export Error", "Unable to write to USB drive!");
            return ;
        }

        QTextStream out(&outFile);
        out.setCodec("UTF-8");
        for (const QString &l : filtered)
            out << l << "\n";
        outFile.close();

        QMessageBox::information(nullptr, "Export Complete",
                                QString("Exported last %1 days to:\n%2")
                                    .arg(days)
                                    .arg(exportedFilePath));
        
    }
}

// -----------------------------
// Serial slots
// -----------------------------
void MainWindow::onChannelDataReady(int ch, const QJsonObject &obj)
{
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

    // change text color on the progress bar when >= 50%
    if (progressValue >= 50) {
        ui->progressBar->setStyleSheet(R"(
            QProgressBar {
                border: 2px solid #0078d7;
                border-radius: 12px;
                background-color: white;
                text-align: center;
                color: white;            /* white text when >= 50% */
                font: bold 14px;
            }
            QProgressBar::chunk {
                border-radius: 8px;
                background-color: #0078d7;
                margin: 2px;
            }
        )");
    } 

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
    settings->on_mqtt_load_btn_clicked(true);
    settings->on_sftp_load_btn_clicked(true);
    settings->on_mqtts_load_btn_clicked(true);

    if (!factSettings) {
        factSettings = new FactSetting;
    }
    factSettings->on_mqtt_load_btn_clicked(true);
    factSettings->on_sftp_load_btn_clicked(true);
    factSettings->on_mqtts_load_btn_clicked(true);

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

    QTimer::singleShot(5000, this, [this](){
        startApp = true;
        sendRatioSettings();
        QTimer::singleShot(20000, this, [this]() { 
            startApp = false; 
        });
    });    

    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // First: handle key events globally
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Home) {
            emit homeDetected();
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void MainWindow::showChannelsHome()
{
    // Hide all possible open windows
    if (login && login->isVisible()) login->hide();
    if (settings && settings->isVisible()) settings->hide();
    if (factorySettings && factorySettings->isVisible()) factorySettings->hide();
    if (factSettings && factSettings->isVisible()) factSettings->hide();
    if (measure && measure->isVisible()) measure->hide();
    if (registration && registration->isVisible()) registration->hide();
    if (usbExporter && usbExporter->isVisible()) usbExporter->hide();

    // Finally, show channels
    if (!channels)
        channels = new ChannelsHome(hardwareVersion, softwareVersion);

    connectChannels(channels);
    channels->showFullScreen();
}


void MainWindow::startPingTimer()
{
    auto handlePingResult = [this](bool reachable) {
        if (reachable != previousStatus) {
            previousStatus = reachable;
            QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            if (reachable){
                //qDebug() << timestamp << "✅ Internet Connected";
                // Reconnect clients
                sftpHandler->reloadSettings();
                mqttHandler->reloadSettings();
                awsMqttsHandler->reloadAwsMqttsSettings();
            } else {
                //qDebug() << timestamp << "❌ Internet Disconnected";

                sftpHandler->isConnected = false;
                mqttHandler->isConnected = false;
                awsMqttsHandler->isConnected = false;
                sftpHandler->disconnectClient();
                mqttHandler->disconnectClient();
                awsMqttsHandler->disconnectAwsClient();
            }
            settings->refreshInfo();
        }
    };

    // Connect socket signals to handlePingResult
    connect(&socket, &QTcpSocket::connected, this, [this, handlePingResult]() {
        handlePingResult(true);
        socket.abort(); // close connection after success
    });

    connect(&socket, &QTcpSocket::errorOccurred, this, [this, handlePingResult](QAbstractSocket::SocketError) {
        handlePingResult(false);
        socket.abort(); // ensure socket is reset after failure
    });

    // Timer to trigger periodic pings
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        socket.abort(); // reset any previous connection
        socket.connectToHost("8.8.8.8", 53); // start async connection
    });
    timer->start(5000);
}

