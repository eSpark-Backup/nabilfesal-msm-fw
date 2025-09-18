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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      progressValue(0),
      login(nullptr),
      settings(nullptr),
      factorySettings(nullptr),
      channels(nullptr),
      registration(nullptr)
{
    ui->setupUi(this);

    initConfig();

    // create login once
    login = new LoginWidget;
    usbExporter = new UsbExporter(this);
    connectLoginSignals();

    ui->progressBar->setValue(progressValue);
    ui->process_info->setText("Initializing...");

    watcher = new UsbWatcher(this);

    connect(watcher, &UsbWatcher::usbMounted, this, [=](const QString &dev, const QString &path){
        usbExporter->setDeviceName(dev, path);
        if (isLogin)
        {
            usbExporter->show();
        }

    });

    connect(watcher, &UsbWatcher::usbUnmounted, this, [=](){
        usbExporter->removeDeviceName();

    });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    timer->start(500);

    Utils::keySound();
    showFullScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
    // no need to delete children separately, Qt parent-child takes care
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

    progressValue += 10;
}

void MainWindow::handlePostProgress()
{
    if (m_uid.isEmpty() || m_adminPass.isEmpty()) {
        if (!registration) {
            registration = new Registration;
            connect(registration, &Registration::registrationComplete, this, [this]() {
                login->showFullScreen();
                registration->hide();
            });
        }
        registration->showFullScreen();
        hide();
    } else {
        if (!settings) {
            settings = new DeviceSettings();
        }
        settings->load_and_start_mqtt_data();
        login->showFullScreen();
        hide();
    }
}

void MainWindow::connectLoginSignals()
{
    connect(login, &LoginWidget::adminLoginSuccess, this, [this]() {
        isLogin = true;
        if (!settings) {
            settings = new DeviceSettings;
            connect(settings, &DeviceSettings::channels_view, this, [this]() {
                if (!channels) {
                    channels = new ChannelsHome;
                    connect(channels, &ChannelsHome::channelsExit, this, [this]() {
                        isLogin = false;
                        login->showFullScreen();
                        channels->hide();
                        settings->hide();
                    });
                    connect(channels, &ChannelsHome::deviceInfo, this, [this]() {
                        settings->showFullScreen();
                        channels->hide();
                    });
                }
                channels->showFullScreen();
                settings->hide();
            });
            connect(settings, &DeviceSettings::usbExport, this, [this](){
                usbExporter->show();
            });
        }
        settings->showFullScreen();
        login->hide();
    });

    connect(login, &LoginWidget::factoryLoginSuccess, this, [this]() {
        isLogin = true;
        if (!factorySettings) {
            factorySettings = new FactorySettings;
            connect(factorySettings, &FactorySettings::factoryExit, this, [this]() {
                isLogin = false;
                login->showFullScreen();
                factorySettings->hide();
            });
        }
        factorySettings->showFullScreen();
        login->hide();
    });
}
