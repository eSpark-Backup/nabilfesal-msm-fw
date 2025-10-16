#include "sftpManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "utils.h"
#include <QDir>
#include <QThread>


#define SFTP_PATH "config/sftp_settings.json"


sftpManager& sftpManager::instance()
{
    static sftpManager _instance;  // created on first call, shared afterwards
    return _instance;
}


sftpManager::sftpManager(QObject *parent) : QObject(parent)
{
    // existing code ...

    publishTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);
    isConnected  = false;

    process = new QProcess(this);

    reconnectTimer->setInterval(5 * 60 * 1000);   // 5 minute
    reconnectTimer->setSingleShot(false);     // keep retrying

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) {
            qWarning() << "Retrying SFTP connection...";
            connectToSftp();
        }
    });

    // Process connected
    connect(process, &QProcess::started, this, [this]() {
        isConnected = true;
        qDebug() << "Connected to SFTP!";
        reconnectTimer->stop();   // stop retries when connected
        scheduleSftpUpload();
        publishTimer->start(intervalSec * 1000);
    });

    // Process disconnected
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(exitCode)
        Q_UNUSED(status)
        isConnected = false;
        publishTimer->stop();
        qDebug() << "Disconnected from SFTP.";
        if (!reconnectTimer->isActive()) {
            reconnectTimer->start();   // start retry loop
        }
    });

    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        isConnected = false;
        publishTimer->stop();
        qWarning() << "SFTP process error:" << error;
        if (!reconnectTimer->isActive()) {
            reconnectTimer->start();   // start retry loop
        }
    });

    // Publish CSV timer
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (m_filePath.isEmpty()) {
            QString uid = Utils::getUid();
            QDir dir("data/" + uid);
            dir.mkpath(".");
            m_filePath = QString("data/%1/%1-%2.csv")
                            .arg(uid)
                            .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmm"));
        }
        exportTelemetryToCsv(m_filePath);
    });
}


