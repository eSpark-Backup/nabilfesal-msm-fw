// MainWindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include "utils.h"
#include "toast.h"
#include <QThread>
#include <QKeyEvent>
#include <QJsonObject>
#include <QMessageBox>
#include "factdialog.h"
#include <QApplication>
#include <QJsonDocument>
#include "rebootdialog.h"
#include "encryptionhelper.h"

// -----------------------------
// Singleton static pointer
// -----------------------------
MainWindow* MainWindow::m_instance = nullptr;

MainWindow* MainWindow::instance()
{
    return m_instance;
}

void MainWindow::setInstance(MainWindow* ptr)
{
    m_instance = ptr;
}

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
    setupSerial();
    setupLogin();

    // UI progress and polling
    setupProgressBar();
    setupPolling();

    channelDataWatchdog = new QTimer(this);
    channelDataWatchdog->setSingleShot(true);
    connect(channelDataWatchdog, &QTimer::timeout, this, &MainWindow::onChannelDataWatchdogTimeout);
    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);

    responseWaitTimer = new QTimer(this);
    responseWaitTimer->setSingleShot(true);

    connect(responseWaitTimer, &QTimer::timeout, this, [this]() {
        if (waitingForCalibrationResponse) {

            QJsonDocument doc = QJsonDocument::fromJson(last_cmd.toUtf8());
            QJsonObject rootObj = doc.object();

            QString command = "calibration";
            int id = -1;
            QString msg = "No response from ATSAM, trying again!";

            if (rootObj.contains("command") && rootObj["command"].isString()) {
                command = rootObj["command"].toString();
            }

            if (command == "i-calibration" && rootObj.contains("current") && rootObj["current"].isObject()) {
                QJsonObject currentObj = rootObj["current"].toObject();
                if (currentObj.contains("ID") && currentObj["ID"].isDouble()) {
                    id = currentObj["ID"].toInt();
                }
            }

            waitingForCalibrationResponse = false;
            if(cal_retry_count < 3)
            {
                cal_retry_count++;
                if(id == -1)
                    msg = QString("[ %1 ] No response from ATSAM, trying again!").arg(command);
                else
                    msg = QString("%1 [%2] No response from ATSAM, trying again!").arg(command).arg(id);

                Toast::show(msg, this, Toast::Warning);
            }
            else
            {
                 if(id == -1)
                    msg = QString("No response from ATSAM received for %1 settings.").arg(command);
                else
                    msg = QString("%1 [%2] No response from ATSAM, trying again!").arg(command).arg(id);

                Toast::show(msg, this, Toast::Error);
                enableUiButton();
                cal_retry_count = 1;
                if(BootUpSync)
                    bootFileIndex += 1;
                else
                {
                    if (pollTimer) pollTimer->start(200); return;
                }
            }
                QTimer::singleShot(1000, this, [this](){
                    BootUpSync ? sendFileWithDelay(bootFiles, bootFileIndex) :
                                        measure->requestSerialCommand(last_cmd);
                });                
        }
    });

    Utils::mainSound();
    showFullScreen();

    // Send CT/I/V ratio and offsets at startup (if config present)
    startApp = true;
    if (pollTimer) pollTimer->stop();

    QStringList files = {
        "config/v_offset_response.json",
        "config/i_offset_response.json",
        "config/ct_ratio_settings.json",
        "config/v_offset_settings.json",
        "config/i_offset_settings.json"
    };
    sendFileWithDelay(files);
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
    QDir dir(CONFIG_ADMIN_PATH);
    if (!dir.exists())
        dir.mkpath(".");

    QFile file(PROJECT_JSON_PATH);
    if (!file.exists()) {
        QJsonObject defaultSettings {
            {"uid", ""},
            {"admin", ""},
            {"factory", "admin@441322@"}
        };

        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(defaultSettings).toJson(QJsonDocument::Compact));
            file.close();
        }
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto encryptedData = file.readAll();
        file.close();
        QByteArray decryptedData = encryptedData;
        if (!EncryptionHelper::isEncrypted(file)) Utils::getUid();
        decryptedData = EncryptionHelper::readAndDecryptFile(PROJECT_JSON_PATH, Utils::getEncryptionKey());
        QJsonParseError parseError;
        QJsonDocument doc1 = QJsonDocument::fromJson(decryptedData, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc1.isObject()) {
            qDebug() << "Invalid config/project.json file format.";
        }
        const auto doc = QJsonDocument::fromJson(decryptedData);
        if (doc.isObject()) {
            const auto obj = doc.object();
            m_uid = obj.value("uid").toString();
            m_adminPass = obj.value("admin").toString();
            m_factoryPass = obj.value("factory").toString();
        }
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
    #if defined(BUILD_X86)
        worker = new SerialWorker("/tmp/qt_port", 115200);
    #elif defined(BUILD_V3S)
        worker = new SerialWorker("/dev/ttyS1", 115200);
    #endif
    worker->moveToThread(serialThread);

    connect(serialThread, &QThread::started, worker, &SerialWorker::initHandler);

    connect(this, &MainWindow::requestChannelPoll, worker, &SerialWorker::requestChannel);
    connect(this, &MainWindow::requestSerialCommand, worker, &SerialWorker::sendRawCommand);

    connect(worker, &SerialWorker::channelDataReady, this,
            &MainWindow::onChannelDataReady);

    connect(worker, &SerialWorker::genericResponseReady, this, &MainWindow::onGenericResponseReady);

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
    connect(this, &MainWindow::receivedChannelData, ch, &ChannelsHome::handleChannelData, Qt::QueuedConnection);
}

