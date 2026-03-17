#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <atomic>
#include <QThread>
#include <QJsonObject>

#include "login.h"
#include "channels.h"
#include "usbwatcher.h"
#include "usbexporter.h"
#include "mqttManager.h"
#include "factsetting.h"
#include "sftpManager.h"
#include "registration.h"
#include "measurements.h"
#include "serialworker.h"
#include "commonSetting.h"
#include "awsmqttmanager.h"
#include "systemexporter.h"
#include "devicesettings.h"
#include "factorysettings.h"
#include "calibrationdialog.h"
#include "inactivitywatcher.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow* instance();
    static void setInstance(MainWindow* ptr);
    explicit MainWindow(QWidget *parent = nullptr);
    void sendFileWithDelay(const QStringList &files, int index = 0);
    ~MainWindow();
    void refreshInfo(bool isOnline);
    DeviceInfo curDevInfo, prvDevInfo;
    ConnStatus curProtState_A, prvProtState_A, curProtState_F, prvProtState_F;

signals:
    void requestChannelPoll(int channel);
    void receivedChannelData(int channel, const QJsonObject &data);
    void requestSerialCommand(const QString &cmd);
    void homeDetected();
    void enableSendButton();
    void admin_deviceInfoChanged(const DeviceInfo &info);
    void factory_deviceInfoChanged(const DeviceInfo &info);

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
    void onUsbLogin();
    void onChannelsExit();

    // Progress management
    void updateProgress();
    void setupProgressBar();

    void onChannelDataWatchdogTimeout();
    void enableUiButton();

    void processHttpResponse(const QJsonObject &respObj, int httpCode);

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
    void connectFactSettings();
    void connectStartUpSignals();

    bool isDifferent(const DeviceInfo &a, const DeviceInfo &b);
    bool isConnectionDifferent(const ConnStatus &a, const ConnStatus &b);
    bool isValidConfigBlock(const QJsonObject &root, const QString &key);

    void scheduleGSMReset();
    QDateTime nextSundayAt0400();

public:
    mqttManager *adminMqttHandler = nullptr;
    sftpManager *adminSftpHandler = nullptr;
    AwsMqttManager *adminMqttsHandler = nullptr;

    mqttManager *factoryMqttHandler = nullptr;
    sftpManager *factorySftpHandler = nullptr;
    AwsMqttManager *factoryMqttsHandler = nullptr;

private:
    static MainWindow* m_instance;
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
    InactivityWatcher *inActWatcher = nullptr;
    settingsManager *settingsHandler = nullptr;

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
    const int WATCHDOG_TIMEOUT_MS = 100000;
    int CALIBRATION_DELAY = 6000;
    uint8_t cal_retry_count = 1;
    QString last_cmd;

    QTimer *responseWaitTimer = nullptr;
    bool waitingForCalibrationResponse = false;

    // Ping Timer declarations 
    bool previousStatus = false;
    QTcpSocket socket;

    // Boot calibration declarations 
    bool BootUpSync = true;
    QStringList bootFiles;
    int bootFileIndex = 0;

    QTimer *refreshTimer = nullptr;    
};

#endif // MAINWINDOW_H
