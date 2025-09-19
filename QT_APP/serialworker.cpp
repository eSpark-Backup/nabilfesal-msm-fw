#include "serialworker.h"
#include "serialhandler.h"
#include <QThread>
#include <QTimer>

SerialWorker::SerialWorker(const QString &port, int baud)
    : portName(port), baudRate(baud) {

    handler = new SerialHandler;
    handler->moveToThread(QThread::currentThread());
    handler->open(portName, baudRate);

    connect(handler, &SerialHandler::jsonResponseReady,
            this, &SerialWorker::handleJsonResponse);

    connect(handler, &SerialHandler::stringResponseReady,
            this, &SerialWorker::handleStringResponse);

}

void SerialWorker::requestChannel(int channel) {
    channelQueue.enqueue(channel);
    if (!processing)
        QTimer::singleShot(0, this, &SerialWorker::processQueue);
}

void SerialWorker::processQueue() {
    if (channelQueue.isEmpty()) {
        processing = false;
        return;
    }

    processing = true;
    int channel = channelQueue.dequeue();
    QString cmd = QString("channel-%1").arg(channel);
    handler->sendCommand(cmd, channel);

    // If no response within 1s, assume error and process next
    QTimer::singleShot(200, this, [=] {
        if (processing) {
            emit channelError(channel);
            QTimer::singleShot(0, this, &SerialWorker::processQueue);
        }
    });
}

void SerialWorker::handleJsonResponse(int channel, const QJsonObject &json) {
    emit channelDataReady(channel, json);
    processing = false;
    QTimer::singleShot(0, this, &SerialWorker::processQueue);
}

void SerialWorker::sendRawCommand(const QString &cmd) {
    handler->sendCommand(cmd, -1);
}

void SerialWorker::handleStringResponse(const QString &resp) {
    emit genericResponseReady(resp);
}