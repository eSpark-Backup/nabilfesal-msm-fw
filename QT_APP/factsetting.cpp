#include "factsetting.h"
#include "ui_factsetting.h"
#include "rebootdialog.h"

FactSetting::FactSetting(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FactSetting)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
}

FactSetting::~FactSetting()
{
    delete ui;
}

void FactSetting::on_next_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}


void FactSetting::on_back_btn_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}


void FactSetting::on_exp_btn_clicked()
{
    emit usbExport();
}


void FactSetting::on_exit_btn_clicked()
{
    emit factoryExit();
}


void FactSetting::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}

