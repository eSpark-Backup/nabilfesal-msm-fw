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
    UsbDataExporter(const QString &mountPath, int intervalDays, QMap<int, QJsonObject> snapShot, QObject *parent = nullptr)
        : QObject(parent), m_mountPath(mountPath), m_intervalDays(intervalDays), m_elapsedDays(0)
    {
        auto uuid = Utils::getUid();
        auto filename = uuid + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
        m_filePath = m_mountPath + "/" + filename;

        {
            QMutexLocker locker(&m_mutex);
            m_latestSnapshot = snapShot;
        }

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &UsbDataExporter::onDayPassed);

        // export immediately at startup
        exportSnapshot();

        // tick every 1 day (safe: 86,400,000 < INT_MAX)
        m_timer->start(24 * 60 * 60 * 1000);

        if (parent) {
            connect(parent, SIGNAL(receivedChannelData(int,QJsonObject)),
                    this, SLOT(onNewChannelData(int,QJsonObject)));
        }
    }

    ~UsbDataExporter() override = default;

public slots:
    void stop() {
        m_timer->stop();
    }

    void onNewChannelData(int channel, const QJsonObject &data) {
        QMutexLocker locker(&m_mutex);
        m_latestSnapshot[channel] = data;
    }

private slots:
    void onDayPassed() {
        m_elapsedDays++;
        if (m_elapsedDays >= m_intervalDays) {
            exportSnapshot();
            m_elapsedDays = 0; // reset
        }
    }

    void exportSnapshot() {
        QMap<int, QJsonObject> snapshotCopy;
        {
            // take a copy while holding lock
            QMutexLocker locker(&m_mutex);
            snapshotCopy = m_latestSnapshot;
        }

        if (snapshotCopy.isEmpty())
            return;

        QFile file(m_filePath);
        bool newFile = !file.exists();
        if (!file.open(QIODevice::Append | QIODevice::Text))
            return;

        QTextStream out(&file);

        // Write header only once
        if (newFile) {
            QStringList headers = {"timestamp", "channel"};
            QJsonObject firstChannel = snapshotCopy[snapshotCopy.firstKey()];
            QStringList flatKeys;
            flattenJson(firstChannel, QString(), flatKeys);
            headers.append(flatKeys);
            out << headers.join(",") << "\n";
        }

        // For each channel → one row
        QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
        for (auto it = snapshotCopy.begin(); it != snapshotCopy.end(); ++it) {
            QString channelKey = QString::number(it.key());
            QJsonObject channelObj = it.value();

            QStringList flatValues;
            flattenJsonValues(channelObj, QString(), flatValues);

            QStringList row = {timestamp, channelKey};
            row.append(flatValues);
            out << row.join(",") << "\n";
        }

        file.close();

        // trim logic
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

        if (lines.size() < 20) return;
        int removeCount = lines.size() / 10;
        lines = lines.mid(removeCount);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &l : lines)
                out << l << "\n";
        }
    }

    void flattenJson(const QJsonObject &obj, const QString &prefix, QStringList &keys) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = prefix.isEmpty() ? it.key() : prefix + "." + it.key();

            if (it.value().isObject()) {
                flattenJson(it.value().toObject(), key, keys);
            } else {
                keys << key;
            }
        }
    }

    void flattenJsonValues(const QJsonObject &obj, const QString &prefix, QStringList &values) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.value().isObject()) {
                flattenJsonValues(it.value().toObject(), prefix.isEmpty() ? it.key() : prefix + "." + it.key(), values);
            } else {
                if (it.value().isDouble()) {
                    values << QString::number(it.value().toDouble(), 'f', 2);
                } else if (it.value().isBool()) {
                    values << (it.value().toBool() ? "true" : "false");
                } else {
                    values << it.value().toString();
                }
            }
        }
    }

    QString m_mountPath;
    QString m_filePath;
    int m_intervalDays;
    QTimer *m_timer;
    int m_elapsedDays;
    QMap<int, QJsonObject> m_latestSnapshot;
    QMutex m_mutex;  // protects m_latestSnapshot
};