void MainWindow::connectFactSettings()
{
    connect(factSettings, &FactSetting::usbExport, this, [this]() {usbExporter->show();});

    connect(factSettings, &FactSetting::factoryExit, this, [this]() {
        if (factorySettings) {
            factorySettings->showFullScreen();
            factSettings->hide();
        }
    });

    connect(factSettings, &FactSetting::runFleetProvisioning, this,
         [this]() { factSettings->runFleetProvisioningWrapper(); }, Qt::UniqueConnection);
}


void MainWindow::connectStartUpSignals()
{
    inActWatcher = new InactivityWatcher(this);
    qApp->installEventFilter(inActWatcher);

    if (!httpsHandler)
        httpsHandler = new HttpsManager;

    if (!measure)
        measure = new Measurements;

    if(!gainCalibrator)
        gainCalibrator = new CalibrationDialog;

    if (!factorySettings)
        factorySettings = new FactorySettings(hardwareVersion, softwareVersion);

    if (!settings)
        settings = new DeviceSettings;
    
    if (!factSettings)
        factSettings = new FactSetting;

    if(!factoryMqttHandler)
        factoryMqttHandler = new mqttManager(Factory, this);

    if(!factorySftpHandler)
        factorySftpHandler = new sftpManager(Factory, this);

    if(!factoryMqttsHandler)
        factoryMqttsHandler = new AwsMqttManager(Factory, this);

    if(!adminMqttHandler)
        adminMqttHandler = new mqttManager(Admin, this);

    if(!adminSftpHandler)
        adminSftpHandler = new sftpManager(Admin, this);

    if(!adminMqttsHandler)
        adminMqttsHandler = new AwsMqttManager(Admin, this);

    if (!channels)
        channels = new ChannelsHome(hardwareVersion, softwareVersion);
    
    Utils::getEncryptionKey();
    auto showRebootDialog = [this]() {
        factoryMqttsHandler->processReboot();
        adminMqttsHandler->processReboot();
        RebootDialog reboot(this);
        reboot.exec();
    };

    connect(httpsHandler, &HttpsManager::requestFinished, this, &MainWindow::processHttpResponse);

    connect(factSettings,    &FactSetting::rebootMCM,   this, showRebootDialog);
    connect(factorySettings, &FactorySettings::rebootMCM, this, showRebootDialog);
    connect(factoryMqttsHandler, &AwsMqttManager::settingsChanged, inActWatcher, &InactivityWatcher::wakeUp);
    connect(adminMqttsHandler, &AwsMqttManager::settingsChanged, inActWatcher, &InactivityWatcher::wakeUp);

    // navigation actions for settings
    // -> when settings asks to view channels
    connect(settings, &DeviceSettings::channels_view, this, [this]() {
        isLogin = false;
        connectChannels(channels);
        channels->showFullScreen();
        settings->hide();
    });

    // -> when settings requests USB export
    connect(settings, &DeviceSettings::usbExport, this, [this]() {
        if (usbExporter) usbExporter->show();
    });

    // exit back to login
    connect(factorySettings, &FactorySettings::factoryExit, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        isLogin = false;
        login->showFullScreen();
        factorySettings->hide();
    });

    connect(factorySettings, &FactorySettings::calibrate, this, [this](bool current) {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        gainCalibrator->loadCalibrationScreen(current);
        gainCalibrator->showFullScreen();
        factorySettings->hide();
    });

    connect(gainCalibrator, &CalibrationDialog::requestSerialCommand, measure, &Measurements::requestSerialCommand);

    connect(gainCalibrator, &CalibrationDialog::exitScreen, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        gainCalibrator->hide();
        factorySettings->showFullScreen();
    });

    connect(factorySettings, &FactorySettings::ctRatio, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        factorySettings->hide();
        measure->showFullScreen();
        measure->ration_btn_clicked();
    });

    connect(factorySettings, &FactorySettings::i_offset, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        factorySettings->hide();
        measure->showFullScreen();
        measure->i_off_btn_clicked();
    });

    connect(factorySettings, &FactorySettings::v_offset, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        factorySettings->hide();
        measure->showFullScreen();
        measure->v_off_btn_clicked();
    });

    // load fact settings (factory -> factSettings)
    connect(factorySettings, &FactorySettings::loadFactSettings, this, [this]() {
        
        factSettings->load_mqtt_settings(true);
        factSettings->load_sftp_settings(true);
        factSettings->load_mqtts_settings(true);
        
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        connectFactSettings();
        factSettings->showFullScreen();
        factorySettings->hide();
    });

    connect(measure, &Measurements::measureExit, this, [this]() {
        QWidget *fw = QApplication::focusWidget();
        if (fw) fw->clearFocus();
        factorySettings->showFullScreen();
        measure->hide();
    });

    connect(measure, &Measurements::requestSerialCommand, this, [this](const QString &cmd) {
        if (pollTimer && pollTimer->isActive()) {
            pollTimer->stop();
        }
        emit requestSerialCommand(cmd);
        last_cmd = cmd;
        waitingForCalibrationResponse = true;
        if (cmd == "set-ct-ratio" || cmd == "set-v-offset" || cmd == "set-i-offset")
            CALIBRATION_DELAY = 2000;
        else
            CALIBRATION_DELAY = 6000;
        responseWaitTimer->start(CALIBRATION_DELAY);
    });
}


