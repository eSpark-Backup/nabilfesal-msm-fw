// serialworker.h
#pragma once
#include <QObject>
#include <QJsonObject>
#include <QQueue>
#include "serialhandler.h"

class SerialWorker : public QObject {
    Q_OBJECT
public:
    SerialWorker(const QString &port, int baud);

public slots:
    void requestChannel(int channel);
    void sendRawCommand(const QString &cmd);

signals:
    void channelDataReady(int channel, const QJsonObject &);
    void channelError(int channel);
    void genericResponseReady(QString response);

private slots:
    void handleJsonResponse(int channel, const QJsonObject &json);
    void processQueue();
    void handleStringResponse(const QString &resp);

private:
    QString portName;
    int baudRate;
    SerialHandler *handler;
    QQueue<int> channelQueue;
    bool processing = false;
};

