#include "factorysettings.h"
#include "ui_factorysettings.h"

FactorySettings::FactorySettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FactorySettings)
{
    ui->setupUi(this);
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