// -----------------------------
// connectLoginSignals (restored & cleaned)
// -----------------------------
void MainWindow::connectLoginSignals()
{
    connect(factSettings, &FactSetting::sftp_load, this, [this]() {
        factorySftpHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { factorySftpHandler->reloadSettings(); });
    });
    connect(factSettings, &FactSetting::mqtt_load, this, [this]() {
        factoryMqttHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { factoryMqttHandler->reloadSettings(); });
    });
    connect(factSettings, &FactSetting::aws_mqtts_load, this, [this]() {
    QMetaObject::invokeMethod(this, [this]() {
        factoryMqttsHandler->disconnectAwsClient();
        factSettings->checkCertificatesWrapper();
        QTimer::singleShot(1000, [this]() { factoryMqttsHandler->reloadAwsMqttsSettings(); });
        }, Qt::QueuedConnection);
    });


    connect(settings, &DeviceSettings::sftp_load, this, [this]() {
        adminSftpHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { adminSftpHandler->reloadSettings(); });
    });
    connect(settings, &DeviceSettings::mqtt_load, this, [this]() {
        adminMqttHandler->disconnectClient();
        QTimer::singleShot(1000, [this]() { adminMqttHandler->reloadSettings(); });
    });
    connect(settings, &DeviceSettings::aws_mqtts_load, this, [this]() {
    QMetaObject::invokeMethod(this, [this]() {
        adminMqttsHandler->disconnectAwsClient();
        settings->checkCertificatesWrapper();
        QTimer::singleShot(1000, [this]() { adminMqttsHandler->reloadAwsMqttsSettings(); });
        }, Qt::QueuedConnection);
    });
    connect(settings, &DeviceSettings::runFleetProvisioning, this,
        [this]() { settings->runFleetProvisioningWrapper(); }, Qt::UniqueConnection);

    connect(factoryMqttsHandler, &AwsMqttManager::rebootRequested, this, [this]() {
        this->close();
        factoryMqttsHandler->disconnectAwsClient();
        Utils::rebootSystem();
    },Qt::QueuedConnection);

    connect(adminMqttsHandler, &AwsMqttManager::rebootRequested, this, [this]() {
        this->close();
        factoryMqttsHandler->disconnectAwsClient();
        Utils::rebootSystem();
    },Qt::QueuedConnection);

    connect(factoryMqttsHandler, &AwsMqttManager::ntpConfigReceived,
            settings, &DeviceSettings::parse_load_store_ntp);
    connect(adminMqttsHandler, &AwsMqttManager::ntpConfigReceived,
            settings, &DeviceSettings::parse_load_store_ntp);

    connect(factoryMqttsHandler, &AwsMqttManager::sftpConfigReceived,  factSettings, &FactSetting::parse_load_store_sftp);
    connect(factoryMqttsHandler, &AwsMqttManager::mqttConfigReceived,  factSettings, &FactSetting::parse_load_store_mqtt);
    connect(factoryMqttsHandler, &AwsMqttManager::mqttsConfigReceived, factSettings, &FactSetting::parse_load_store_mqtts);
    connect(factoryMqttsHandler, &AwsMqttManager::updateAwsData, this, [this]() {
            factSettings->send_aws_updated_data();
            measure->send_measurement_data();
            factSettings->sendOTAData(0);
            },Qt::QueuedConnection);

    // When settings change, notify AWS manager to process them
    connect(this, &MainWindow::factory_deviceInfoChanged,
            factoryMqttsHandler, &AwsMqttManager::processDeviceInfo);
    connect(factSettings, &FactSetting::mqttSettingsChanged,
            factoryMqttsHandler, &AwsMqttManager::processMqttSettings);
    connect(factSettings, &FactSetting::sftpSettingsChanged,
            factoryMqttsHandler, &AwsMqttManager::processSftpSettings);
    connect(factSettings, &FactSetting::mqttsSettingsChanged,
            factoryMqttsHandler, &AwsMqttManager::processMqttsSettings);
    connect(factSettings, &DeviceSettings::ntpSettingsChanged,
            factoryMqttsHandler, &AwsMqttManager::processNtpSettings); // ntp is in admin but sent for factory too

    connect(adminMqttsHandler, &AwsMqttManager::sftpConfigReceived,  settings, &DeviceSettings::parse_load_store_sftp);
    connect(adminMqttsHandler, &AwsMqttManager::mqttConfigReceived,  settings, &DeviceSettings::parse_load_store_mqtt);
    connect(adminMqttsHandler, &AwsMqttManager::mqttsConfigReceived, settings, &DeviceSettings::parse_load_store_mqtts);
    connect(adminMqttsHandler, &AwsMqttManager::updateAwsData,settings,
                &DeviceSettings::send_aws_updated_data,Qt::QueuedConnection);
    
    connect(this, &MainWindow::admin_deviceInfoChanged,
            adminMqttsHandler, &AwsMqttManager::processDeviceInfo);
    connect(settings, &DeviceSettings::mqttSettingsChanged,
            adminMqttsHandler, &AwsMqttManager::processMqttSettings);
    connect(settings, &DeviceSettings::sftpSettingsChanged,
            adminMqttsHandler, &AwsMqttManager::processSftpSettings);
    connect(settings, &DeviceSettings::mqttsSettingsChanged,
            adminMqttsHandler, &AwsMqttManager::processMqttsSettings);
    connect(settings, &DeviceSettings::ntpSettingsChanged, this, [this](const NtpSettings &settings) {
        adminMqttsHandler->processNtpSettings(settings);
        factoryMqttsHandler->processNtpSettings(settings);
    });
    
    // When measurement settings change, notify AWS manager to process them
    connect(measure, &Measurements::ct_ratio_changed,
            factoryMqttsHandler, &AwsMqttManager::processCtRatio);
    connect(measure, &Measurements::v_offset_changed,
            factoryMqttsHandler, &AwsMqttManager::processVOffset);
    connect(measure, &Measurements::i_offset_changed,
            factoryMqttsHandler, &AwsMqttManager::processIOffset);

    // If AWS sends back measurement config, populate measurements' fields
    connect(factoryMqttsHandler, &AwsMqttManager::ctRatioReceived,
            measure, &Measurements::parse_load_store_ctRatio);
    connect(factoryMqttsHandler, &AwsMqttManager::vOffsetReceived,
            measure, &Measurements::parse_load_store_vOffset);
    connect(factoryMqttsHandler, &AwsMqttManager::iOffsetReceived,
            measure, &Measurements::parse_load_store_iOffset);

    connect(factSettings, &FactSetting::OTA_status, factoryMqttsHandler, &AwsMqttManager::processOTA);
    connect(factoryMqttsHandler, &AwsMqttManager::otaReceived, factSettings, &FactSetting::startOTA);

    // Wire telemetry -> managers
    connect(this, &MainWindow::receivedChannelData,
            factoryMqttHandler, &mqttManager::processIncomingJson, Qt::QueuedConnection);
    connect(this, &MainWindow::receivedChannelData,
            factorySftpHandler, &sftpManager::processIncomingJson, Qt::QueuedConnection);
    connect(this, &MainWindow::receivedChannelData,
            factoryMqttsHandler, &AwsMqttManager::processAwsTelemetry, Qt::QueuedConnection);

    connect(this, &MainWindow::receivedChannelData,
            adminMqttHandler, &mqttManager::processIncomingJson, Qt::QueuedConnection);
    connect(this, &MainWindow::receivedChannelData,
            adminSftpHandler, &sftpManager::processIncomingJson, Qt::QueuedConnection);
    connect(this, &MainWindow::receivedChannelData,
            adminMqttsHandler, &AwsMqttManager::processAwsTelemetry, Qt::QueuedConnection);

    // channel UI related signals
    connect(settings, &DeviceSettings::resetEnergy, this, [this]() {
        channels->resetAccumulatedEnergy();
        adminMqttsHandler->resetAccumulatedEnergy();
        adminMqttHandler->resetAccumulatedEnergy();
        adminSftpHandler->resetAccumulatedEnergy();
        factoryMqttsHandler->resetAccumulatedEnergy();
        factoryMqttHandler->resetAccumulatedEnergy();
        factorySftpHandler->resetAccumulatedEnergy();
        dataExporter->resetAccumulatedEnergy();
    });

    connect(channels, &ChannelsHome::energyUpdated, this,
            [this](const std::array<EnergyData, 13> &energy) {
        factoryMqttsHandler->updateAccumulatedEnergy(energy);
        factoryMqttHandler->updateAccumulatedEnergy(energy);
        factorySftpHandler->updateAccumulatedEnergy(energy);
        adminMqttsHandler->updateAccumulatedEnergy(energy);
        adminMqttHandler->updateAccumulatedEnergy(energy);
        adminSftpHandler->updateAccumulatedEnergy(energy);
        dataExporter->updateAccumulatedEnergy(energy);
    });

    channels->loadEnergyData();

    // Note:
    // Navigation-related connections (showing/hiding settings/channels/usbExport)
    // are handled where UI is shown (onAdminLogin / onFactoryLogin) to avoid double
    // connecting repeated lambdas.
    startPingTimer();

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshInfo);
    refreshTimer->start(5 * 1000);

    scheduleGSMReset();
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
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (w->inherits("QDialog")) {
                    QDialog *dlg = qobject_cast<QDialog*>(w);
                    if (dlg) dlg->reject();
                } else {
                    w->hide();
                }
            }
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
    if (waitingForCalibrationResponse) {
        waitingForCalibrationResponse = false;
        enableUiButton();
        responseWaitTimer->stop();
        cal_retry_count = 1;
    }

    if (resp.trimmed().startsWith("{") || resp.trimmed().startsWith("[")) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8(), &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            QJsonObject obj = doc.object();
            if (!BootUpSync && gainCalibrator)
                gainCalibrator->parse_and_save_response(obj);
            else
            {
                bool ok = obj.contains("message");
                Toast::show(obj[ok ? "message" : "error"].toString(),
                            this, ok ? Toast::Info : Toast::Error);
                QTimer::singleShot(1000, this, [this](){
                    sendFileWithDelay(bootFiles, bootFileIndex + 1);
                });
            }
        } else {
            // qDebug() << "Invalid JSON response:" << resp;
            // qDebug() << "Parse error:" << parseError.errorString();
        }
    }

    if (!BootUpSync && pollTimer) {
        pollTimer->start(200);
    }

}

