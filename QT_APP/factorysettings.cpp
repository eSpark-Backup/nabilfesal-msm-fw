#include "factorysettings.h"
#include "ui_factorysettings.h"
#include "rebootdialog.h"

FactorySettings::FactorySettings(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FactorySettings)
{
    ui->setupUi(this);
    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);
}

FactorySettings::~FactorySettings()
{
    delete ui;
}

void FactorySettings::on_pushButton_clicked()
{
    emit factoryExit();
}


void FactorySettings::on_measurement_clicked()
{
    emit loadMeasurements();
}


void FactorySettings::on_system_clicked()
{
    emit loadFactSettings();
}


void FactorySettings::on_v_cal_btn_clicked()
{

}


void FactorySettings::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}


void FactorySettings::on_ration_btn_clicked()
{
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->ration_btn->setFocus();
    isRatioActivated = true;
    isVOffsetActivated = false;
    isIOffsetActivated = false;
    on_load_btn_clicked();
}


void FactorySettings::on_i_off_btn_clicked()
{
    ui->header_label->setText("I-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->i_off_btn->setFocus();
    isRatioActivated = false;
    isVOffsetActivated = false;
    isIOffsetActivated = true;
    on_load_btn_clicked();
}


void FactorySettings::on_v_off_btn_clicked()
{
    ui->header_label->setText("V-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->v_off_btn->setFocus();
    isRatioActivated = false;
    isVOffsetActivated = true;
    isIOffsetActivated = false;
    on_load_btn_clicked();
}


void FactorySettings::on_cal_btn_clicked()
{
    ui->verticalStackedWidget->setCurrentIndex(1);
    ui->cal_btn->setFocus();
}


void FactorySettings::on_i_cal_btn_clicked()
{

}

