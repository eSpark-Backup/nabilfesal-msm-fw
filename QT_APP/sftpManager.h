#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QReadWriteLock>
#include <QMap>
#include <QJsonObject>
#include "common_headers.h"

class sftpManager : public QObject
{
    Q_OBJECT
public:
    explicit sftpManager(Mode mode, QObject *parent = nullptr);
    sftpManager(const sftpManager&) = delete;
    bool loadConfig(const QString &filePath);
    void reloadSettings();
    void disconnectClient();
    bool sftpState();
    void uploadFile(const QString &localFile, const QString &remotePath);
    bool isConnected = false;
    
public slots:
    void processIncomingJson(int channel, const QJsonObject &payload);
    void resetAccumulatedEnergy();
    void updateAccumulatedEnergy(const std::array<EnergyData,13> &energyData);

private:
    void connectToSftp();
    void disconnectFromSftp();
    void scheduleSftpUpload();
    void exportTelemetryToCsv(const QString &filePath);

    QProcess *process;

    // SFTP settings from JSON
    QString host;
    QString username;
    QString password;
    int port = 22;
    QString clientId;
    QString topic;
    int intervalSec = 0;
    QString schedule = "0100";
    QString configPath;
    QTimer *publishTimer = nullptr;
    QString m_filePath;

    mutable QReadWriteLock telemetryLock;
    QMap<int, QJsonObject> channelTelemetry;
    QTimer *reconnectTimer;

    // Assume channel numbers start at 1
    std::array<EnergyData, 13> sftpEnergy;
    Mode clientMode;

public:
    QString tag;
};

