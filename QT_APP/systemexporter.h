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
#include <QDir>
#include "common_headers.h"

class SystemDataExporter : public QObject {
    Q_OBJECT
public:
    SystemDataExporter( QObject *parent = nullptr)
        : QObject(parent)
    {
        QString logDir = "data/logs";
        QDir().mkpath(logDir);
        auto uuid = Utils::getUid();
        auto filename = "uart_logs.csv";
        m_filePath = logDir + "/" + filename;

        // Timers for periodic actions
        m_logTimer = new QTimer(this);

        connect(m_logTimer, &QTimer::timeout, this, &SystemDataExporter::exportSnapshot);


        // Log every 10 minutes
        m_logTimer->start(10 * 60 * 1000);

        // Connect to parent data signal if available
        if (parent) {
            connect(parent, SIGNAL(receivedChannelData(int, QJsonObject)),
                    this, SLOT(onNewChannelData(int, QJsonObject)));
        }
    }

    QString getFilePath()
    {
        return m_filePath;
    }

    ~SystemDataExporter() override = default;

public slots:
    void stop() {
        if (m_logTimer) m_logTimer->stop();
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

        // Accumulate energy for the current channel
        sdEnergy[channel].negActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["negative"].toDouble();
        sdEnergy[channel].negActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["negative"].toDouble();
        sdEnergy[channel].negActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["negative"].toDouble();

        sdEnergy[channel].posActEnergy[0] += phaseA["energy"].toObject()["active"].toObject()["positive"].toDouble();
        sdEnergy[channel].posActEnergy[1] += phaseB["energy"].toObject()["active"].toObject()["positive"].toDouble();
        sdEnergy[channel].posActEnergy[2] += phaseC["energy"].toObject()["active"].toObject()["positive"].toDouble();

        sdEnergy[channel].negReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
        sdEnergy[channel].negReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["negative"].toDouble();
        sdEnergy[channel].negReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["negative"].toDouble();

        sdEnergy[channel].posReacEnergy[0] += phaseA["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
        sdEnergy[channel].posReacEnergy[1] += phaseB["energy"].toObject()["reactive"].toObject()["positive"].toDouble();
        sdEnergy[channel].posReacEnergy[2] += phaseC["energy"].toObject()["reactive"].toObject()["positive"].toDouble();

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
            telemetry["Epexp1"] = sdEnergy[1].negActEnergy[0] + sdEnergy[1].negActEnergy[1] + sdEnergy[1].negActEnergy[2];
            telemetry["Epimp1"] = sdEnergy[1].posActEnergy[0] + sdEnergy[1].posActEnergy[1] + sdEnergy[1].posActEnergy[2];
            telemetry["Eqexp1"] = sdEnergy[1].negReacEnergy[0] + sdEnergy[1].negReacEnergy[1] + sdEnergy[1].negReacEnergy[2];
            telemetry["Eqimp1"] = sdEnergy[1].posReacEnergy[0] + sdEnergy[1].posReacEnergy[1] + sdEnergy[1].posReacEnergy[2];
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
            telemetry[prefix + "Epexp"] = sdEnergy[channel].negActEnergy[0] + sdEnergy[channel].negActEnergy[1] + sdEnergy[channel].negActEnergy[2];
            telemetry[prefix + "Epimp"] = sdEnergy[channel].posActEnergy[0] + sdEnergy[channel].posActEnergy[1] + sdEnergy[channel].posActEnergy[2];
            telemetry[prefix + "Eqexp"] = sdEnergy[channel].negReacEnergy[0] + sdEnergy[channel].negReacEnergy[1] + sdEnergy[channel].negReacEnergy[2];
            telemetry[prefix + "Eqimp"] = sdEnergy[channel].posReacEnergy[0] + sdEnergy[channel].posReacEnergy[1] + sdEnergy[channel].posReacEnergy[2];
        }

        QMap<int, QJsonObject> snapshotCopy;
        {
            QMutexLocker locker(&m_mutex);
            m_latestSnapshot[channel] = telemetry;
            snapshotCopy = m_latestSnapshot;
        }

        QFile file(m_filePath);
        if (snapshotCopy.size() == 12 && !file.exists()) {
            exportSnapshot();
        }
    }

    void resetAccumulatedEnergy()
    {
        for(int ch = 1; ch <= 12; ++ch) {
            for(int ph = 0; ph < 3; ph++)
            {
                sdEnergy[ch].posActEnergy[ph] = 0.0;
                sdEnergy[ch].negActEnergy[ph] = 0.0;
                sdEnergy[ch].posReacEnergy[ph] = 0.0;
                sdEnergy[ch].negReacEnergy[ph] = 0.0;
            }
        }
    }

