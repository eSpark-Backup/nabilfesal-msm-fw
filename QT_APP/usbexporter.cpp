#include "usbexporter.h"
#include "ui_usbexporter.h"
#include <QPushButton>
#include <QPainterPath>

UsbExporter::UsbExporter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UsbExporter)
{
    ui->setupUi(this);

    QPushButton *applyBtn = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyBtn) {
        applyBtn->setText("Load");   // Rename Apply -> Load
        applyBtn->setIcon(QIcon());  // Remove icon
    }

    QPushButton *cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelBtn) {
        cancelBtn->setIcon(QIcon());
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    // apply rounded corners
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20); // radius = 20
    setMask(QRegion(path.toFillPolygon().toPolygon()));
}

UsbExporter::~UsbExporter()
{
    delete ui;
}

void UsbExporter::setDeviceName(const QString &dev, const QString &path)
{
    ui->usb_status->setText(dev + " is mounted at " + path);
}

void UsbExporter::removeDeviceName()
{
    ui->usb_status->setText("No Mass Storage Found");
}
