#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QJsonObject>

class SerialHandler : public QObject
{
    Q_OBJECT

public:
    explicit SerialHandler(QObject *parent = nullptr);

    bool open(const QString &portName, qint32 baudRate);
    QJsonObject sendCommandAndWait(const QString &command, int timeout = 3000);

signals:
    void jsonResponseReady(const QJsonObject &json);

private slots:
    void handleReadyRead();

private:
    QSerialPort serial;
    QByteArray buffer;
    QJsonObject receivedJson;
    bool jsonReceived = false;
};

#endif // SERIALHANDLER_H
