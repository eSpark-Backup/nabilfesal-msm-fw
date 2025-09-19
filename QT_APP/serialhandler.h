#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H

#include <QObject>
#include <QSerialPort>
#include <QJsonObject>

class SerialHandler : public QObject {
    Q_OBJECT
public:
    explicit SerialHandler(QObject *parent = nullptr);
    bool open(const QString &portName, qint32 baudRate);
    void sendCommand(const QString &command, int channel);

signals:
    void jsonResponseReady(int channel, QJsonObject);
    void stringResponseReady(const QString &response);

private slots:
    void handleReadyRead();

private:
    QSerialPort serial;
    QByteArray buffer;
    int currentChannel = -1;
};

#endif // SERIALHANDLER_H
