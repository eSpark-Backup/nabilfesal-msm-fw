#pragma once

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QStorageInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include "utils.h"

class UsbDataExporter : public QObject {
    Q_OBJECT
public:
    UsbDataExporter(const QString &mountPath, int intervalDays, const QMap<int, QJsonObject> &snapShot, QObject *parent = nullptr)
        : QObject(parent),
          m_mountPath(mountPath),
          m_intervalDays(intervalDays),
          m_elapsedDays(0)
    {
        auto uuid = Utils::getUid();
        auto filename = uuid + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
        m_filePath = m_mountPath + "/" + filename;

        {
            QMutexLocker locker(&m_mutex);
            m_latestSnapshot = snapShot;
        }

        // Timers for periodic actions
        m_logTimer = new QTimer(this);
        m_dayTimer = new QTimer(this);

        exportSnapshot();

        connect(m_logTimer, &QTimer::timeout, this, &UsbDataExporter::exportSnapshot);
        connect(m_dayTimer, &QTimer::timeout, this, &UsbDataExporter::onDayPassed);


        // Log every 10 minutes
        m_logTimer->start(10 * 60 * 1000);

        // Track day progress every 24 hours
        m_dayTimer->start(24 * 60 * 60 * 1000);

        // Connect to parent data signal if available
        if (parent) {
            connect(parent, SIGNAL(receivedChannelData(int, QJsonObject)),
                    this, SLOT(onNewChannelData(int, QJsonObject)));
        }
    }

    ~UsbDataExporter() override = default;

public slots:
    void stop() {
        if (m_logTimer) m_logTimer->stop();
        if (m_dayTimer) m_dayTimer->stop();
    }

    void onNewChannelData(int channel, const QJsonObject &payload) {
        // Defensive parsing
        if (payload.isEmpty()) return;
        
        QJsonObject root = payload;
        QJsonObject data = root["data"].toObject();

        if (data.isEmpty()) return;

        QJsonObject phaseA = data["phase_a"].toObject();
        QJsonObject phaseB = data["phase_b"].toObject();
        QJsonObject phaseC = data["phase_c"].toObject();
        QJsonObject telemetry;

        // =================== INCOMING CHANNEL ===================
        if (channel == 1) {
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
            double P1_Y = phaseB["power"].toObject()["active"].toDouble();
            double P1_B = phaseC["power"].toObject()["active"].toDouble();
            telemetry["P1"] = P1_R + P1_Y + P1_B;

            // Reactive
            double Q1_R = phaseA["power"].toObject()["reactive"].toDouble();
            double Q1_Y = phaseB["power"].toObject()["reactive"].toDouble();
            double Q1_B = phaseC["power"].toObject()["reactive"].toDouble();
            telemetry["Q1"] = Q1_R + Q1_Y + Q1_B;

            // Apparent
            double S1_R = phaseA["power"].toObject()["apparent"].toDouble();
            double S1_Y = phaseB["power"].toObject()["apparent"].toDouble();
            double S1_B = phaseC["power"].toObject()["apparent"].toDouble();
            telemetry["S1"] = S1_R + S1_Y + S1_B;

            // Energy
            QJsonObject energyA   = phaseA["energy"].toObject();
            QJsonObject activeA   = energyA["active"].toObject();
            QJsonObject reactiveA = energyA["reactive"].toObject();

            telemetry["T_C"]    = root["temperature"].toDouble();
            telemetry["Epimp1"] = activeA["negative"].toDouble();
            telemetry["Epexp1"] = activeA["positive"].toDouble();
            telemetry["Eqimp1"] = reactiveA["negative"].toDouble();
            telemetry["Eqexp1"] = reactiveA["positive"].toDouble();
        } 
        // =================== OUTGOING CHANNELS ===================
        else {
            int idx = channel - 1;
            QString prefix = QString("F%1_").arg(idx);

            double P_R = phaseA["power"].toObject()["active"].toDouble();
            double P_Y = phaseB["power"].toObject()["active"].toDouble();
            double P_B = phaseC["power"].toObject()["active"].toDouble();

            double Q_R = phaseA["power"].toObject()["reactive"].toDouble();
            double Q_Y = phaseB["power"].toObject()["reactive"].toDouble();
            double Q_B = phaseC["power"].toObject()["reactive"].toDouble();

            double S_R = phaseA["power"].toObject()["apparent"].toDouble();
            double S_Y = phaseB["power"].toObject()["apparent"].toDouble();
            double S_B = phaseC["power"].toObject()["apparent"].toDouble();

            QJsonObject energyA   = phaseA["energy"].toObject();
            QJsonObject activeA   = energyA["active"].toObject();
            QJsonObject reactiveA = energyA["reactive"].toObject();

            telemetry[prefix + "R"]     = phaseA["general"].toObject()["current"].toDouble();
            telemetry["PF_R" + QString::number(idx)] = phaseA["power"].toObject()["factor"].toDouble();

            telemetry[prefix + "Y"]     = phaseB["general"].toObject()["current"].toDouble();
            telemetry["PF_Y" + QString::number(idx)] = phaseB["power"].toObject()["factor"].toDouble();

            telemetry[prefix + "B"]     = phaseC["general"].toObject()["current"].toDouble();
            telemetry["PF_B" + QString::number(idx)] = phaseC["power"].toObject()["factor"].toDouble();

            telemetry[prefix + "P"] = P_R + P_Y + P_B;
            telemetry[prefix + "Q"] = Q_R + Q_Y + Q_B;
            telemetry[prefix + "S"] = S_R + S_Y + S_B;

            telemetry[prefix + "T"]     = root["temperature"].toDouble();
            telemetry[prefix + "Epimp"] = activeA["negative"].toDouble();
            telemetry[prefix + "Epexp"] = activeA["positive"].toDouble();
            telemetry[prefix + "Eqimp"] = reactiveA["negative"].toDouble();
            telemetry[prefix + "Eqexp"] = reactiveA["positive"].toDouble();
        }

        QMutexLocker locker(&m_mutex);
        m_latestSnapshot[channel] = telemetry;
    }

private slots:
    void onDayPassed() {
        m_elapsedDays++;
        if (m_elapsedDays >= m_intervalDays) {
            stop();
        }
    }

