#include "channels.h"
#include "ui_channels.h"
#include <QJsonDocument>
#include <QDebug>
#include <QGestureEvent>
#include <QSwipeGesture>
#include <QPainter>
#include <QPainterPath>
#include "toast.h"


#define EnergyFilePath "data/energy_data.json"

ChannelsHome::ChannelsHome(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChannelsHome)
    , m_channel(1)
    , m_swipeStartPos(0, 0)
    , m_minSwipeDistance(100)
{
    ui->setupUi(this);

    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);
    
    // Create and configure shadow overlays once
    leftShadowOverlay = new QWidget(ui->stackedWidget->currentWidget());
    rightShadowOverlay = new QWidget(ui->stackedWidget->currentWidget());
    setupShadowOverlay(leftShadowOverlay, true);
    setupShadowOverlay(rightShadowOverlay, false);

    ui->stackedWidget->widget(1)->grabGesture(Qt::SwipeGesture);
    ui->stackedWidget->widget(1)->installEventFilter(this);
    ui->stackedWidget->widget(1)->setMouseTracking(true);

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

    loadEnergyData();

    QTimer *saveTimer = new QTimer(this);
    connect(saveTimer, &QTimer::timeout, this, &ChannelsHome::saveEnergyData);
    saveTimer->start(5 * 60 * 1000); // Save every 5 minutes
}


ChannelsHome::~ChannelsHome()
{
    saveEnergyData();
    delete ui;
}


void ChannelsHome::setupShadowOverlay(QWidget* overlay, bool isLeft)
{
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    overlay->setAttribute(Qt::WA_NoSystemBackground);
    overlay->setAttribute(Qt::WA_TranslucentBackground);
    overlay->setGeometry(0, 0, ui->stackedWidget->width(), ui->stackedWidget->height());
    overlay->hide();

    // Store direction flag
    overlay->setProperty("isLeftShadow", isLeft);

    // Important: tell ChannelsHome to filter paint events for this overlay
    overlay->installEventFilter(this);
}


bool paintShadowEventFilter(QObject *obj, QEvent *event)
{
    if (event->type() != QEvent::Paint)
        return false;

    QWidget *overlay = static_cast<QWidget*>(obj);
    bool isLeft = overlay->property("isLeftShadow").toBool();
    int shadowWidth = overlay->property("shadowWidth").toInt();
    int shadowAlpha = overlay->property("shadowAlpha").toInt();

    if (shadowWidth <= 0) shadowWidth = 60;
    if (shadowAlpha <= 0) shadowAlpha = 70;

    QPainter p(overlay);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const int h = overlay->height();
    const int w = overlay->width();

    QColor baseColor(90, 90, 90, shadowAlpha);  // RGB 90–90–90 gives a soft gray
    p.setBrush(baseColor);
    p.setPen(Qt::NoPen);

    QPainterPath path;

    // Shift the semicircle down (10% from the top)
    const int topOffset = h * 0.10;
    const int effectiveHeight = h - topOffset;

    if (isLeft) {
        QRectF arcRect(-shadowWidth, topOffset, shadowWidth * 2, effectiveHeight);
        path.moveTo(0, topOffset);
        path.arcTo(arcRect, 270, 180);
        path.lineTo(0, h);
        path.closeSubpath();
    } else {
        QRectF arcRect(w - shadowWidth, topOffset, shadowWidth * 2, effectiveHeight);
        path.moveTo(w, topOffset);
        path.arcTo(arcRect, 90, 180);
        path.lineTo(w, h);
        path.closeSubpath();
    }

    p.drawPath(path);
    return true;
}


