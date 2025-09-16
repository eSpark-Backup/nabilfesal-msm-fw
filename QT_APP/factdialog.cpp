#include "factdialog.h"
#include "ui_factdialog.h"
#include <QPainterPath>

FactDialog::FactDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FactDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // apply rounded corners
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20); // radius = 20
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

FactDialog::~FactDialog()
{
    delete ui;
}

void FactDialog::setLabel(const std::string& data)
{
    ui->label->setText(QString::fromStdString(data));
}