// -----------------------------
// Login / Navigation slots
// -----------------------------
void MainWindow::onLoadChannels()
{
    connectChannels(channels);
    channels->showFullScreen();
    login->hide();
}

void MainWindow::onAdminLogin()
{
    settings->load_mqtt_settings(true);
    settings->load_sftp_settings(true);
    settings->load_mqtts_settings(true);

    isLogin = true;

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
    // Setup segmented responsibilities
    connectStartUpSignals();
    dataExporter = new SystemDataExporter(this);
    connect(this, &MainWindow::receivedChannelData, dataExporter, &SystemDataExporter::onNewChannelData, Qt::QueuedConnection);
    setupUsb();
    // Important: set connections between settings/managers
    connectLoginSignals();

    // If first setup not done, show registration UI
    if (m_uid.isEmpty() || m_adminPass.isEmpty()) {
        if (!registration) {
            registration = new Registration(hardwareVersion, softwareVersion);

            connect(registration, &Registration::registrationComplete, this, [this]() {
                httpsHandler->connect_https(this);
                connect(this, &MainWindow::homeDetected, this, &MainWindow::showChannelsHome);
                connectChannels(channels);
                channels->showFullScreen();
                registration->hide();
            });
        }

        registration->showFullScreen();
        hide();
        return;
    }

    connect(this, &MainWindow::homeDetected, this, &MainWindow::showChannelsHome);
    
    // load preset settings into DeviceSettings and FactSetting (original behavior)
    settings->load_mqtt_settings(true);
    settings->load_sftp_settings(true);
    settings->load_mqtts_settings(true);
    factSettings->load_mqtt_settings(true);
    factSettings->load_sftp_settings(true);
    factSettings->load_mqtts_settings(true);

    connectChannels(channels);
    channels->showFullScreen();
    hide();
}

