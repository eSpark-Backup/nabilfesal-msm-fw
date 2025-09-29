#include "usbexporter.h"
#include "ui_usbexporter.h"
#include <QPushButton>
#include <QPainterPath>
#include <QMessageBox>
#include "utils.h"

UsbExporter::UsbExporter(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::UsbExporter)
{
    ui->setupUi(this);

    QPushButton *applyBtn = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyBtn) {
        applyBtn->setText("Load");   // Rename Apply -> Load
        applyBtn->setIcon(QIcon());  // Remove icon
        connect(applyBtn, &QPushButton::clicked, this, &UsbExporter::slotsAccepted);
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

    keyboard = new NumKeyboard(this);
    keyboard->move(200, 100);
    keyboard->hide();

    connect(ui->lineEdit, &QLineEdit::editingFinished, this, [=]() {
        ui->lineEdit->clearFocus();
        keyboard->hide();
    });
    ui->lineEdit->installEventFilter(this);
    this->installEventFilter(this);
}

UsbExporter::~UsbExporter()
{
    delete ui;
}

bool UsbExporter::eventFilter(QObject *obj, QEvent *event)
{
    // Then: handle QLineEdit focus/keyboard logic
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
    if (lineEdit) {
        if (event->type() == QEvent::FocusIn) {
            keyboard->setTarget(lineEdit);
            Utils::keySound();
            keyboard->show();
            lineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                keyboard->hide();
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void UsbExporter::setDeviceName(const QString &dev, const QString &path)
{
    m_device = dev.toStdString();
    m_mountPath = path.toStdString();
    ui->usb_status->setText(dev + " is mounted at " + path);
}

void UsbExporter::removeDeviceName()
{
    m_device.clear();
    m_mountPath.clear();
    ui->usb_status->setText("No Mass Storage Found");
}

int UsbExporter::getSelectedIntervalDays() const {

    if(ui->lineEdit->text().length() > 0) {
        bool ok = false;
        int val = ui->lineEdit->text().toInt(&ok);
        if(ok && val > 0) return val;
    }
    if (ui->r_7->isChecked())   return 7;
    if (ui->r_14->isChecked())  return 14;
    if (ui->r_28->isChecked())  return 28;
    if (ui->r_30->isChecked())  return 30;
    if (ui->r_60->isChecked())  return 60;
    if (ui->r_90->isChecked())  return 90;
    if (ui->r_120->isChecked()) return 120;
    if (ui->r_180->isChecked()) return 180;
    if (ui->r_240->isChecked()) return 240;
    return 0;
}

void UsbExporter::slotsAccepted()
{
    if (m_device.empty() || m_mountPath.empty()) {
        QMessageBox::warning(this, "No Device", "No USB device is connected.");
        return;
    }

    int days = getSelectedIntervalDays();
    if (days <= 0) {
        QMessageBox::warning(this, "No Interval", "Please select a valid interval.");
        return;
    }

    accept();
}
