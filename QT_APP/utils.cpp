#include "utils.h"
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QLabel>
#include <cstdlib>
#include <QPixmap>
#include <QThread>
#include <unistd.h>
#include <QProcess>
#include <QDateTime>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QHostAddress>
#include "mainwindow.h"
#include "mqttManager.h"
#include "sftpManager.h"
#include <QJsonDocument>
#include "awsmqttmanager.h"
#include "common_headers.h"
#include <QNetworkInterface>
#include "encryptionhelper.h"


namespace Utils {
void keySound() {
    #if defined(BUILD_V3S)
    QProcess::startDetached("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});

    QTimer::singleShot(50, [] {
        QProcess::startDetached("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
    });
    #endif
}


void mainSound() {
    #if defined(BUILD_V3S)
    QProcess::startDetached("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});

    QTimer::singleShot(200, [] {
        QProcess::startDetached("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
    });
    #endif
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
                                   ? "Running"
                                   : "Stopped";
    } else {
        result["ntp_status"] = "UNKNOWN";
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


QString extractImeiFromResponses(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "UNKNOWN";

    QString content = file.readAll();
    file.close();

    // Find the "AT+GSN" command block
    QStringList lines = content.split("\n");
    bool inGsnBlock = false;

    for (QString line : lines) {
        line = line.trimmed();

        if (line.contains("command:", Qt::CaseInsensitive) &&
            line.contains("AT+GSN", Qt::CaseInsensitive)) 
        {
            inGsnBlock = true;
            continue;
        }

        if (inGsnBlock) {
            // Skip "response:" or "AT+GSN" echo
            if (line.isEmpty() ||
                line.startsWith("response", Qt::CaseInsensitive) ||
                line.compare("AT+GSN", Qt::CaseInsensitive) == 0)
                continue;

            // IMEI must be a digit-only string, typically 14–17 digits
            if (line.contains(QRegularExpression("^\\d{14,17}$"))) {
                return line;
            }

            // If we hit something else, exit the block
            if (line.startsWith("command:", Qt::CaseInsensitive))
                break;
        }
    }
    return "UNKNOWN";
}


QString getDeviceImei()
{
    #if defined(BUILD_V3S)
    QString imei = extractImeiFromResponses("/MSM/potential-pancake/MSM/bin/at_axec/responses.txt");
    #elif defined(BUILD_X86)
    QString imei = "UNKNOWN";
    #endif
    return imei;
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
    QFile prj_file(PROJECT_JSON_PATH);
    if (!prj_file.exists()) return {};
        if (!EncryptionHelper::isEncrypted(PROJECT_JSON_PATH)){
            if (!Utils::copyUidToUidJson()) {
                return QString();
            }
            EncryptionHelper::check_and_encrypt_files(PROJECT_JSON_PATH,
                                                 "project.json", true, nullptr);
        }

    QFile file("config/uid.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            auto uid = obj.value("uid").toString();
            file.close();
            return uid;
        }
    }
    return QString();
}


QString getDevPassword()
{
    QFile file(PROJECT_JSON_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(PROJECT_JSON_PATH, Utils::getEncryptionKey());
        if (decryptedData.isEmpty()) return QString();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(decryptedData, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) return QString();
        
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            auto uid = obj.value("admin").toString();
            return uid;
        }
        file.close();
    }
    return QString();
}


QString getMqttStatus(Mode mode)
{
    auto w = MainWindow::instance();
    if (!w) return "Disconnected";

    bool connected = false;

    switch (mode) {
    case Factory:
        connected = (w->factoryMqttHandler && w->factoryMqttHandler->mqttState());
        break;

    case Admin:
        connected = (w->adminMqttHandler && w->adminMqttHandler->mqttState());
        break;
    }

    return connected ? "Connected" : "Disconnected";
}


QString getMqttsStatus(Mode mode)
{
    auto w = MainWindow::instance();
    if (!w) return "Disconnected";

    bool connected = false;

    switch (mode) {
    case Factory:
        connected = (w->factoryMqttsHandler && w->factoryMqttsHandler->awsMqttsState());
        break;

    case Admin:
        connected = (w->adminMqttsHandler && w->adminMqttsHandler->awsMqttsState());
        break;
    }

    return connected ? "Connected" : "Disconnected";
}


QString getSftpStatus(Mode mode)
{
    auto w = MainWindow::instance();
    if (!w) return "Disconnected";

    bool connected = false;

    switch (mode) {
    case Factory:
        connected = (w->factorySftpHandler && w->factorySftpHandler->sftpState());
        break;

    case Admin:
        connected = (w->adminSftpHandler && w->adminSftpHandler->sftpState());
        break;
    }

    return connected ? "Connected" : "Disconnected";
}


void rebootSystem()
{
    QString imgPath = ":/icons/rebooting.png";
    QLabel *label = showImageFullscreenFor(imgPath, -1);
    QTimer::singleShot(2000, [label]() {
    #if defined(BUILD_V3S)
        QProcess::startDetached("sh", {"-c", "sync; reboot"});
    #endif
    });
}


bool updateSerialInConfig(const QString &AwsEp, const QString &configJson_path)
{
    QFile file(configJson_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open config file for reading:" << configJson_path;
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
    QString uid = Utils::getUid();
    if (uid.isEmpty()) {
        qWarning() << "UID is empty, not updating Serial!";
        return false;
    }
    if(configJson_path == CONFIG_JSON_PATH)
        uid += "_factory";

    obj["serial"] = uid;
    obj["endpoint"] = AwsEp;

    doc.setObject(obj);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to open config file for writing:" << configJson_path;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}


QLabel *showImageFullscreenFor(const QString &imagePath, int milliseconds)
{
    QLabel *label = new QLabel;
    QPixmap pixmap(imagePath);

    if (pixmap.isNull()) {
        qWarning("Failed to load image");
        delete label;
        return nullptr;
    }

    label->setPixmap(pixmap);
    label->setFixedSize(pixmap.size());
    label->setScaledContents(false);
    label->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    label->move(0, 0);
    label->show();

    if(milliseconds != -1)
    {
        QTimer::singleShot(milliseconds, [label]() {
            label->close();
            label->deleteLater();
        });
    }

    return label;
}


QString getFirmwareVersion()
{
    QString softwareVersion;
    QFile hwFile("version.json");
    if (hwFile.open(QIODevice::ReadOnly)) {
        const auto data = hwFile.readAll();
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const auto obj = doc.object();
            softwareVersion = obj.value("software_version").toString();
        }
        hwFile.close();
    }

    return softwareVersion;
}


QString getHardwareVersion()
{
    QString hardwareVersion;
    QFile hwFile("version.json");
    if (hwFile.open(QIODevice::ReadOnly)) {
        const auto data = hwFile.readAll();
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const auto obj = doc.object();
            hardwareVersion = obj.value("hardware_version").toString();
        }
        hwFile.close();
    }

    return hardwareVersion;
}


QString getLastOTAInitiated()
{
    QString otaTime;
    QFile otaFile("/etc/ota_update.json");
    if (otaFile.open(QIODevice::ReadOnly)) {
        const auto data = otaFile.readAll();
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            const auto obj = doc.object();
            otaTime = obj.value("ota_update").toString();
        }
        otaFile.close();
    }

    return otaTime;
}


void setLastOTAInitiated()
{
    QFile file("/etc/ota_update.json");
    QJsonObject obj;

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject())
            obj = doc.object();
        file.close();
    }

    QString currentTime = QDateTime::currentDateTime().toString(Qt::ISODate);
    obj["ota_update"] = currentTime;

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QJsonDocument newDoc(obj);
        file.write(newDoc.toJson(QJsonDocument::Indented));
        file.close();
    }
}


