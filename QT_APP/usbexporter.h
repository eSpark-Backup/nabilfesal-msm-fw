#ifndef USBEXPORTER_H
#define USBEXPORTER_H

#include <QDialog>

namespace Ui {
class UsbExporter;
}

class UsbExporter : public QDialog
{
    Q_OBJECT

public:
    explicit UsbExporter(QWidget *parent = nullptr);
    ~UsbExporter();
    void setDeviceName(const QString &dev, const QString &path);
    void removeDeviceName();

private:
    Ui::UsbExporter *ui;
};

#endif // USBEXPORTER_H
