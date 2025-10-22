#include "factdialog.h"
#include "ui_factdialog.h"
#include <QPainterPath>

FactDialog::FactDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FactDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

FactDialog::~FactDialog()
{
    delete ui;
}

void FactDialog::setLabel(const std::string& data)
{
    ui->label->setText(QString::fromStdString(data));
}
