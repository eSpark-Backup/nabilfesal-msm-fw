#ifndef UTILS_H
#define UTILS_H

#include <QJsonObject>
#include <QString>
#include <QLabel>
#include "common_headers.h"

namespace Utils {
void keySound();
QJsonObject getNtpStatus();
QString getDeviceIp();
QString getDeviceImei();
bool setNtpServers(const QStringList &servers);
QString getUid();
QString getDevPassword();
QString getMqttStatus(Mode mode);
QString getSftpStatus(Mode mode);
QString getMqttsStatus(Mode mode);
void rebootSystem();
void mainSound();
bool updateSerialInConfig(const QString &AwsEp, const QString &configJson_path);
QString getSignalCategoryFromFile(const QString &filePath = "/MSM/potential-pancake/MSM/bin/at_axec/responses.txt");
QString extractImeiFromResponses(const QString &filePath);
QLabel *showImageFullscreenFor(const QString &imagePath, int milliseconds);
QString getFirmwareVersion();
QString getHardwareVersion();
QString getLastOTAInitiated();
QStringList getAvailableVersions();
void setLastOTAInitiated();
QJsonObject readJsonFile(const QString &filePath);
void startAlignedTimer( QObject* context, QTimer* timer,
    int intervalSeconds, std::function<void()> firstTickCallback = nullptr);
}

#endif // UTILS_H