void MainWindow::sendFileWithDelay(const QStringList &files, int index)
{
    BootUpSync = true;
    bootFiles = files;
    bootFileIndex = index;
    if (index >= files.size()) {
        BootUpSync = false;
        startApp = false;
        responseWaitTimer->stop();
        if (pollTimer) pollTimer->start(200);
        return;
    }

    QString filePath = files[index];
    QFile file(filePath);

    if (!file.exists()) {
        qWarning() << "File not found, skipping:" << filePath;
        sendFileWithDelay(files, index + 1);
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file, skipping:" << filePath;
        sendFileWithDelay(files, index + 1);
        return;
    }


    EncryptionHelper::check_and_encrypt_files(filePath, filePath.toUpper(), true, this);
    QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(filePath, Utils::getEncryptionKey());
    QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
    if (!doc.isNull() && doc.isObject()) {
        QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

        if (pollTimer && pollTimer->isActive())
            pollTimer->stop();

        emit requestSerialCommand(jsonString);
        waitingForCalibrationResponse = true;
        CALIBRATION_DELAY = index > 1? 2000 : 6000;
        responseWaitTimer->start(CALIBRATION_DELAY);
    } else {
        sendFileWithDelay(files, index + 1);
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

    QTimer::singleShot(5000, this, [this](){
        startApp = true;

        QStringList files = {
            "config/v_offset_response.json",
            "config/i_offset_response.json",
            "config/ct_ratio_settings.json",
            "config/v_offset_settings.json",
            "config/i_offset_settings.json"
        };
        sendFileWithDelay(files);
    });    

    channelDataWatchdog->start(WATCHDOG_TIMEOUT_MS);
}


bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // First: handle key events globally
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Home && Utils::getUid() != "") {
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
                factorySftpHandler->reloadSettings();
                factoryMqttHandler->reloadSettings();
                factoryMqttsHandler->reloadAwsMqttsSettings();
                adminSftpHandler->reloadSettings();
                adminMqttHandler->reloadSettings();
                adminMqttsHandler->reloadAwsMqttsSettings();
            } else {
                //qDebug() << timestamp << "❌ Internet Disconnected";

                factorySftpHandler->isConnected = false;
                factoryMqttHandler->isConnected = false;
                factoryMqttsHandler->isConnected = false;
                factorySftpHandler->disconnectClient();
                factoryMqttHandler->disconnectClient();
                adminMqttsHandler->disconnectAwsClient();
                adminSftpHandler->isConnected = false;
                adminMqttHandler->isConnected = false;
                adminMqttsHandler->isConnected = false;
                adminSftpHandler->disconnectClient();
                adminMqttHandler->disconnectClient();
                adminMqttsHandler->disconnectAwsClient();
            }
            refreshInfo();
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


