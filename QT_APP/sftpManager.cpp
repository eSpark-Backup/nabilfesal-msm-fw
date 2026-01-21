#include "sftpManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "utils.h"
#include <QDir>
#include <QThread>
#include "encryptionhelper.h"


#define SFTP_PATH sftp_path

sftpManager::sftpManager(Mode mode, QObject *parent) : clientMode(mode), QObject(parent)
{
    if (clientMode == Mode::Factory) {
        tag = COLOR_TAG(F_CYAN, B_WHITE, "Factory [SFTP]");
        sftp_path = "config/sftp_settings.json";
    } else {
        tag = COLOR_TAG(F_CYAN, B_WHITE, "Admin [SFTP]");
    }

    publishTimer = new QTimer(this);
    reconnectTimer = new QTimer(this);
    isConnected  = false;
    process = new QProcess(this);

    reconnectTimer->setInterval(5 * 60 * 1000);   // 5 minutes
    reconnectTimer->setSingleShot(false);         // keep retrying

    connect(reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected) reloadSettings();
    });

    if (!reconnectTimer->isActive()) reconnectTimer->start();

    // Process connected
    connect(process, &QProcess::started, this, [this]() {
        isConnected = true;
        logDebug << "Connected to SFTP!";
        reconnectTimer->stop();
        scheduleSftpUpload();
        Utils::startAlignedTimer( this,
            publishTimer, intervalSec,
            [this]() {
                if (!m_filePath.isEmpty()) {
                    exportTelemetryToCsv(m_filePath);
                }
            }
        );
    });

    // Process disconnected
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
        isConnected = false;
        publishTimer->stop();
        logDebug << "Disconnected from SFTP.";
        if (!reconnectTimer->isActive()) reconnectTimer->start();
    });

    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        isConnected = false;
        publishTimer->stop();
        logWarn << "SFTP process error:" << error;
        if (!reconnectTimer->isActive()) reconnectTimer->start();
    });

    // Publish CSV timer
    connect(publishTimer, &QTimer::timeout, this, [this]() {
        if (m_filePath.isEmpty()) {
            QString uid = Utils::getUid();
            uid += ((clientMode == Mode::Factory) ? "_factory" : "");
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
        logWarn << "Skipped scheduling, SFTP not connected.";
        return;
    }

    QString hhmm = this->schedule;
    if (hhmm.length() != 4) {
        logWarn << "Invalid schedule format:" << hhmm;
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
    // logDebug << "Next SFTP upload scheduled at:" << scheduledTime
    //          << "(in" << msecToGo / 1000 << "seconds)";

    QTimer::singleShot(msecToGo, this, [this]() {
        QString uid = Utils::getUid();
        uid += ((clientMode == Mode::Factory) ? "_factory" : "");
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
        logWarn << "Config file not found:" << filePath;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logWarn << "Failed to open SFTP config file";
        return false;
    }

    EncryptionHelper::check_and_encrypt_files(filePath, "SFTP", false, nullptr);

    QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(filePath, Utils::getEncryptionKey());
    if (decryptedData.isEmpty()) {
        logWarn << "Failed to decrypt SFTP config file (wrong password or corrupted file)";
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
    if (!doc.isObject()) {
        logWarn << "Invalid SFTP config file format.";
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

    if (schedule.isEmpty()) schedule = "0100";

    // // Log the values
    // logDebug << "SFTP Settings Loaded:";
    // logDebug << "  Host:" << host;
    // logDebug << "  UserName:" << username;
    // logDebug << "  Pass:" << password;
    // logDebug << "  Port:" << port;
    // logDebug << "  Interval (sec):" << intervalSec;

    return true;
}


void sftpManager::reloadSettings()
{
    configPath = SFTP_PATH;
    if (!loadConfig(configPath)) {
        logWarn << "No valid config found. Skipping SFTP connection.";
        return;
    }
    connectToSftp();
}

void sftpManager::connectToSftp()
{
    if (host.isEmpty()) {
        logWarn << "SFTP settings incomplete.";
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
        logWarn << "Failed to start SFTP process!";
        logWarn << "Program:" << program << "Args:" << arguments;
        return;
    }

    // logDebug << "SFTP process started for" << host;
}


void sftpManager::disconnectClient()
{
    disconnectFromSftp();
}


void sftpManager::disconnectFromSftp()
{
    if (process->state() == QProcess::Running) {
        // logDebug << "Disconnecting SFTP...";
        process->write("bye\n");
        process->waitForFinished(2000);
        process->close();
    }
}


void sftpManager::uploadFile(const QString &localFile, const QString &remotePath)
{
    publishTimer->stop();

    if (host.isEmpty() || username.isEmpty() || password.isEmpty()) {
        logWarn << "SFTP config missing.";
        return;
    }

    QFileInfo fi(localFile);
    if (!fi.exists() || !fi.isFile()) {
        logWarn << "Local file missing:" << localFile;
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
        logWarn << "Failed to start SFTP process!";
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

    if(channel >= 1 && channel <= 12)
    {
        QMutexLocker locker(&sftpMutex);
        // Accumulate energy for the current channel
        sftpEnergy[channel].negActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["negative"].toDouble();
        sftpEnergy[channel].negActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["negative"].toDouble();
        sftpEnergy[channel].negActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["negative"].toDouble();

        sftpEnergy[channel].posActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["positive"].toDouble();
        sftpEnergy[channel].posActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["positive"].toDouble();
        sftpEnergy[channel].posActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["positive"].toDouble();

        sftpEnergy[channel].negReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
        sftpEnergy[channel].negReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
        sftpEnergy[channel].negReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["negative"].toDouble();

        sftpEnergy[channel].posReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
        sftpEnergy[channel].posReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
        sftpEnergy[channel].posReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
    }

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

        telemetry["Epexp1"] = sftpEnergy[1].negActEnergy[0] + sftpEnergy[1].negActEnergy[1] + sftpEnergy[1].negActEnergy[2];
        telemetry["Epimp1"] = sftpEnergy[1].posActEnergy[0] + sftpEnergy[1].posActEnergy[1] + sftpEnergy[1].posActEnergy[2];
        telemetry["Eqexp1"] = sftpEnergy[1].negReacEnergy[0] + sftpEnergy[1].negReacEnergy[1] + sftpEnergy[1].negReacEnergy[2];
        telemetry["Eqimp1"] = sftpEnergy[1].posReacEnergy[0] + sftpEnergy[1].posReacEnergy[1] + sftpEnergy[1].posReacEnergy[2];
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

        telemetry[prefix + "Epexp"] = sftpEnergy[channel].negActEnergy[0] + sftpEnergy[channel].negActEnergy[1] + sftpEnergy[channel].negActEnergy[2];
        telemetry[prefix + "Epimp"] = sftpEnergy[channel].posActEnergy[0] + sftpEnergy[channel].posActEnergy[1] + sftpEnergy[channel].posActEnergy[2];
        telemetry[prefix + "Eqexp"] = sftpEnergy[channel].negReacEnergy[0] + sftpEnergy[channel].negReacEnergy[1] + sftpEnergy[channel].negReacEnergy[2];
        telemetry[prefix + "Eqimp"] = sftpEnergy[channel].posReacEnergy[0] + sftpEnergy[channel].posReacEnergy[1] + sftpEnergy[channel].posReacEnergy[2];
    }
    
    QWriteLocker locker(&telemetryLock);
    channelTelemetry[channel] = telemetry;
}

void sftpManager::exportTelemetryToCsv(const QString &filePath)
{
    QFile file(filePath);
    bool writeHeader = !file.exists();

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        logWarn << "Could not open file for writing:" << filePath;
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-16");

    QStringList baseKeysIncoming_Modified = {
        "V(R)","In1(R1)","In1(RPF)",
        "V(Y)","In1(Y1)","In1(YPF)",
        "V(B)","In1(B1)","In1(BPF)",
        "P1","Q1","S1",
        "Epimp1","Epexp1","Eqimp1","Eqexp1"
    };

    QStringList baseKeysIncoming = {
        "V_R","In1_R1","In1_RPF",
        "V_Y","In1_Y1","In1_YPF",
        "V_B","In1_B1","In1_BPF",
        "P1","Q1","S1",
        "Epimp1","Epexp1","Eqimp1","Eqexp1"
    };

    QStringList baseKeysOutgoing_Modified = {
        "R","PF(R","Y","PF(Y","B","PF(B","P","Q","S",
        "Epimp","Epexp","Eqimp","Eqexp"
    };

    QStringList baseKeysOutgoing = {
        "R","PF_R","Y","PF_Y","B","PF_B","P","Q","S",
        "Epimp","Epexp","Eqimp","Eqexp"
    };

    // ---- Write Header ----
    if (writeHeader) {
        QStringList header;
        header << "\"Date\"" << "\"Time\"" << "\"IP\"";

        for (const QString &k : baseKeysIncoming_Modified) header << "\"" + k + "\"";
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
    row << "\"" + now.time().toString("HH:mm:ss") + "\"";
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
                QString fullKey = (k.startsWith("PF_")) ? k + QString::number(idx)
                                                         : QString("F%1_%2").arg(idx).arg(k);
                row << "\"" + QString::number(obj.value(fullKey).toDouble(), 'f', 4) + "\"";
            }
        }
    }

    out << row.join(",") << "\n";
    file.close();
}


void sftpManager::resetAccumulatedEnergy()
{
    for(int ch = 1; ch <= 12; ++ch) {
        for(int ph = 0; ph < 3; ph++)
        {
            sftpEnergy[ch].posActEnergy[ph] = 0.0;
            sftpEnergy[ch].negActEnergy[ph] = 0.0;
            sftpEnergy[ch].posReacEnergy[ph] = 0.0;
            sftpEnergy[ch].negReacEnergy[ph] = 0.0;
        }
    }
}


void sftpManager::updateAccumulatedEnergy(const std::array<EnergyData,13> &data)
{
    QMutexLocker locker(&sftpMutex);
    sftpEnergy = data;
}
