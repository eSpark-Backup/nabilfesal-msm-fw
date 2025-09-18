#include "sftpManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "utils.h"
#include <QDir>


#define SFTP_PATH "config/sftp_settings.json"
#define UPLOAD_PATH "/ToOFP/"

sftpManager::sftpManager(QObject *parent) : QObject(parent)
{
    publishTimer = new QTimer(this);
    isConnected  = false;

    process = new QProcess(this);

    // Track process state changes
    connect(process, &QProcess::started, this, [this]() {
        isConnected = true;
        qDebug() << "Connected to SFTP!";
        if (intervalSec > 0) {
            publishTimer->start(intervalSec * 1000);
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(exitCode)
        Q_UNUSED(status)
        isConnected = false;
        qDebug() << "Disconnected from SFTP.";
        publishTimer->stop();
    });

    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        isConnected = false;
        qWarning() << "SFTP process error:" << error;
        publishTimer->stop();
    });

    // Timer: only runs when connected
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "Skipped publish, SFTP not connected.";
            return;
        }

        QString localPath = "data/" + Utils::getUid();
        QDir dir;
        if (!dir.exists(localPath)) {
            dir.mkpath(localPath);
        }

        uploadFile(localPath, UPLOAD_PATH);
    });
}


bool sftpManager::loadConfig(const QString &filePath)
{
    configPath = filePath;
    QFile file(filePath);
    if (!file.exists()) {
        qWarning() << "Config file not found:" << filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON config";
        return false;
    }

    QJsonObject obj = doc.object();
    host = obj.value("ip").toString();
    username = obj.value("username").toString();
    password = obj.value("password").toString();
    QString portStr = obj.value("port").toString();
    port = portStr.toInt();
    QString intervalSecStr = obj.value("interval").toString();
    intervalSec = intervalSecStr.toInt();

    // Log the values
    qDebug() << "SFTP Settings Loaded:";
    qDebug() << "  Host:" << host;
    qDebug() << "  UserName:" << username;
    qDebug() << "  Pass:" << password;
    qDebug() << "  Port:" << port;
    qDebug() << "  Interval (sec):" << intervalSec;

    return true;
}

void sftpManager::reloadSettings()
{
    configPath = SFTP_PATH;
    if (!loadConfig(configPath)) {
        qWarning() << "No valid config found. Skipping SFTP connection.";
        return;
    }
    connectToSftp();
}

void sftpManager::connectToSftp()
{
    if (host.isEmpty()) {
        qWarning() << "SFTP settings incomplete.";
        return;
    }

    // Build sshpass + sftp command
    QString program = "sshpass";
    QStringList arguments;

    arguments << "-p" << password
              << "sftp"
              << "-P" << QString::number(port)      // pass port
              << username + "@" + host;

    // Start process
    process->start(program, arguments);
    if (!process->waitForStarted()) {
        qDebug() << "Failed to start SFTP process!";
        return;
    }

    qDebug() << "SFTP connected to" << host;
}


void sftpManager::disconnectClient()
{
    disconnectFromSftp();
}


void sftpManager::disconnectFromSftp()
{
    if (process->state() == QProcess::Running) {
        qDebug() << "Disconnecting SFTP...";
        process->write("bye\n");
        process->waitForFinished(2000);
        process->close();
    }
}


void sftpManager::uploadFile(const QString &localFile, const QString &remotePath)
{
    if (process->state() == QProcess::Running) {
        QString cmd = QString("put -r %1 %2\n").arg(localFile, remotePath);
        process->write(cmd.toUtf8());
        qDebug() << "SFTP file uploaded.";
    } else {
        qDebug() << "SFTP not connected.";
    }
}

bool sftpManager::sftpState()
{
    return isConnected;
}
