#include "serialhandler.h"
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent) : QObject(parent) {
    connect(&serial, &QSerialPort::readyRead, this, &SerialHandler::handleReadyRead);
}

bool SerialHandler::open(const QString &portName, qint32 baudRate) {
    if (serial.isOpen())
        return true;
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);
    return serial.open(QIODevice::ReadWrite);
}

void SerialHandler::sendCommand(const QString &command, int channel) {
    currentChannel = channel;
    if(channel == -1){
        serial.clear(QSerialPort::AllDirections); // Clears both input & output buffers
    }
    buffer.clear();
    serial.write((command + "\n").toUtf8());
}


void SerialHandler::handleReadyRead() {
    buffer.append(serial.readAll());
    if (buffer.endsWith("}\n")) {
        QByteArray raw = buffer.trimmed();
        // Replace invalid JSON tokens (NaN, Inf, -Inf) with null
        QByteArray sanitized = raw;
        sanitized.replace("NaN", "null");
        sanitized.replace("nan", "null");
        sanitized.replace("-inf", "null");
        sanitized.replace("-INF", "null");
        sanitized.replace("inf", "null");
        sanitized.replace("INF", "null");

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(sanitized, &err);
        buffer.clear();

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();

            // Handle calibration/CT JSONs as string responses
            if (obj.contains("command")) {
                QString cmd = obj.value("command").toString();
                if (cmd == "reboot-calibration" || cmd == "v_offset" || cmd == "i_offset" || cmd == "i-calibration" || cmd == "v-calibration" || cmd == "ct-ratio") {
                    QString jsonString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
                    emit stringResponseReady(jsonString);
                    return;
                }
            }
            emit jsonResponseReady(currentChannel, obj);
        }
    }
}