    void updateAccumulatedEnergy(const std::array<EnergyData,13> &data)
    {
        sdEnergy = data;
    }

private slots:
    void exportSnapshot()
    {
        QMap<int, QJsonObject> snapshotCopy;
        {
            QMutexLocker locker(&m_mutex);
            snapshotCopy = m_latestSnapshot;
        }

        if (snapshotCopy.isEmpty())
            return;

        QFile file(m_filePath);
        bool writeHeader = !file.exists();

        if (!file.open(QIODevice::Append | QIODevice::Text))
            return;

        QTextStream out(&file);
        out.setCodec("UTF-8");

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

        // ---------------- WRITE HEADER ----------------
        if (writeHeader) {
            QStringList header;
            header << "\"Date\"" << "\"Time\"" << "\"IP\"";

            // Channel 1
            for (const QString &k : baseKeysIncoming_Modified)
                header << "\"" + k + "\"";

            // Channel 2–12
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

        // ---------------- BUILD ROW ----------------
        QStringList row;

        QDateTime now = QDateTime::currentDateTime();
        row << "\"" + now.date().toString("dd/MM/yyyy") + "\"";
        row << "\"" + now.time().toString("HH:mm:ss") + "\"";
        row << "\"" + Utils::getDeviceIp() + "\"";

        // --------------- CHANNEL 1 -------------------
        const QJsonObject obj1 = snapshotCopy.value(1);

        for (const QString &k : baseKeysIncoming) {
                 if (k == "Epexp1") row << "\"" + QString::number(sdEnergy[1].negActEnergy[0] + sdEnergy[1].negActEnergy[1] + sdEnergy[1].negActEnergy[2], 'f', 2) + "\"";
            else if (k == "Epimp1") row << "\"" + QString::number(sdEnergy[1].posActEnergy[0] + sdEnergy[1].posActEnergy[1] + sdEnergy[1].posActEnergy[2], 'f', 2) + "\"";
            else if (k == "Eqexp1") row << "\"" + QString::number(sdEnergy[1].negReacEnergy[0] + sdEnergy[1].negReacEnergy[1] + sdEnergy[1].negReacEnergy[2], 'f', 2) + "\"";
            else if (k == "Eqexp1") row << "\"" + QString::number(sdEnergy[1].posReacEnergy[0] + sdEnergy[1].posReacEnergy[1] + sdEnergy[1].posReacEnergy[2], 'f', 2) + "\"";
            else
                row << "\"" + QString::number(obj1.value(k).toDouble(), 'f', 2) + "\"";
        }

        // ---------------- CHANNELS 2–12 ----------------
        for (int ch = 2; ch <= 12; ++ch) {
            int idx = ch - 1;
            const QJsonObject obj = snapshotCopy.value(ch);

            for (const QString &k : baseKeysOutgoing) {
                QString fullKey;

                if (k == "Epexp") {
                    double total =
                        sdEnergy[ch].negActEnergy[0] +
                        sdEnergy[ch].negActEnergy[1] +
                        sdEnergy[ch].negActEnergy[2];

                    row << "\"" + QString::number(total, 'f', 2) + "\"";
                }
                else if (k == "Epimp") {
                    double total =
                        sdEnergy[ch].posActEnergy[0] +
                        sdEnergy[ch].posActEnergy[1] +
                        sdEnergy[ch].posActEnergy[2];

                    row << "\"" + QString::number(total, 'f', 2) + "\"";
                }
                else if (k == "Eqexp") {
                    double total =
                        sdEnergy[ch].negReacEnergy[0] +
                        sdEnergy[ch].negReacEnergy[1] +
                        sdEnergy[ch].negReacEnergy[2];

                    row << "\"" + QString::number(total, 'f', 2) + "\"";
                }
                else if (k == "Eqimp") {
                    double total =
                        sdEnergy[ch].posReacEnergy[0] +
                        sdEnergy[ch].posReacEnergy[1] +
                        sdEnergy[ch].posReacEnergy[2];

                    row << "\"" + QString::number(total, 'f', 2) + "\"";
                }
                else {
                    fullKey = k.startsWith("PF_")
                                ? (k + QString::number(idx))
                                : QString("F%1_%2").arg(idx).arg(k);
                    row << "\"" + QString::number(obj.value(fullKey).toDouble(), 'f', 2) + "\"";
                }
            }
        }

        out << row.join(",") << "\n";
        file.close();

        trimCsvFile();
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

        if (lines.size() <= 2) return;

        QString header = lines.first();
        QStringList filtered;
        filtered << header;

        QDate cutoff = QDate::currentDate().addDays(-730);
        for (int i = 1; i < lines.size(); ++i) {
            QStringList parts = lines[i].split(",");
            if (parts.size() < 1) continue;
            QDate date = QDate::fromString(parts[0].remove("\""), "dd/MM/yyyy");
            if (date >= cutoff)
                filtered << lines[i];
        }

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &l : filtered)
                out << l << "\n";
        }
    }

private:
    QString m_filePath;
    QTimer *m_logTimer;
    QMap<int, QJsonObject> m_latestSnapshot;
    QMutex m_mutex;

    // Accumultion
    std::array<EnergyData, 13> sdEnergy;
};