QStringList getAvailableVersions()
{
    QStringList versionList;
    QFile versionsFile("/etc/ota_versions.json");

    if (!versionsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open version file:" << versionsFile.errorString();
        return versionList;  // return empty list if file can't be opened
    }

    QByteArray data = versionsFile.readAll();
    versionsFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return versionList;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format: expected an object at root.";
        return versionList;
    }

    QJsonObject rootObj = doc.object();
    QJsonArray versionsArray = rootObj.value("versions").toArray();

    for (const QJsonValue &val : versionsArray) {
        if (val.isString())
            versionList.append(val.toString());
    }

    return versionList;
}


QJsonObject readJsonFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        return QJsonObject();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return QJsonObject();
    }

    if (!doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}


void startAlignedTimer(
    QObject* context,
    QTimer* timer,
    int intervalSeconds,
    std::function<void()> firstTickCallback
) {
    if (!context || !timer || intervalSeconds <= 0)
        return;

    timer->stop();
    timer->setTimerType(Qt::PreciseTimer);

    QDateTime now = QDateTime::currentDateTimeUtc();

    int totalSeconds = now.time().hour() * 3600 +
                       now.time().minute() * 60 +
                       now.time().second();

    int nextAlignedSeconds = ((totalSeconds / intervalSeconds) + 1) * intervalSeconds;

    if (nextAlignedSeconds >= 24 * 3600) {
        nextAlignedSeconds -= 24 * 3600;
        now = now.addDays(1);
    }

    int hours = nextAlignedSeconds / 3600;
    int minutes = (nextAlignedSeconds % 3600) / 60;
    int seconds = nextAlignedSeconds % 60;

    QDateTime nextAligned(now.date(), QTime(hours, minutes, seconds), Qt::UTC);

    int initialDelayMs = int(now.msecsTo(nextAligned));
    if (initialDelayMs < 0)
        initialDelayMs = 0; // Safety fallback

    QTimer::singleShot(initialDelayMs, context, [=]() {
        if (firstTickCallback)
            firstTickCallback();

        timer->start(intervalSeconds * 1000);
    });
}


