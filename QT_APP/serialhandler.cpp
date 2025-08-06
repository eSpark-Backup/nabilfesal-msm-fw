#include "serialhandler.h"
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

SerialHandler::SerialHandler(QObject *parent)
    : QObject(parent)
{
    connect(&serial, &QSerialPort::readyRead, this, &SerialHandler::handleReadyRead);
}

bool SerialHandler::open(const QString &portName, qint32 baudRate)
{
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);
    return serial.open(QIODevice::ReadWrite);
}

QJsonObject SerialHandler::sendCommandAndWait(const QString &command, int timeout)
{
    buffer.clear();
    jsonReceived = false;
    receivedJson = QJsonObject();

    serial.write((command + "\n").toUtf8());

    QEventLoop loop;
    QTimer timer;

    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.setSingleShot(true);
    timer.start(timeout);

    connect(this, &SerialHandler::jsonResponseReady, &loop, &QEventLoop::quit);

    loop.exec();

    if (jsonReceived) {
        return receivedJson;
    }

    return QJsonObject();
}

void SerialHandler::handleReadyRead()
{
    buffer.append(serial.readAll());

    if (buffer.endsWith("}\n") || buffer.endsWith("}")) {
        buffer = buffer.trimmed();  // Remove newlines/spaces

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(buffer, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            if (doc.isObject()) {
                receivedJson = doc.object();
                jsonReceived = true;
                emit jsonResponseReady(receivedJson);
            } else {
                QVariant var = doc.toVariant();
                if (var.type() == QVariant::String) {
                    // Decode the inner string
                    QString jsonString = doc.toVariant().toString();
                    QJsonDocument innerDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && innerDoc.isObject()) {
                        receivedJson = innerDoc.object();
                        jsonReceived = true;
                        emit jsonResponseReady(receivedJson);
                    } else {
                        qDebug() << "Inner JSON parse error:" << parseError.errorString();
                    }
                } else {
                    qDebug() << "Unexpected JSON structure";
                }
            }

            buffer.clear();
        }
    }
}
