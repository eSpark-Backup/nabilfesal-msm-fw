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
    buffer.clear();
    serial.write((command + "\n").toUtf8());
}

void SerialHandler::handleReadyRead() {
    buffer.append(serial.readAll());
    if (buffer.endsWith("}\n") || buffer.endsWith("}")) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(buffer.trimmed(), &err);
        buffer.clear();

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            emit jsonResponseReady(currentChannel, doc.object());
        }
    }
}
