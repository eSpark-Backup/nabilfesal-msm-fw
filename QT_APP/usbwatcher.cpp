#include <QDebug>
#include "usbwatcher.h"
#include <QStorageInfo>

UsbWatcher::UsbWatcher(QObject *parent) : QObject(parent) {
    m_mountPath.clear();
    m_deviceName.clear();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &UsbWatcher::checkUsb);
    timer->start(2000); // check every 2 sec
}

void UsbWatcher::checkUsb()
{
    QString foundDevice;
    QString foundPath;

    for (const QStorageInfo &storage : QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady())
            continue;

        const QString root = storage.rootPath();
        if (!root.startsWith("/media") && !root.startsWith("/run/media"))
            continue;

        // simple FS type filter
        QString fsType = storage.fileSystemType().toLower();
        if (!(fsType.contains("fat") || fsType == "vfat" || fsType == "exfat" || fsType == "ntfs" || fsType.contains("fuse")))
            continue;

        foundDevice = QString::fromUtf8(storage.device());
        foundPath   = root;
        break; // track first USB found
    }

    if (!foundDevice.isEmpty() && m_deviceName.isEmpty()) {
        m_deviceName = foundDevice;
        m_mountPath  = foundPath;
        emit usbMounted(m_deviceName, m_mountPath);
    }
    else if (foundDevice.isEmpty() && !m_deviceName.isEmpty()) {
        emit usbUnmounted();
        m_deviceName.clear();
        m_mountPath.clear();
    }
}
