#ifndef USBWATCHER_H
#define USBWATCHER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QString>

class UsbWatcher : public QObject {
    Q_OBJECT
public:
    explicit UsbWatcher(QObject *parent = nullptr);

    QString currentMountPath() const { return m_mountPath; }
    QString currentDeviceName() const { return m_deviceName; }

signals:
    void usbMounted(const QString &device, const QString &path);
    void usbUnmounted();

private slots:
    void checkUsb();

private:
    QString m_mountPath;   // e.g. /media/usb
    QString m_deviceName;  // e.g. /dev/sda1
};

#endif // USBWATCHER_H
