#ifndef CHANNELS_H
#define CHANNELS_H

#include <QWidget>
#include "serialhandler.h"
#include "serialworker.h"
#include <QLabel>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QJsonObject>
#include <QPoint>
#include <QSwipeGesture>

namespace Ui {
class ChannelsHome;
}

class ChannelsHome : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelsHome(QString hardwareVersion, QString softwareVersion, QWidget *parent = nullptr);
    ~ChannelsHome();
    void handleChannelData(int channel, const QJsonObject &json);

signals:
    void channelsExit();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_exit_clicked();
    void on_tempBtn_clicked(bool updateTrue = false, double val = 0.00);

private:
    void handleChannelClick(int channelIndex, bool preserveSubTab = false);
    void handleSubButtonClick(int channelIndex, const QString &buttonType);
    void onAnyHomeButtonClicked();
    void updateChannelUI(int channel, const QJsonObject &data);
    void cacheChannelLabels();
    void handleSwipeGesture(QSwipeGesture *gesture);
    void handleManualSwipe(const QPoint &startPos, const QPoint &endPos);
    void navigateToChannel(int newChannel);
    int findNextActiveChannel(int startChannel, int direction);
    void loadEnergyData();
    void saveEnergyData();

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

    int m_channel;
    int energyCounter = 0;

    QPoint m_swipeStartPos;
    bool m_isSwipeInProgress = false;
    int m_minSwipeDistance;

    struct EnergyData {
        double activePositive = 0.0;
        double activeNegative = 0.0;
        double reactivePositive = 0.0;
        double reactiveNegative = 0.0;
    };

    QMap<int, QMap<QString, EnergyData>> accumulatedEnergy; 

    QWidget *leftShadowOverlay = nullptr;
    QWidget *rightShadowOverlay = nullptr;
    
    void setupShadowOverlay(QWidget* overlay, bool isLeft);
    void resizeEvent(QResizeEvent *event);
};

#endif // CHANNELS_H
