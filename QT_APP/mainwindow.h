#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QJsonObject>
#include <QMutex>
#include <atomic>

#include "login.h"
#include "channels.h"
#include "devicesettings.h"
#include "registration.h"
#include "factorysettings.h"
#include "usbexporter.h"
#include "usbwatcher.h"
#include "factsetting.h"
#include "measurements.h"
#include "serialworker.h"
#include "mqttManager.h"
#include "awsmqttmanager.h"
#include "sftpManager.h"
#include "commonSetting.h"
#include "systemexporter.h"
#include "calibrationdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void requestChannelPoll(int channel);
    void receivedChannelData(int channel, const QJsonObject &data);
    void requestSerialCommand(const QString &cmd);
    void homeDetected();

private slots:
    // USB handling
    void onUsbMounted(const QString &dev, const QString &path);
    void onUsbUnmounted();
    void onUsbExportAccepted();

    // Serial handling
    void onChannelDataReady(int ch, const QJsonObject &obj);
    void onGenericResponseReady(const QString &resp);

    // Login / navigation
    void onLoadChannels();
    void onAdminLogin();
    void onFactoryLogin();
    void onChannelsExit();

    // Progress management
    void updateProgress();
    void setupProgressBar();

    void onChannelDataWatchdogTimeout();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // Setup helpers
    void initConfig();
    void setupLogin();
    void setupUsb();
    void setupSerial();
    void setupPolling();
    void connectLoginSignals();
    void startProgress();
    void handlePostProgress();
    void showChannelsHome();
    void startPingTimer();

    // Utility connection helpers
    void connectChannels(ChannelsHome *ch);
    void connectFactSettings(FactSetting *fs);

    void sendRatioSettings();

private:
    Ui::MainWindow *ui;

    // major UI components (created lazily, reused instead of deleted/recreated)
    LoginWidget *login = nullptr;
    ChannelsHome *channels = nullptr;
    DeviceSettings *settings = nullptr;
    Registration *registration = nullptr;
    FactorySettings *factorySettings = nullptr;
    UsbExporter *usbExporter = nullptr;
    UsbWatcher *watcher = nullptr;
    FactSetting *factSettings = nullptr;
    Measurements *measure = nullptr;
    SystemDataExporter *dataExporter = nullptr;
    CalibrationDialog *gainCalibrator = nullptr;

    // managers (mostly singletons)
    settingsManager *settingsHandler = nullptr;
    mqttManager *mqttHandler = &mqttManager::instance();
    sftpManager *sftpHandler = &sftpManager::instance();
    AwsMqttManager *awsMqttsHandler = &AwsMqttManager::instance();

    // state
    int progressValue = 0;
    QString m_uid;
    QString m_adminPass;
    QString m_factoryPass;
    QString hardwareVersion;
    QString softwareVersion;
    std::atomic<bool> isLogin{false};
    std::atomic<bool> startApp{true};
    std::atomic< bool> usbDetected{false};

    std::string m_mountPath;
    QThread *serialThread = nullptr;
    SerialWorker *worker = nullptr;
    QTimer *pollTimer = nullptr;

    QTimer *channelDataWatchdog;
    const int WATCHDOG_TIMEOUT_MS = 50000;

    QTimer *responseWaitTimer = nullptr;
    bool waitingForCalibrationResponse = false;

    // Ping Timer declarations 
    bool previousStatus = false;
    QTcpSocket socket;
};

#endif // MAINWINDOW_H
