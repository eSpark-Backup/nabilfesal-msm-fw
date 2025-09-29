#ifndef UTILS_H
#define UTILS_H

#include <QJsonObject>
#include <QString>

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
void mainSound();
bool updateSerialInConfig();
QString getSignalCategoryFromFile(const QString &filePath = "/MSM/potential-pancake/MSM/bin/at_axec/responses.txt");
}

#endif // UTILS_H