void MainWindow::refreshInfo()
{
    bool sendBothSignal = false;
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
    curDevInfo.date      = ntpData.value("date").toString();
    curDevInfo.gsmStatus = Utils::getSignalCategoryFromFile();
    curDevInfo.ntpStatus = ntpData.value("ntp_status").toString();

    if (isDifferent(curDevInfo, prvDevInfo)) {
        sendBothSignal = true;
        prvDevInfo = curDevInfo;
    }

    curProtState_A.mqttStatus = Utils::getMqttStatus(Mode::Admin);
    curProtState_A.sftpStatus = Utils::getSftpStatus(Mode::Admin);
    curProtState_A.mqttsStatus = Utils::getMqttsStatus(Mode::Admin);
    if (sendBothSignal || isConnectionDifferent(curProtState_A, prvProtState_A))
    {        
        curDevInfo.conn = curProtState_A;
        emit admin_deviceInfoChanged(curDevInfo);
        settings->updateInfo(curDevInfo);
        prvProtState_A = curProtState_A;
    }

    curProtState_F.mqttStatus = Utils::getMqttStatus(Mode::Factory);
    curProtState_F.sftpStatus = Utils::getSftpStatus(Mode::Factory);
    curProtState_F.mqttsStatus = Utils::getMqttsStatus(Mode::Factory);

    if (sendBothSignal || isConnectionDifferent(curProtState_F, prvProtState_F))
    {    
        curDevInfo.conn = curProtState_F;
        emit factory_deviceInfoChanged(curDevInfo);
        factSettings->updateInfo(curDevInfo);
        prvProtState_F = curProtState_F;
    }

    if(refreshTimer != nullptr) refreshTimer->start(10 * 1000);
}


