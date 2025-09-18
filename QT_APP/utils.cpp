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
#include "awsmqttmanager.h"

namespace Utils {
void keySound() {
    QProcess::startDetached("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});

    QTimer::singleShot(50, [] {
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
    QProcess process;
    process.start("mmcli", {"-m", "0"});
    if (!process.waitForFinished(1000))  // 1s max
        return "UNKNOWN";

    QString output = process.readAllStandardOutput();
    for (const QString &line : output.split("\n")) {
        if (line.contains("imei", Qt::CaseInsensitive)) {
            return line.section(":", 1).trimmed();
        }
    }
    return "UNKNOWN";
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
    AwsMqttManager mqtt;
    if (mqtt.mqttState())
        return "Connected";
    else
        return "Disconnected";
}

}
