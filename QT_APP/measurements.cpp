#include "measurements.h"
#include "ui_measurements.h"
#include "factdialog.h"
#include "rebootdialog.h"

Measurements::Measurements(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Measurements)
{
    ui->setupUi(this);
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
}

Measurements::~Measurements()
{
    delete ui;
}

void Measurements::on_back_1_clicked()
{
    emit measureExit();
}


void Measurements::on_back_2_clicked()
{
    emit measureExit();
}


void Measurements::on_pushButton_13_clicked()
{
    FactDialog dialog(this);
    dialog.setLabel(std::string("Please make sure to input the correct \n nominal voltage and reboot the system."));
    dialog.exec();
}


void Measurements::on_pushButton_14_clicked()
{
    FactDialog dialog(this);
    dialog.setLabel(std::string("Please make sure to input the correct \n nominal current and reboot the system."));
    dialog.exec();
}


void Measurements::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}


void Measurements::on_ration_btn_clicked()
{
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
}


void Measurements::on_i_off_btn_clicked()
{
    ui->header_label->setText("I-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
}


void Measurements::on_v_off_btn_clicked()
{
    ui->header_label->setText("V-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
}


void Measurements::on_cal_btn_clicked()
{
    ui->verticalStackedWidget->setCurrentIndex(1);
}

