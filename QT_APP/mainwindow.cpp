#include "mainwindow.h"
#include <QTimer>
#include "ui_mainwindow.h"
#include "login.h"
#include "channels.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , progressValue(0)
{
    ui->setupUi(this);

    // Start progress on app launch
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateProgress);
    timer->start(500);
    login = new LoginWidget;
    QObject::connect(login, &LoginWidget::loginSuccess, [&]() {
        channels = new ChannelsHome;
        channels->showFullScreen();
        login->deleteLater();
    });
    this->showFullScreen();
}

MainWindow::~MainWindow()
{
    delete ui;
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
        QTimer::singleShot(500, this, [=]() {
            login->showFullScreen();
            this->hide();

        });
    }

    progressValue = progressValue + 5;
}