bool MainWindow::isDifferent(const DeviceInfo &a, const DeviceInfo &b)
{
    return a.uuid       != b.uuid
        || a.pass       != b.pass
        || a.ip         != b.ip
        || a.imei       != b.imei
        || a.date       != b.date
        || a.ntpStatus  != b.ntpStatus
        || a.gsmStatus != b.gsmStatus;
}


bool MainWindow::isConnectionDifferent(const ConnStatus &a, const ConnStatus &b)
{
    return a.mqttStatus != b.mqttStatus
        || a.sftpStatus != b.sftpStatus
        || a.mqttsStatus != b.mqttsStatus;
}


void MainWindow::enableUiButton()
{
    if(gainCalibrator) gainCalibrator->enableButton();
    if(measure) measure->enableButton();
}


void MainWindow::scheduleGSMReset()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime scheduledTime = nextSundayAt0400();

    qint64 msecToGo = now.msecsTo(scheduledTime);

    qDebug() << "Next GSM reset scheduled at:"
             << scheduledTime
             << "(in" << msecToGo / 1000 << "seconds)";

    QTimer::singleShot(msecToGo, this, [this]() {

        QString script_path = "/etc/init.d/gsm";
        Toast::show("Resetting GSM!", this, Toast::Info);
        QProcess::execute(script_path, {"restart"});
        scheduleGSMReset(); // Reschedule next Sunday
    });
}


