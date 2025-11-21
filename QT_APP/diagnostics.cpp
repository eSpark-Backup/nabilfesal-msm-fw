#include "diagnostics.h"
#include "ui_diagnostics.h"

Diagnostics::Diagnostics(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Diagnostics)
{
    ui->setupUi(this);
}

Diagnostics::~Diagnostics()
{
    delete ui;
}

void Diagnostics::on_pushButton_clicked()
{
    emit diagExit();
}


void Diagnostics::on_moveRight_clicked()
{
    if(index < 4) index++;
    else index = 0;
    ui->stackedWidget->setCurrentIndex(index);
}


void Diagnostics::on_moveLeft_clicked()
{
    if(index > 0) index--;
    else index = 4;
    ui->stackedWidget->setCurrentIndex(index);
}
