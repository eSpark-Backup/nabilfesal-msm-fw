#include "rebootdialog.h"
#include "ui_rebootdialog.h"
#include <QPainterPath>
#include "utils.h"

RebootDialog::RebootDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RebootDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // apply rounded corners
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20); // radius = 20
    setMask(QRegion(path.toFillPolygon().toPolygon()));

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this] {
        this->close();
        Utils::rebootSystem();
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}


RebootDialog::~RebootDialog()
{
    delete ui;
}
