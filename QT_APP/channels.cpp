#include "channels.h"
#include "ui_channels.h"
#include <QTimer>
#include <QJsonDocument>

ChannelsHome::ChannelsHome(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChannelsHome)
{
    ui->setupUi(this);
    handler = new SerialHandler;
    handler->open("/dev/pts/2", 115200);

    QList<QPushButton*> buttons = {
        ui->ch_home_bt,
        ui->ch_home_bt_2,
        ui->ch_home_bt_3,
        ui->ch_home_bt_4,
        ui->ch_home_bt_5,
        ui->ch_home_bt_6,
        ui->ch_home_bt_7,
        ui->ch_home_bt_8,
        ui->ch_home_bt_9,
        ui->ch_home_bt_10,
        ui->ch_home_bt_11,
        ui->ch_home_bt_12
    };

    for (QPushButton *btn : buttons) {
        connect(btn, &QPushButton::clicked, this, &ChannelsHome::onAnyHomeButtonClicked);
    }

    for (int i = 1; i <= 12; ++i) {
        QString suffix = QString::number(i);

        // Main channel button
        if (QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + suffix)) {
            connect(chBtn, &QPushButton::clicked, this, [=]() {
                handleChannelClick(i);
            });
        }

        // Sub-buttons (gn, en, pw)
        if (QPushButton* gnBtn = findChild<QPushButton*>("gn_bt_" + suffix)) {
            connect(gnBtn, &QPushButton::clicked, this, [=]() {
                handleSubButtonClick(i, "gn");
            });
        }

        if (QPushButton* enBtn = findChild<QPushButton*>("en_bt_" + suffix)) {
            connect(enBtn, &QPushButton::clicked, this, [=]() {
                handleSubButtonClick(i, "en");
            });
        }

        if (QPushButton* pwBtn = findChild<QPushButton*>("pw_bt_" + suffix)) {
            connect(pwBtn, &QPushButton::clicked, this, [=]() {
                handleSubButtonClick(i, "pw");
            });
        }
    }

    QTimer *channelPollTimer = new QTimer(this);
    connect(channelPollTimer, &QTimer::timeout, this, &ChannelsHome::pollAllChannels);
    channelPollTimer->start(15000);

    ui->stackedWidget->setCurrentIndex(0);
}

ChannelsHome::~ChannelsHome()
{
    delete ui;
}

void ChannelsHome::handleChannelClick(int channelIndex)
{
    ui->stackedWidget->setCurrentIndex(channelIndex);
    handleSubButtonClick(channelIndex, "gn");  // default to General tab
}

void ChannelsHome::handleSubButtonClick(int channelIndex, const QString &buttonType)
{
    QString suffix = QString::number(channelIndex);

    QPushButton* gn = findChild<QPushButton*>("gn_bt_" + suffix);
    QPushButton* en = findChild<QPushButton*>("en_bt_" + suffix);
    QPushButton* pw = findChild<QPushButton*>("pw_bt_" + suffix);
    QStackedWidget* channelStack = findChild<QStackedWidget*>("st_ch_" + suffix);

    if (!gn || !en || !pw || !channelStack)
        return;

    gn->setStyleSheet(buttonNormalStyle);
    en->setStyleSheet(buttonNormalStyle);
    pw->setStyleSheet(buttonNormalStyle);

    if (buttonType == "gn") {
        gn->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(0);
    } else if (buttonType == "pw") {
        pw->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(1);
    } else if (buttonType == "en") {
        en->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(2);
    }
}

void ChannelsHome::onAnyHomeButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void ChannelsHome::pollAllChannels()
{
    for (int i = 1; i <= 12; ++i) {
        QString command = QString("channel-%1").arg(i);
        QJsonObject data = handler->sendCommandAndWait(command);

        if (!data.isEmpty()) {
            updateChannelUI(i, data);
        } else {
            QString suffix = QString::number(i);
            QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + suffix);
            chBtn->setDisabled(true);
        }
    }
}

void ChannelsHome::updateChannelUI(int channel, const QJsonObject &json)
{
    bool status = json.value("status").toBool();
    QString suffix = QString::number(channel);
    QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + suffix);
    chBtn->setDisabled(!status);

    double temperature = json.value("temperature").toDouble();

    QJsonObject data = json.value("data").toObject();

    QStringList phases = {"phase_a", "phase_b", "phase_c"};
    for (const QString &phase : phases) {
        QJsonObject phaseObj = data.value(phase).toObject();

        QJsonObject general = phaseObj.value("general").toObject();
        double lineVoltage = general.value("line_voltage").toDouble();
        double rmsVoltage = general.value("rms_voltage").toDouble();
        double frequency = general.value("frequency").toDouble();
        double current = general.value("current").toDouble();

        QJsonObject power = phaseObj.value("power").toObject();
        double factor = power.value("factor").toDouble();
        double active = power.value("active").toDouble();
        double reactive = power.value("reactive").toDouble();
        double apparent = power.value("apparent").toDouble();

        QJsonObject threshold = phaseObj.value("threshold").toObject();
        double thCurrent = threshold.value("current").toDouble();
        double thVoltage = threshold.value("voltage").toDouble();
        double thAngle = threshold.value("angle").toDouble();

        QJsonObject energy = phaseObj.value("energy").toObject();
        QJsonObject activeEnergy = energy.value("active").toObject();
        QJsonObject reactiveEnergy = energy.value("reactive").toObject();
        double positiveEnergy = activeEnergy.value("positive").toDouble();
        double negativeEnergy = activeEnergy.value("negative").toDouble();
        double biEnergy = reactiveEnergy.value("positive").toDouble();

        QString prefix;
        if (phase == "phase_a") prefix = "a_";
        else if (phase == "phase_b") prefix = "b_";
        else if (phase == "phase_c") prefix = "c_";

        QLabel* lbl = nullptr;

        lbl = findChild<QLabel*>("rms_v_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(rmsVoltage, 'f', 2));

        lbl = findChild<QLabel*>("line_v_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(lineVoltage, 'f', 2));

        lbl = findChild<QLabel*>("rms_i_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(current, 'f', 2));

        lbl = findChild<QLabel*>("f_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(frequency, 'f', 2));

        lbl = findChild<QLabel*>("ap_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(apparent, 'f', 2));

        lbl = findChild<QLabel*>("acp_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(active, 'f', 2));

        lbl = findChild<QLabel*>("rp_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(reactive, 'f', 2));

        lbl = findChild<QLabel*>("pf_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(factor, 'f', 2));

        lbl = findChild<QLabel*>("pe_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(positiveEnergy, 'f', 2));

        lbl = findChild<QLabel*>("ne_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(negativeEnergy, 'f', 2));

        lbl = findChild<QLabel*>("be_" + prefix + suffix);
        if (lbl) lbl->setText(QString::number(biEnergy, 'f', 2));

    }

}
