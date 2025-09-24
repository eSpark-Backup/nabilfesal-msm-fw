#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "login.h"
#include "channels.h"
#include "devicesettings.h"
#include "registration.h"
#include "factorysettings.h"
#include "usbexporter.h"
#include "usbwatcher.h"
#include "factsetting.h"
#include "measurements.h"
#include "dataexporter.h"
#include "serialhandler.h"
#include "serialworker.h"
#include "mqttManager.h"
#include "awsmqttmanager.h"
#include "sftpManager.h"
#include <atomic>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QJsonObject>
#include <QMutex>
#include "commonSetting.h"

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

private:
    // helper functions for clarity & optimization
    void initConfig();
    void startProgress();
    void updateProgress();
    void handlePostProgress();
    void connectLoginSignals();

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
    UsbDataExporter *dataExporter = nullptr;
    settingsManager *settingsHandler = nullptr;
    mqttManager *mqttHandler = &mqttManager::instance();
    sftpManager *sftpHandler = &sftpManager::instance();;
    AwsMqttManager *awsMqttsHandler = &AwsMqttManager::instance();

    // state
    int progressValue = 0;
    QString m_uid;
    QString m_adminPass;
    QString m_factoryPass;
    std::atomic<bool> isLogin{false};
    std::string m_mountPath;

    QThread *serialThread = nullptr;
    SerialWorker *worker = nullptr;
    QTimer *pollTimer;
    QMap<int, QJsonObject> latestData;
    QMutex latestDataMutex;
};

#endif // MAINWINDOW_H
