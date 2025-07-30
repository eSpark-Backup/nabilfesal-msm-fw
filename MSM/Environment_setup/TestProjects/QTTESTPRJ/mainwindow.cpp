#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onPushButton1Clicked);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::onPushButton2Clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPushButton1Clicked()
{
    qDebug() << "Button 1 pressed!";
    system("echo Button1 pressed > /tmp/button_log.txt");
}

void MainWindow::onPushButton2Clicked()
{
    qDebug() << "Button 2 pressed!";
    system("reboot");  // Example: reboot device
}
