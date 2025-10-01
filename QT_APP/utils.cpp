#include "utils.h"
#include <QProcess>
#include <QJsonArray>
#include <QDateTime>
#include <QStringList>
#include <QThread>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QFile>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include "mqttManager.h"
#include "sftpManager.h"
#include <cstdlib>

namespace Utils {
void keySound() {
    QProcess::startDetached("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});

    QTimer::singleShot(50, [] {
        QProcess::startDetached("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
    });
}

void mainSound() {
    QProcess::startDetached("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});

    QTimer::singleShot(200, [] {
        QProcess::startDetached("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
    });
}


QJsonObject getNtpStatus()
{
    QJsonObject result;

    // servers
    QJsonArray serverArray;
    QFile file("/etc/ntp.conf");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.startsWith("server ")) {
                QString server = line.section(" ", 1, 1).trimmed();
                if (!server.isEmpty())
                    serverArray.append(server);
            }
        }
        file.close();
    }
    result["ntp_servers"] = serverArray;

    // date & time
    QDateTime now = QDateTime::currentDateTime();
    result["date"] = now.toString("yyyy-MM-dd");
    result["time"] = now.toString("hh:mm:ss");

    // service check (non-blocking better done in background thread)
    QProcess process;
    process.start("/etc/init.d/S48sntp", {"status"});
    if (process.waitForFinished(500)) { // timeout 500ms
        QString output = process.readAllStandardOutput();
        result["ntp_status"] = output.contains("SUCCESS", Qt::CaseInsensitive)
                                   ? "running"
                                   : "stopped";
    } else {
        result["ntp_status"] = "unknown";
    }

    return result;
}

bool setNtpServers(const QStringList &servers)
{
    if (servers.isEmpty())
        return false;

    // Read the existing config
    QFile file("/etc/ntp.conf");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QStringList lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        // Skip existing "server ..." lines
        if (!line.trimmed().startsWith("server "))
            lines << line;
    }
    file.close();

    // Insert new servers at the top
    QStringList newServerLines;
    for (const QString &server : servers) {
        newServerLines << QString("server %1 iburst").arg(server);
    }
    lines.prepend(newServerLines.join("\n"));

    // Write back to /etc/ntp.conf
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream out(&file);
    for (const QString &line : lines)
        out << line << "\n";
    file.close();

    // Restart NTP service
    QProcess process;
    process.start("/etc/init.d/S48sntp", {"restart"});
    process.waitForFinished();

    return process.exitStatus() == QProcess::NormalExit;
}


QString getDeviceImei()
{
    // Try modems 0,1,2 in sequence
    for (int i = 0; i < 3; ++i) {
        QProcess process;
        process.start("mmcli", {"-m", QString::number(i)});
        if (!process.waitForFinished(3000))  // 3s max
            continue;

        QString output = process.readAllStandardOutput();
        for (const QString &line : output.split("\n")) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("imei", Qt::CaseInsensitive) ||
                trimmedLine.contains("imei", Qt::CaseInsensitive)) {
                return trimmedLine.section(":", 1).trimmed();
            }
        }
    }
    return "UNKNOWN";
}

QString getSignalCategoryFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "UNKNOWN";
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // Parse response: +CSQ: <rssi>,<ber>
    QRegularExpression re("\\+CSQ:\\s*(\\d+),(\\d+)");
    QRegularExpressionMatch match = re.match(content);
    if (!match.hasMatch())
        return "UNKNOWN";

    int rssi = match.captured(1).toInt();
    if (rssi == 99)
        return "UNKNOWN";

    int dBm = -113 + (2 * rssi);

    if (dBm >= -70) return QString("Strong (%1)").arg(dBm);
    else if (dBm >= -85) return QString("Normal (%1)").arg(dBm);
    else if (dBm >= -100) return QString("Weak (%1)").arg(dBm);
    else return QString("Low (%1)").arg(dBm);
}

QString getDeviceIp()
{
    foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                    return ip.toString();
                }
            }
        }
    }
    return "0.0.0.0"; // fallback if not found
}

QString getUid()
{
    QFile file("config/project.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            auto uid = obj.value("uid").toString();
            return uid;
        }
        file.close();
    }
    return QString();
}


QString getMqttStatus()
{
    mqttManager &mqtt = mqttManager::instance();
    if (mqtt.mqttState())
        return "Connected";
    else
        return "Disconnected";
}


QString getSftpStatus()
{
    sftpManager &sftp = sftpManager::instance();
    if (sftp.sftpState())
        return "Connected";
    else
        return "Disconnected";
}


void rebootSystem()
{
    // Make sure any pending writes are flushed
    QProcess::startDetached("sh", {"-c", "sync; reboot"});
}


bool updateSerialInConfig()
{
    QString path = "/MSM/potential-pancake/MSM/bin/fleet_provisioner/config.json";

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open config file for reading:" << path;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid JSON in config file:" << parseError.errorString();
        return false;
    }

    QJsonObject obj = doc.object();

    // ✅ overwrite Serial
    QString uid = Utils::getUid();
    if (uid.isEmpty()) {
        qWarning() << "UID is empty, not updating Serial!";
        return false;
    }
    obj["serial"] = uid;

    // Write back to file
    doc.setObject(obj);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Failed to open config file for writing:" << path;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Serial updated to:" << uid;
    return true;
}

}