void sftpManager::scheduleSftpUpload()
{
    if (!isConnected) {
        qWarning() << "Skipped scheduling, SFTP not connected.";
        return;
    }

    QString hhmm = this->schedule;
    if (hhmm.length() != 4) {
        qWarning() << "Invalid schedule format:" << hhmm;
        return;
    }

    int hour   = hhmm.left(2).toInt();
    int minute = hhmm.mid(2, 2).toInt();

    // Make both explicitly LocalTime
    QDateTime now = QDateTime::currentDateTime().toLocalTime();
    QDateTime scheduledTime(QDate::currentDate(), QTime(hour, minute), Qt::LocalTime);

    // Only roll to tomorrow if time already passed
    if (scheduledTime < now) {
        scheduledTime = scheduledTime.addDays(1);
    }

    qint64 msecToGo = now.msecsTo(scheduledTime);
    qDebug() << "Next SFTP upload scheduled at:" << scheduledTime
             << "(in" << msecToGo / 1000 << "seconds)";

    QTimer::singleShot(msecToGo, this, [this]() {
        QString uid = Utils::getUid();
        QString uploadFilePath = "/FromOFP/" + uid;

        uploadFile(m_filePath, uploadFilePath);

        // deleting the uploaded file
        if (QFile::remove(m_filePath)) {
            m_filePath = ""; // reset for next file
        }

        publishTimer->start(intervalSec  * 1000);

        QThread::sleep(1);
        scheduleSftpUpload(); // reschedule
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
    schedule = obj.value("schedule").toString();

    // // Log the values
    // qDebug() << "SFTP Settings Loaded:";
    // qDebug() << "  Host:" << host;
    // qDebug() << "  UserName:" << username;
    // qDebug() << "  Pass:" << password;
    // qDebug() << "  Port:" << port;
    // qDebug() << "  Interval (sec):" << intervalSec;

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

    QString program = "sshpass";
    QStringList arguments;
    arguments << "-p" << password
              << "sftp"
              << "-oStrictHostKeyChecking=no"
              << "-P" << QString::number(port)
              << username + "@" + host;

    process->start(program, arguments);
    if (!process->waitForStarted(5000)) {
        qWarning() << "Failed to start SFTP process!";
        qWarning() << "Program:" << program << "Args:" << arguments;
        return;
    }

    qDebug() << "SFTP process started for" << host;
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
    publishTimer->stop();

    if (host.isEmpty() || username.isEmpty() || password.isEmpty()) {
        qWarning() << "SFTP config missing.";
        return;
    }

    QFileInfo fi(localFile);
    if (!fi.exists() || !fi.isFile()) {
        qWarning() << "Local file missing:" << localFile;
        return;
    }

    QString absLocal   = fi.absoluteFilePath();
    QString remoteFile = remotePath + "/" + fi.fileName();

    QStringList arguments;
    arguments << "-p" << password
              << "sftp"
              << "-oStrictHostKeyChecking=no"
              << "-P" << QString::number(port)
              << username + "@" + host;

    QProcess sftpProc;
    sftpProc.start("sshpass", arguments);

    if (!sftpProc.waitForStarted(5000)) {
        qWarning() << "Failed to start SFTP process!";
        return;
    }

    // Batch commands
    QByteArray batch;
    batch.append(QString("mkdir %1\n").arg(remotePath).toUtf8());
    batch.append(QString("put %1 %2\n").arg(absLocal, remoteFile).toUtf8());
    batch.append("bye\n");

    sftpProc.write(batch);
    sftpProc.closeWriteChannel();

    sftpProc.waitForFinished(180000);
}


bool sftpManager::sftpState()
{
    return isConnected;
}

void sftpManager::processIncomingJson(int channel, const QJsonObject &payload)
{
    QJsonObject root = payload;
    QJsonObject data = root["data"].toObject();

    QJsonObject phaseA = data["phase_a"].toObject();
    QJsonObject phaseB = data["phase_b"].toObject();
    QJsonObject phaseC = data["phase_c"].toObject();
    QJsonObject telemetry;

    if(channel == 1)
    {
        // =============== INCOMING CHANNEL =================
        telemetry["V_R"]     = phaseA["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_R1"]  = phaseA["general"].toObject()["current"].toDouble();
        telemetry["In1_RPF"] = phaseA["power"].toObject()["factor"].toDouble();

        telemetry["V_Y"]     = phaseB["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_Y1"]  = phaseB["general"].toObject()["current"].toDouble();
        telemetry["In1_YPF"] = phaseB["power"].toObject()["factor"].toDouble();

        telemetry["V_B"]     = phaseC["general"].toObject()["rms_voltage"].toDouble();
        telemetry["In1_B1"]  = phaseC["general"].toObject()["current"].toDouble();
        telemetry["In1_BPF"] = phaseC["power"].toObject()["factor"].toDouble();

        // Power
        double P1_R = phaseA["power"].toObject()["active"].toDouble();
        double P1_B = phaseB["power"].toObject()["active"].toDouble();
        double P1_Y = phaseC["power"].toObject()["active"].toDouble();
        telemetry["P1_R"] = P1_R;
        telemetry["P1_B"] = P1_B;
        telemetry["P1_Y"] = P1_Y;
        telemetry["P1"]   = P1_R + P1_B + P1_Y;

        // Reactive
        double Q1_R = phaseA["power"].toObject()["reactive"].toDouble();
        double Q1_B = phaseB["power"].toObject()["reactive"].toDouble();
        double Q1_Y = phaseC["power"].toObject()["reactive"].toDouble();
        telemetry["Q1_R"] = Q1_R;
        telemetry["Q1_B"] = Q1_B;
        telemetry["Q1_Y"] = Q1_Y;
        telemetry["Q1"]   = Q1_R + Q1_B + Q1_Y;

        // Apparent
        double S1_R = phaseA["power"].toObject()["apparent"].toDouble();
        double S1_B = phaseB["power"].toObject()["apparent"].toDouble();
        double S1_Y = phaseC["power"].toObject()["apparent"].toDouble();
        telemetry["S1_R"] = S1_R;
        telemetry["S1_B"] = S1_B;
        telemetry["S1_Y"] = S1_Y;
        telemetry["S1"]   = S1_R + S1_B + S1_Y;

        // Energy
        QJsonObject energyA = phaseA["energy"].toObject();
        QJsonObject activeA   = energyA["active"].toObject();
        QJsonObject reactiveA = energyA["reactive"].toObject();

        telemetry["T_C"]    = root["temperature"].toDouble();
        telemetry["Epimp1"] = activeA["negative"].toDouble();
        telemetry["Epexp1"] = activeA["positive"].toDouble();
        telemetry["Eqimp1"] = reactiveA["negative"].toDouble();
        telemetry["Eqexp1"] = reactiveA["positive"].toDouble();
    }
    else
    {
        // =============== OUTGOING CHANNELS =================
          // Outgoing channels: F(channel-1)_ prefix
        int idx = channel - 1;  // Channel 2 → F1, Channel 3 → F2, … Channel 12 → F11
        QString prefix = QString("F%1_").arg(idx);

        double P_R = phaseA["power"].toObject()["active"].toDouble();
        double P_B = phaseB["power"].toObject()["active"].toDouble();
        double P_Y = phaseC["power"].toObject()["active"].toDouble();

        double Q_R = phaseA["power"].toObject()["reactive"].toDouble();
        double Q_B = phaseB["power"].toObject()["reactive"].toDouble();
        double Q_Y = phaseC["power"].toObject()["reactive"].toDouble();

        double S_R = phaseA["power"].toObject()["apparent"].toDouble();
        double S_B = phaseB["power"].toObject()["apparent"].toDouble();
        double S_Y = phaseC["power"].toObject()["apparent"].toDouble();

        QJsonObject energyA   = phaseA["energy"].toObject();
        QJsonObject activeA   = energyA["active"].toObject();
        QJsonObject reactiveA = energyA["reactive"].toObject();

        telemetry[prefix + "R"]     = phaseA["general"].toObject()["current"].toDouble();
        telemetry["PF_R" + QString::number(idx)] = phaseA["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "Y"]     = phaseB["general"].toObject()["current"].toDouble();
        telemetry["PF_Y" + QString::number(idx)] = phaseB["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "B"]     = phaseC["general"].toObject()["current"].toDouble();
        telemetry["PF_B" + QString::number(idx)] = phaseC["power"].toObject()["factor"].toDouble();

        telemetry[prefix + "P"] = P_R + P_B + P_Y;
        telemetry[prefix + "Q"] = Q_R + Q_B + Q_Y;
        telemetry[prefix + "S"] = S_R + S_B + S_Y;

        telemetry[prefix + "T"]     = root["temperature"].toDouble();
        telemetry[prefix + "Epimp"] = activeA["negative"].toDouble();
        telemetry[prefix + "Epexp"] = activeA["positive"].toDouble();
        telemetry[prefix + "Eqimp"] = reactiveA["negative"].toDouble();
        telemetry[prefix + "Eqexp"] = reactiveA["positive"].toDouble();
    }
    
    QWriteLocker locker(&telemetryLock);
    channelTelemetry[channel] = telemetry;
}

void sftpManager::exportTelemetryToCsv(const QString &filePath)
{
    QFile file(filePath);
    bool writeHeader = !file.exists();

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filePath;
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-16");  // matches your file encoding

    // Fixed key order per channel
    QStringList baseKeysIncoming_Modified = {
        "V(R)","In1(R1)","In1(RPF)",
        "V(Y)","In1(Y1)","In1(YPF)",
        "V(B)","In1(B1)","In1(BPF)",
        "P1","Q1","S1","T(*C)",
        "Epimp1","Epexp1","Eqimp1","Eqexp1"
    };

    QStringList baseKeysIncoming = {
        "V_R","In1_R1","In1_RPF",
        "V_Y","In1_Y1","In1_YPF",
        "V_B","In1_B1","In1_BPF",
        "P1","Q1","S1","T_C",
        "Epimp1","Epexp1","Eqimp1","Eqexp1"
    };

    QStringList baseKeysOutgoing_Modified = {
        "R","PF(R","Y","PF(Y","B","PF(B","P","Q","S","T",
        "Epimp","Epexp","Eqimp","Eqexp"
    };

    QStringList baseKeysOutgoing = {
        "R","PF_R","Y","PF_Y","B","PF_B","P","Q","S","T",
        "Epimp","Epexp","Eqimp","Eqexp"
    };

    // ---- Write Header ----
    if (writeHeader) {
        QStringList header;
        header << "\"Date\"" << "\"Time\"" << "\"IP\"";

        // Channel 1 (incoming)
        for (const QString &k : baseKeysIncoming_Modified) {
            header << "\"" + k + "\"";
        }

        // Channels 2–12 (outgoing)
        for (int ch = 2; ch <= 12; ++ch) {
            int idx = ch - 1;
            for (const QString &k : baseKeysOutgoing_Modified) {
                if (k == "PF(R" || k == "PF(Y" || k == "PF(B")
                    header << QString("\"%1%2)\"").arg(k).arg(idx);
                else
                    header << QString("\"F%1(%2)\"").arg(idx).arg(k);
            }
        }
        out << header.join(",") << "\n";
    }

    // ---- Write Row ----
    QStringList row;
    QDateTime now = QDateTime::currentDateTime();
    row << "\"" + now.date().toString("dd/MM/yyyy") + "\"";
    row << "\"" + now.time().toString("HH:mm") + "\"";
    row << "\"" + Utils::getDeviceIp() + "\"";

    {
        QReadLocker locker(&telemetryLock);

        // Channel 1
        const auto obj1 = channelTelemetry.value(1);
        for (const QString &k : baseKeysIncoming) {
            row << "\"" + QString::number(obj1.value(k).toDouble(), 'f', 4) + "\"";
        }

        // Channels 2–12
        for (int ch = 2; ch <= 12; ++ch) {
            int idx = ch - 1;
            const auto obj = channelTelemetry.value(ch);
            for (const QString &k : baseKeysOutgoing) {
                QString fullKey;
                if (k.startsWith("PF_"))
                    fullKey = k + QString::number(idx);
                else
                    fullKey = QString("F%1_%2").arg(idx).arg(k);

                row << "\"" + QString::number(obj.value(fullKey).toDouble(), 'f', 4) + "\"";
            }
        }
    }

    out << row.join(",") << "\n";
    file.close();
}
