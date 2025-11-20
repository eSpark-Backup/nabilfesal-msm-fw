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

