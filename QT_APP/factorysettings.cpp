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


void FactorySettings::on_system_clicked()
{
    emit loadFactSettings();
}


void FactorySettings::on_v_cal_btn_clicked()
{
    emit calibrate(false);
}


void FactorySettings::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}


void FactorySettings::on_ration_btn_clicked()
{
    emit ctRatio();
}


void FactorySettings::on_i_off_btn_clicked()
{
    emit i_offset();
}


void FactorySettings::on_v_off_btn_clicked()
{
    emit v_offset();
}


void FactorySettings::on_i_cal_btn_clicked()
{
    emit calibrate(true);
}

