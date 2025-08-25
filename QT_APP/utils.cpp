#include "utils.h"
#include <QProcess>
#include <QJsonArray>
#include <QDateTime>
#include <QStringList>
#include <QThread>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QFile>

namespace Utils {
void keySound() {
    QProcess::execute("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});
    QThread::usleep(50000); // 50ms
    QProcess::execute("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
}

QJsonObject getNtpStatus()
{
    QJsonObject result;

    // 1. Get NTP status using timedatectl
    QProcess process;
    process.start("timedatectl", {"timesync-status"});
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    bool synced = output.contains("synchronized: yes", Qt::CaseInsensitive) ||
                  output.contains("System clock synchronized: yes", Qt::CaseInsensitive);

    result["ntp_status"] = synced ? "synchronized" : "not synchronized";

    // 2. Extract servers (if any)
    QStringList servers;
    for (const QString& line : output.split("\n")) {
        if (line.contains("Server:", Qt::CaseInsensitive) ||
            line.contains("Server address:", Qt::CaseInsensitive)) {
            QString server = line.section(":", 1).trimmed();
            if (!server.isEmpty())
                servers << server;
        }
    }

    // Limit to 3 servers
    QJsonArray serverArray;
    for (int i = 0; i < qMin(3, servers.size()); i++) {
        serverArray.append(servers[i]);
    }
    result["ntp_servers"] = serverArray;

    // 3. Current system date and time (separate keys)
    QDateTime now = QDateTime::currentDateTime();
    result["date"] = now.date().toString("yyyy-MM-dd");
    result["time"] = now.time().toString("hh:mm:ss");

    return result;
}

bool setNtpServers(const QStringList &servers)
{
    if (servers.size() < 1)
        return false;

    // 1. Prepare config text
    QString config = "[Time]\n";
    config += "NTP=" + servers.join(" ") + "\n";
    config += "FallbackNTP=ntp.ubuntu.com\n";

    // 2. Write to /etc/systemd/timesyncd.conf
    QFile file("/etc/systemd/timesyncd.conf");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << config;
    file.close();

    // 3. Restart systemd-timesyncd service
    QProcess process;
    process.start("systemctl", {"restart", "systemd-timesyncd"});
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