    void exportSnapshot() {
        QMap<int, QJsonObject> snapshotCopy;
        {
            QMutexLocker locker(&m_mutex);
            snapshotCopy = m_latestSnapshot;
        }

        if (snapshotCopy.isEmpty()) {
            return;
        }

        QFile file(m_filePath);
        bool writeHeader = !file.exists();

        if (!file.open(QIODevice::Append | QIODevice::Text)) {
            return;
        }

        QTextStream out(&file);
        out.setCodec("UTF-16");

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
            "P1","Q1","S1", "T_C",
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
            header << "\"Date\"" << "\"Time\"" << "\"IP\"" << "\"UUID\"";

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

        // Row
        QStringList row;
        QDateTime now = QDateTime::currentDateTime();
        row << "\"" + now.date().toString("dd/MM/yyyy") + "\"";
        row << "\"" + now.time().toString("HH:mm:ss") + "\"";
        row << "\"" + Utils::getDeviceIp() + "\"";
        row << "\"" + Utils::getUid() + "\"";

        const auto obj1 = snapshotCopy.value(1);
        for (const QString &k : baseKeysIncoming)
            row << "\"" + QString::number(obj1.value(k).toDouble(), 'f', 4) + "\"";

        for (int ch = 2; ch <= 12; ++ch) {
            int idx = ch - 1;
            const auto obj = snapshotCopy.value(ch);
            for (const QString &k : baseKeysOutgoing) {
                QString fullKey = k.startsWith("PF_") ? k + QString::number(idx)
                                                      : QString("F%1_%2").arg(idx).arg(k);
                row << "\"" + QString::number(obj.value(fullKey).toDouble(), 'f', 4) + "\"";
            }
        }

        out << row.join(",") << "\n";
        file.close();

        // Trim if low on storage
        QStorageInfo storage(m_mountPath);
        if (storage.bytesAvailable() < 5 * 1024 * 1024) {
            trimCsvFile();
        }
    }

private:
    void trimCsvFile() {
        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QStringList lines;
        QTextStream in(&file);
        while (!in.atEnd())
            lines << in.readLine();
        file.close();

        // Keep the header
        if (lines.size() <= 2) return;

        int removeCount = lines.size() / 10;
        lines = lines.mid(removeCount);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &l : lines)
                out << l << "\n";
        }
    }

private:
    QString m_mountPath;
    QString m_filePath;
    int m_intervalDays;
    int m_elapsedDays;
    QTimer *m_logTimer;
    QTimer *m_dayTimer;
    QMap<int, QJsonObject> m_latestSnapshot;
    QMutex m_mutex;
};
