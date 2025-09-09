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
#include <atomic>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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

    // state
    int progressValue = 0;
    QString m_uid;
    QString m_adminPass;
    QString m_factoryPass;
    std::atomic<bool> isLogin{false};
};

#endif // MAINWINDOW_H
