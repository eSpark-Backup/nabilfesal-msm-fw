#include "usbwatcher.h"
#include <QDebug>

UsbWatcher::UsbWatcher(QObject *parent) : QObject(parent) {
    m_mountPath.clear();
    m_deviceName.clear();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &UsbWatcher::checkUsb);
    timer->start(2000); // check every 2 sec
}

void UsbWatcher::checkUsb() {
    QFile file("/proc/mounts");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QString foundDevice;
    QString foundPath;

    QString line = file.readAll();
    QStringList lines = line.split('\n');
    for (const QString &l : lines) {
        if (l.contains("/media/")) {
            QStringList parts = l.split(" ");
            if (parts.size() >= 2) {
                foundDevice = parts[0];
                foundPath   = parts[1];
                break;
            }
        }
    }

    // case: new device plugged in
    if (!foundDevice.isEmpty() && m_deviceName.isEmpty()) {
        m_deviceName = foundDevice;
        m_mountPath  = foundPath;
        emit usbMounted(m_deviceName, m_mountPath);
    }
    // case: device removed
    else if (foundDevice.isEmpty() && !m_deviceName.isEmpty()) {
        emit usbUnmounted();
        m_deviceName.clear();
        m_mountPath.clear();
    }
    // case: still same device (do nothing)
}