bool ChannelsHome::eventFilter(QObject *obj, QEvent *event)
{
    // Handle shadow overlay painting
    if (obj == leftShadowOverlay || obj == rightShadowOverlay) {
        if (event->type() == QEvent::Paint) {
            paintShadowEventFilter(obj, event);
            return true;
        }
    }

    QWidget *current = ui->stackedWidget->currentWidget();
    if (obj != current)
        return QWidget::eventFilter(obj, event);

    // Initialize overlays once per current page
    if (!leftShadowOverlay || !rightShadowOverlay) {
        leftShadowOverlay = new QWidget(current);
        rightShadowOverlay = new QWidget(current);
        setupShadowOverlay(leftShadowOverlay, true);
        setupShadowOverlay(rightShadowOverlay, false);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        m_swipeStartPos = mouseEvent->pos();
        m_isSwipeInProgress = true;
    }
    else if (event->type() == QEvent::MouseMove && m_isSwipeInProgress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int deltaX = mouseEvent->pos().x() - m_swipeStartPos.x();

        // Calculate intensity and width dynamically
        double intensity = qMin(1.0, qAbs(deltaX) / 250.0);
        int maxWidth = 120; // max expansion of the curved shadow
        int dynamicWidth = qBound(30, int(maxWidth * intensity), maxWidth);
        int alpha = int(100 * intensity); // dynamic transparency

        if (deltaX > 0) { // Swiping right
            leftShadowOverlay->show();
            rightShadowOverlay->hide();

            leftShadowOverlay->update();
            leftShadowOverlay->setProperty("shadowWidth", dynamicWidth);
            leftShadowOverlay->setProperty("shadowAlpha", alpha);
        }
        else if (deltaX < 0) { // Swiping left
            rightShadowOverlay->show();
            leftShadowOverlay->hide();

            rightShadowOverlay->update();
            rightShadowOverlay->setProperty("shadowWidth", dynamicWidth);
            rightShadowOverlay->setProperty("shadowAlpha", alpha);
        }

        // Trigger repaint
        leftShadowOverlay->update();
        rightShadowOverlay->update();
    }
    else if (event->type() == QEvent::MouseButtonRelease && m_isSwipeInProgress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        handleManualSwipe(m_swipeStartPos, mouseEvent->pos());
        m_isSwipeInProgress = false;

        leftShadowOverlay->hide();
        rightShadowOverlay->hide();
    }

    // Still handle swipe gestures if available
    if (event->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = static_cast<QGestureEvent*>(event);
        if (QGesture *swipe = gestureEvent->gesture(Qt::SwipeGesture)) {
            handleSwipeGesture(static_cast<QSwipeGesture*>(swipe));
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}


void ChannelsHome::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (leftShadowOverlay && rightShadowOverlay) {
        leftShadowOverlay->setGeometry(0, 0, ui->stackedWidget->width(), ui->stackedWidget->height());
        rightShadowOverlay->setGeometry(0, 0, ui->stackedWidget->width(), ui->stackedWidget->height());
    }
}


void ChannelsHome::handleSwipeGesture(QSwipeGesture *gesture)
{
    if (gesture->state() == Qt::GestureFinished) {
        if (gesture->horizontalDirection() == QSwipeGesture::Right) {
            navigateToChannel(m_channel - 1);
        }
        else if (gesture->horizontalDirection() == QSwipeGesture::Left) {
            navigateToChannel(m_channel + 1);
        }
    }
}

void ChannelsHome::handleManualSwipe(const QPoint &startPos, const QPoint &endPos)
{
    int deltaX = endPos.x() - startPos.x();
    int deltaY = endPos.y() - startPos.y();
    
    if (qAbs(deltaX) > qAbs(deltaY) && qAbs(deltaX) > m_minSwipeDistance) {
        if (deltaX > 0) {
            navigateToChannel(m_channel - 1);
        } else {
            navigateToChannel(m_channel + 1);
        }
    }
}

void ChannelsHome::navigateToChannel(int newChannel)
{
    int direction = (newChannel > m_channel) ? 1 : -1;
    int activeChannel = findNextActiveChannel(m_channel, direction);

    if (activeChannel != m_channel) {
        handleChannelClick(activeChannel, true);
    }
}

void ChannelsHome::handleChannelClick(int channelIndex, bool preserveSubTab)
{
    m_channel = channelIndex;
    ui->channel_label->setText("CHANNEL " + QString::number(channelIndex));
    ui->stackedWidget->setCurrentIndex(1);
    if (!preserveSubTab) {
        handleSubButtonClick(1, "gn");
    }
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
        en->setText("Energy");
        gn->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(0);
    } else if (buttonType == "pw") {
        energyCounter = 0;
        en->setText("Energy");
        pw->setStyleSheet(buttonClickedStyle);
        channelStack->setCurrentIndex(1);
    } else if (buttonType == "en") {
        energyCounter++;
        if (energyCounter == 1)
        {
            en->setText("Active\nEnergy");
            ui->label_159->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_162->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_165->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_168->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_171->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_174->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_177->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_180->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
            ui->label_183->setText(QCoreApplication::translate("ChannelsHome", "kWh", nullptr));
        }
        else if (energyCounter == 2)
        {
            en->setText("Reactive\nEnergy");
            ui->label_159->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_162->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_165->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_168->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_171->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_174->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_177->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_180->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
            ui->label_183->setText(QCoreApplication::translate("ChannelsHome", "kVAR", nullptr));
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

    QJsonObject data = json.value("data").toObject();
    for (const QString &phase : {"phase_a", "phase_b", "phase_c"}) {
        QJsonObject phaseObj = data.value(phase).toObject();
        QJsonObject energyObj = phaseObj.value("energy").toObject();
        QJsonObject activeEnergy = energyObj.value("active").toObject();
        QJsonObject reactiveEnergy = energyObj.value("reactive").toObject();

        double activePos = activeEnergy.value("positive").toDouble();
        double activeNeg = activeEnergy.value("negative").toDouble();
        double reactivePos = reactiveEnergy.value("positive").toDouble();
        double reactiveNeg = reactiveEnergy.value("negative").toDouble();

        QString phaseKey = (phase == "phase_a") ? "a" :
                           (phase == "phase_b") ? "b" : "c";

        auto &acc = accumulatedEnergy[channel][phaseKey];
        acc.activePositive  += activePos;
        acc.activeNegative  += activeNeg;
        acc.reactivePositive += reactivePos;
        acc.reactiveNegative += reactiveNeg;
    }

    if (channel == m_channel)
    {
        double temp = json.value("temperature").toDouble();
        on_tempBtn_clicked(true, temp);
        QString suffix = QString::number(1);
        QJsonObject data = json.value("data").toObject();
        for (const QString &phase : {"phase_a", "phase_b", "phase_c"}) {
            QJsonObject phaseObj = data.value(phase).toObject();
            QString prefix = (phase == "phase_a") ? "a_" :
                                 (phase == "phase_b") ? "b_" : "c_";

            QJsonObject general = phaseObj.value("general").toObject();
            QJsonObject power = phaseObj.value("power").toObject();

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
           
        }

        for (const QString &phase : {"a", "b", "c"}) {
            const auto &acc = accumulatedEnergy[channel][phase];

            double positiveEnergy = 0.0;
            double negativeEnergy = 0.0;

            if (energyCounter == 0) {
                positiveEnergy = acc.activePositive;
                negativeEnergy = acc.activeNegative;
            } else if (energyCounter == 1) { 
                positiveEnergy = acc.activePositive;
                negativeEnergy = acc.activeNegative;
            }  else if (energyCounter == 2) { 
                positiveEnergy = acc.reactivePositive;
                negativeEnergy = acc.reactiveNegative;
            }

            double bidirectionalEnergy = positiveEnergy + negativeEnergy;

            auto setValue = [&](const QString &key, double val) {
                QString name = key + phase + "_1";
                if (channelLabelMap[1].contains(name))
                    channelLabelMap[1][name]->setText(QString::number(val, 'f', 2));
            };

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


int ChannelsHome::findNextActiveChannel(int startChannel, int direction)
{
    int newChannel = startChannel;

    for (int i = 0; i < 12; ++i) {
        newChannel += direction;
        if (newChannel < 1) newChannel = 12;
        else if (newChannel > 12) newChannel = 1;

        QPushButton* chBtn = findChild<QPushButton*>("channel_bt_" + QString::number(newChannel));
        if (chBtn && chBtn->isEnabled()) {
            return newChannel;
        }
    }

    return startChannel;
}

void ChannelsHome::loadEnergyData()
{
    QFile file(EnergyFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    for (const QString &chKey : root.keys()) {
        int channel = chKey.toInt();
        QJsonObject chObj = root[chKey].toObject();

        for (const QString &phase : chObj.keys()) {
            QJsonObject phObj = chObj[phase].toObject();

            EnergyData data;
            data.activePositive = phObj["activePositive"].toDouble();
            data.activeNegative = phObj["activeNegative"].toDouble();
            data.reactivePositive = phObj["reactivePositive"].toDouble();
            data.reactiveNegative = phObj["reactiveNegative"].toDouble();

            accumulatedEnergy[channel][phase] = data;
        }
    }
}

void ChannelsHome::saveEnergyData()
{
    QJsonObject root;

    for (auto chIt = accumulatedEnergy.begin(); chIt != accumulatedEnergy.end(); ++chIt) {
        QJsonObject chObj;
        for (auto phIt = chIt.value().begin(); phIt != chIt.value().end(); ++phIt) {
            QJsonObject phObj;
            const EnergyData &data = phIt.value();
            phObj["activePositive"] = data.activePositive;
            phObj["activeNegative"] = data.activeNegative;
            phObj["reactivePositive"] = data.reactivePositive;
            phObj["reactiveNegative"] = data.reactiveNegative;
            chObj[phIt.key()] = phObj;
        }
        root[QString::number(chIt.key())] = chObj;
    }

    QFile file(EnergyFilePath);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(root).toJson());
}


void ChannelsHome::on_tempBtn_clicked(bool updateTrue, double val)
{
    QString text = ui->tempBtn->text(); // e.g., "30.73 °C"

    // Split number and unit
    QStringList parts = text.split(' '); // ["30.73", "°C"]
    if (parts.size() != 2)
        return; // unexpected format

    bool ok;
    double value = parts[0].toDouble(&ok);
    if (!ok)
        return; // parsing failed

    QString unit = parts[1];

    if(updateTrue == false)
    {
        if (unit == "°C") {
            value = value * 9.0/5.0 + 32;
            unit = "°F";
        } else if (unit == "°F") {
            value = (value - 32) * 5.0/9.0;
            unit = "°C";
        } else {
            return; // unknown unit
        }
    }
    else
    {
        if (unit == "°C") {
            value = val;
        } else if (unit == "°F") {
            value = val * 9.0/5.0 + 32;
        }
    }
    ui->tempBtn->setText(QString::number(value, 'f', 2) + " " + unit);
}


void ChannelsHome::on_resetEnergy_Btn_clicked()
{
    ui->resetEnergy_Btn->clearFocus();
    QString filePath = EnergyFilePath;

    if (QFile::exists(filePath)) {
        if (QFile::remove(filePath)) {
            Toast::show("Energy Reset successfully.", this, Toast::Info);
        } else {
            Toast::show("Unable to reset energy data", this, Toast::Error);
        }
    } else {
        Toast::show("No Energy Data available to reset.", this, Toast::Warning);
    }
}

