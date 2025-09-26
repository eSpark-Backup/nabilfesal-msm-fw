#include "channels.h"
#include "ui_channels.h"
#include <QJsonDocument>
#include <QDebug>

ChannelsHome::ChannelsHome(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChannelsHome)
    ,m_channel(1)
{
    ui->setupUi(this);

    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    connect(ui->ch_home_bt, &QPushButton::clicked, this, &ChannelsHome::onAnyHomeButtonClicked);

    for (int i = 1; i <= 12; ++i) {
        QString suffix = QString::number(i);

        // Main channel button
        if (QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + suffix)) {
            connect(chBtn, &QPushButton::clicked, this, [=]() {
                handleChannelClick(i);
            });
        }
    }

    connect(ui->gn_bt_1, &QPushButton::clicked, this, [=]() {
        handleSubButtonClick(1, "gn");
    });


    connect(ui->en_bt_1, &QPushButton::clicked, this, [=]() {
        handleSubButtonClick(1, "en");
    });



    connect(ui->pw_bt_1, &QPushButton::clicked, this, [=]() {
        handleSubButtonClick(1, "pw");
    });

    // Cache label pointers once for speed
    cacheChannelLabels();

    ui->stackedWidget->setCurrentIndex(0);
}

ChannelsHome::~ChannelsHome()
{
    delete ui;
}

void ChannelsHome::handleChannelClick(int channelIndex)
{
    m_channel = channelIndex;
    ui->channel_label->setText("CHANNEL " + QString::number(channelIndex));
    ui->stackedWidget->setCurrentIndex(1);
    handleSubButtonClick(1, "gn");
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
        energyCounter = 0;
        en->setText("Energy\n");
        gn->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(0);
    } else if (buttonType == "pw") {
        energyCounter = 0;
        en->setText("Energy\n");
        pw->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(1);
    } else if (buttonType == "en") {
        energyCounter++;
        if (energyCounter == 1)
        {
            en->setText("Active\nEnergy\n");
        }
        else if (energyCounter == 2)
        {
            en->setText("Reactive\nEnergy\n");
            energyCounter = 0;
        }
        en->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(2);
    }
}

void ChannelsHome::onAnyHomeButtonClicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void ChannelsHome::handleChannelData(int channel, const QJsonObject &json) {
    updateChannelUI(channel, json);
}

void ChannelsHome::cacheChannelLabels() {
    QStringList prefixes = {"a_", "b_", "c_"};
    QStringList fields = {"rms_v_", "line_v_", "rms_i_", "f_", "ap_", "acp_", "rp_", "pf_", "pe_", "ne_", "be_"};

    QMap<QString, QLabel*> map;
    QString suffix = QString::number(1);
    for (const QString &p : prefixes) {
        for (const QString &f : fields) {
            QString objName = f + p + suffix;
            QLabel *lbl = findChild<QLabel*>(objName);
            if (lbl) {
                map[objName] = lbl;
            }
        }
    }
    channelLabelMap[1] = map;
}

void ChannelsHome::updateChannelUI(int channel, const QJsonObject &json) {
    QString suffix = QString::number(channel);
    QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + suffix);
    auto status = json.value("status").toInt();
    if (chBtn) chBtn->setDisabled(status != 1);

    if (channel == m_channel)
    {
        double temp = json.value("temperature").toDouble();
        ui->temp->setText(QString::number(temp, 'f', 2));
        QString suffix = QString::number(1);
        QJsonObject data = json.value("data").toObject();
        for (const QString &phase : {"phase_a", "phase_b", "phase_c"}) {
            QJsonObject phaseObj = data.value(phase).toObject();
            QString prefix = (phase == "phase_a") ? "a_" :
                                 (phase == "phase_b") ? "b_" : "c_";

            QJsonObject general = phaseObj.value("general").toObject();
            QJsonObject power = phaseObj.value("power").toObject();
            QJsonObject energy = phaseObj.value("energy").toObject();

            QJsonObject activeEnergy = energy.value("active").toObject();
            QJsonObject reactiveEnergy = energy.value("reactive").toObject();

            auto setValue = [&](const QString &key, double val) {
                QString name = key + prefix + suffix;
                if (channelLabelMap[1].contains(name)) {
                    channelLabelMap[1][name]->setText(QString::number(val, 'f', 2));
                }
            };

            setValue("rms_v_", general.value("rms_voltage").toDouble());
            setValue("line_v_", general.value("line_voltage").toDouble());
            setValue("rms_i_", general.value("current").toDouble());
            setValue("f_", general.value("frequency").toDouble());
            setValue("ap_", power.value("apparent").toDouble());
            setValue("acp_", power.value("active").toDouble());
            setValue("rp_", power.value("reactive").toDouble());
            setValue("pf_", power.value("factor").toDouble());

            auto positiveEnergy = 0.0;
            auto negativeEnergy = 0.0;
            auto bidirectionalEnergy = positiveEnergy + negativeEnergy;

            if (energyCounter == 0)
            {
                positiveEnergy = activeEnergy.value("positive").toDouble();
                negativeEnergy = activeEnergy.value("negative").toDouble();
                bidirectionalEnergy = positiveEnergy + negativeEnergy;
            }
            else if (energyCounter == 1)
            {
                positiveEnergy = activeEnergy.value("positive").toDouble();
                negativeEnergy = activeEnergy.value("negative").toDouble();
                bidirectionalEnergy = positiveEnergy + negativeEnergy;
            }
                
            else if (energyCounter == 2)
            {
                positiveEnergy = reactiveEnergy.value("positive").toDouble();
                negativeEnergy = reactiveEnergy.value("negative").toDouble();
                bidirectionalEnergy = positiveEnergy + negativeEnergy;
            }

            setValue("pe_", positiveEnergy);
            setValue("ne_", negativeEnergy);
            setValue("be_", bidirectionalEnergy);            
        }
    }
}

void ChannelsHome::on_exit_clicked()
{
    emit channelsExit();
}