QDateTime MainWindow::nextSundayAt0400()
{
    QDateTime now = QDateTime::currentDateTime();

    QDate today = now.date();
    int daysUntilSunday =
        (7 - today.dayOfWeek()) % 7; // Qt: Monday=1, Sunday=7

    if (daysUntilSunday == 0 && now.time() >= QTime(4, 0))
        daysUntilSunday = 7;

    QDate nextSunday = today.addDays(daysUntilSunday);
    return QDateTime(nextSunday, QTime(4, 0), Qt::LocalTime);
}


bool MainWindow::isValidConfigBlock(const QJsonObject &root, const QString &key)
{
    if (!root.contains(key))
        return false;

    QJsonValue v = root.value(key);
    if (!v.isObject())
        return false;

    return !v.toObject().isEmpty();
}


void MainWindow::processHttpResponse(const QJsonObject &respObj, int httpCode)
{
    QJsonObject root = respObj.value("data").toObject();

    if (isValidConfigBlock(root, "ntp")) {
        settings->parse_ntp_from_http(root.value("ntp").toObject());
    }

    if (isValidConfigBlock(root, "mqtt")) {
        factSettings->parse_mqtt_from_http(root.value("mqtt").toObject());
    }

    if (isValidConfigBlock(root, "sftp")) {
        factSettings->parse_sftp_from_http(root.value("sftp").toObject());
    }

    if (isValidConfigBlock(root, "mqtts")) {
        factSettings->parse_mqtts_from_http(root.value("mqtts").toObject());
    }

    if (isValidConfigBlock(root, "ct_ratio")) {
        measure->parse_ctRatio_from_http(root.value("ct_ratio").toObject());
    }

    if (isValidConfigBlock(root, "v_offset")) {
        measure->parse_vOffset_from_http(root.value("v_offset").toObject());
    }

    if (isValidConfigBlock(root, "i_offset")) {
        measure->parse_iOffset_from_http(root.value("i_offset").toObject());
    }
}
