#ifndef UTILS_H
#define UTILS_H

#include <QJsonObject>

namespace Utils {
void keySound();
QJsonObject getNtpStatus();
QString getDeviceIp();
QString getDeviceImei();
bool setNtpServers(const QStringList &servers);
QString getUid();
QString getMqttStatus();
QString getSftpStatus();
void rebootSystem();
}

#endif // UTILS_H