void exportAlignedTimer(
    QObject* context,
    int intervalMinutes,
    std::function<void()> callback
) {
    auto scheduleNext = [=]() {
        QDateTime now = QDateTime::currentDateTime();

        QTime t = now.time();
        int minute = t.minute();
        int nextMinute =
            ((minute / intervalMinutes) + 1) * intervalMinutes;

        QDateTime next = now;

        if (nextMinute >= 60) {
            next = next.addSecs(3600 - minute * 60 - t.second());
            next.setTime(QTime(next.time().hour(), 0, 0));
        } else {
            next.setTime(QTime(t.hour(), nextMinute, 0));
        }

        int delayMs = qMax(0LL, now.msecsTo(next));

        QTimer::singleShot(delayMs, context, [=]() {
            callback();
            exportAlignedTimer(context, intervalMinutes, callback);
        });
    };

    scheduleNext();
}


QString getEncryptionKey()
{
    static bool isKeyFetched = false;
    static QString key;
    if(!isKeyFetched)
    {
        key = generateEncryptionKey();
        if (key.isEmpty()) return QString();
        isKeyFetched = true;
    }
    return key;
}


QString getCpuSerial()
{
    QFile file("/proc/cpuinfo");
    if (!file.open(QIODevice::ReadOnly))
        return "";

    while (!file.atEnd()) {
        QString line = file.readLine();
        if (line.startsWith("Serial")) {
            return line.section(':', 1).trimmed();
        }
    }
    return "";
}


QString generateEncryptionKey()
{
    QString uid;
    QFile file("config/uid.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            uid = obj.value("uid").toString();
            file.close();
            if (uid.isEmpty()) return QString();
        }
    }
    else
    {
        return QString();
    }
    // QString cpuSerial = getCpuSerial();   // empty on PC is OK
    QString salt = "MSM@ADMIN@2025";      // NEVER change

    if (uid.isEmpty()) return QString();
    QByteArray material = (uid + salt).toUtf8(); //(uid + cpuSerial + salt)
    QByteArray hash = QCryptographicHash::hash(
        material, QCryptographicHash::Sha256);
    return hash.toHex(); // 32 bytes → AES‑256
}


bool copyUidToUidJson()
{
    QFile inFile(PROJECT_JSON_PATH);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(inFile.readAll(), &parseError);
    inFile.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QString projectUid = doc.object().value("uid").toString();
    if (projectUid.isEmpty()) {
        return false;
    }
    QFile uidFile("config/uid.json");
    if (uidFile.exists()) {
        if (uidFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument existingDoc = QJsonDocument::fromJson(uidFile.readAll(), &parseError);
            uidFile.close();

            if (parseError.error == QJsonParseError::NoError && existingDoc.isObject()) {
                QString existingUid = existingDoc.object().value("uid").toString();
                if (existingUid == projectUid) {
                    return true; // UID already up-to-date
                }
            }
        }
    }
    QJsonObject uidObj;
    uidObj.insert("uid", projectUid);
    if (!uidFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    QByteArray payload = QJsonDocument(uidObj).toJson(QJsonDocument::Compact);
    qint64 written = uidFile.write(payload);
    if (written != payload.size()) {
        qWarning() << "Incomplete write to uid.json";
        uidFile.close();
        return false;
    }
    
    // CRITICAL: Flush to OS buffers
    if (!uidFile.flush()) {
        qWarning() << "uid.json flush failed";
        uidFile.close();
        return false;
    }
    
    // CRITICAL: Sync to actual storage device
    int fd = uidFile.handle();
    if (fd != -1) {
        if (fsync(fd) != 0) {
            qWarning() << "uid.json fsync failed";
            uidFile.close();
            return false;
        }
    }
    
    uidFile.close();
    sync();
    
    // qDebug() << "uid.json updated with UID:" << projectUid;
    return true;
}


}
