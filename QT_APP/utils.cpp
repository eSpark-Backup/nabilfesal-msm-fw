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

    // 1. Parse /etc/ntp.conf for servers
    QJsonArray serverArray;
    QFile file("/etc/ntp.conf");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("server ")) {
                QString server = line.section(" ", 1, 1).trimmed();
                if (!server.isEmpty())
                    serverArray.append(server);
            }
        }
        file.close();
    }
    result["ntp_servers"] = serverArray;

    // 2. Current system date and time
    QDateTime now = QDateTime::currentDateTime();
    result["date"] = now.date().toString("yyyy-MM-dd");
    result["time"] = now.time().toString("hh:mm:ss");

    // 3. Basic service status (optional)
    QProcess process;
    process.start("/etc/init.d/S48sntp", {"status"});
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    result["ntp_status"] = output.contains("SUCCESS", Qt::CaseInsensitive)
                               ? "running"
                               : "stopped";

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
    // Example: modem manager (mmcli)
    process.start("mmcli", {"-m", "0"});
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    // Try to extract IMEI
    QString imei;
    for (const QString &line : output.split("\n")) {
        if (line.contains("imei", Qt::CaseInsensitive)) {
            imei = line.section(":", 1).trimmed();
            break;
        }
    }

    if (imei.isEmpty())
        imei = "UNKNOWN";

    return imei;
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

}
