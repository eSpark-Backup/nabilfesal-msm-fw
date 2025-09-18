#pragma once
#include <QObject>
#include <QTimer>
#include <QProcess>

class sftpManager : public QObject
{
    Q_OBJECT
public:
    explicit sftpManager(QObject *parent = nullptr);

    bool loadConfig(const QString &filePath);
    void reloadSettings();
    void disconnectClient();
    bool sftpState();
    void uploadFile(const QString &localFile, const QString &remotePath);
    
private:
    void connectToSftp();
    void disconnectFromSftp();

    QProcess *process;

    // SFTP settings from JSON
    QString host;
    QString username;
    QString password;
    int port = 22;
    QString clientId;
    QString topic;
    int intervalSec = 0;
    int sheduleSec = 0100;
    QString configPath;
    bool isConnected = false;
    QTimer *publishTimer = nullptr;

};

