#ifndef CHANNELS_H
#define CHANNELS_H

#include <QWidget>
#include "serialhandler.h"
#include "serialworker.h"
#include <QLabel>
#include <QThread>
#include <QTimer>

namespace Ui {
class ChannelsHome;
}

class ChannelsHome : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelsHome(QWidget *parent = nullptr);
    ~ChannelsHome();

signals:
    void requestChannelPoll(int channel);
    void channelsExit();
    void deviceInfo();
private slots:

    void on_exit_clicked();

    void on_info_clicked();

private:
    void handleChannelClick(int channelIndex);
    void handleSubButtonClick(int channelIndex, const QString &buttonType);
    void onAnyHomeButtonClicked();
    void pollAllChannels();
    void updateChannelUI(int channel, const QJsonObject &data);
    void cacheChannelLabels();
    void handleChannelData(int channel, const QJsonObject &json);
    void handleChannelError(int channel);

private:
    Ui::ChannelsHome *ui;
    SerialHandler *handler;
    QString buttonClickedStyle =
        "border: 2px solid #00CCFF;"
        "color: rgb(255, 255, 255);"
        "border-radius: 15px;"
        "background-color: #00CCFF;";
    QString buttonNormalStyle =
        "border: 2px solid #D9D9D9;"
        "color: #0C89E2;"
        "border-radius: 15px;"
        "background-color: #D9D9D9;";

    QMap<int, QMap<QString, QLabel*>> channelLabelMap;

    QThread *serialThread;
    SerialWorker *worker;
    QTimer *pollTimer;
    int m_channel;

};

#endif // CHANNELS_H
