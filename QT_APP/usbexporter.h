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
    int getSelectedIntervalDays() const;

private slots:
    void slotsAccepted();

private:
    Ui::UsbExporter *ui;
    std::string m_device;
    std::string m_mountPath;
};

#endif // USBEXPORTER_H
