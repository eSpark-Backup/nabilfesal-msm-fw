#include "calibrationdialog.h"
#include "ui_calibrationdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include <QRegularExpressionValidator>
#include "measurements.h"
#include <QDir>
#include "toast.h"

#define V_OFFSET_RESPONSE_PATH "config/v_offset_response.json"
#define I_OFFSET_RESPONSE_PATH "config/i_offset_response.json"


CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);
    setupTouchSpin();
    keyboard = new NumKeyboard(this);
    keyboard->move(100, 100);
    keyboard->hide();

    ui->phase_A_input->installEventFilter(this);
    ui->phase_B_input->installEventFilter(this);
    ui->phase_C_input->installEventFilter(this);

    QDoubleValidator *validator = new QDoubleValidator(0.00, 2000.00, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);
    ui->phase_A_input->setValidator(validator);
    ui->phase_B_input->setValidator(validator);
    ui->phase_C_input->setValidator(validator);

    connect(ui->phase_A_input, &QLineEdit::editingFinished, this, [=]() {
        ui->phase_A_input->clearFocus();
        keyboard->hide();
    });

    connect(ui->phase_B_input, &QLineEdit::editingFinished, this, [=]() {
        ui->phase_B_input->clearFocus();
        keyboard->hide();
    });

    connect(ui->phase_C_input, &QLineEdit::editingFinished, this, [=]() {
        ui->phase_C_input->clearFocus();
        keyboard->hide();
    });

    this->installEventFilter(this);
}

CalibrationDialog::~CalibrationDialog()
{
    delete ui;
}

bool CalibrationDialog::eventFilter(QObject *obj, QEvent *event)
{
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
    if (lineEdit) {
        if (event->type() == QEvent::FocusIn) {
            if (lineEdit->text() == "0.0")
                lineEdit->clear();
            keyboard->setTarget(lineEdit);
            Utils::keySound();
            keyboard->show();
            lineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                keyboard->hide();
            if (lineEdit->text().isEmpty())
                lineEdit->setText("0.0");
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void CalibrationDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ui->backPushBtn->setFocusPolicy(Qt::NoFocus);
    setFocus(Qt::OtherFocusReason);
    if (keyboard && keyboard->isVisible()) {
        keyboard->hide();
    }

    ui->phase_A_input->setText("0.0");
    ui->phase_B_input->setText("0.0");
    ui->phase_C_input->setText("0.0");
}


void CalibrationDialog::setupTouchSpin()
{
    ui->valueDisplay->setText(QString::number(value));

    connect(ui->plusBtn, &QPushButton::clicked, this, [=]() {
        value = (value >= maxValue) ? minValue : value + 1;
        ui->valueDisplay->setText(QString::number(value));
    });

    connect(ui->minusBtn, &QPushButton::clicked, this, [=]() {
        value = (value <= minValue) ? maxValue : value - 1;
        ui->valueDisplay->setText(QString::number(value));
    });
}


void CalibrationDialog::on_sendpushBtn_clicked()
{
    auto phaseA = ui->phase_A_input->text().toDouble();
    auto phaseB = ui->phase_B_input->text().toDouble();
    auto phaseC = ui->phase_C_input->text().toDouble();

    QJsonObject gain;
    gain["phase_a"] = phaseA;
    gain["phase_b"] = phaseB;
    gain["phase_c"] = phaseC;

    QJsonObject rootObj;
    if(isCurrentData)
    {
        gain["ID"] = ui->valueDisplay->text().toInt();
        rootObj["command"] = "i-calibration";
        rootObj["current"] = gain;
    }
    else
    {
        rootObj["command"] = "v-calibration";
        rootObj["voltage"] = gain;
    }

    QString jsonString = QString::fromUtf8(QJsonDocument(rootObj).toJson(QJsonDocument::Compact));
    qDebug().noquote() << "Calibration JSON:" << jsonString;
    emit requestSerialCommand(jsonString);
    ui->sendpushBtn->setEnabled(false);
    ui->backPushBtn->setEnabled(false);
}


void CalibrationDialog::on_backPushBtn_clicked()
{
    emit exitScreen();
}


void CalibrationDialog::enableButton()
{
    ui->sendpushBtn->setEnabled(true);
    ui->backPushBtn->setEnabled(true);
}


void CalibrationDialog::loadCalibrationScreen(bool isEnable)
{
   ui->touchSpinWidget->setVisible(isEnable);
   ui->channelLabel->setText(isEnable ? "Channel":"For All Channels");
   QString label = QString("Perform %1 Calibration").arg(isEnable ? "Current" : "Voltage");
   ui->mainLabel->setText(label);
   isCurrentData = isEnable;
}

void CalibrationDialog::parse_and_save_response(const QJsonObject &response)
{
    if(response.contain("error"))
    {
        QString err = obj.value("error").toString();
        Toast::show(err, this, Toast::Error);
        return;
    }

    QDir().mkpath("config");
    const QString command = response["command"].toString();

    QString filePath = (command == "v-calibration")
        ? V_OFFSET_RESPONSE_PATH
        : I_OFFSET_RESPONSE_PATH;

    QFile file(filePath);

    if (command == "ct-ratio") {
        Toast::show("CT-Ratio Updated Successfully", this, Toast::Info);
        return;
    }

    else if (command == "v_offset") {
        Toast::show("V-Offset Updated Successfully", this, Toast::Info);
        return;
    }

    else if (command == "i_offset") {
        Toast::show("I-Offset Updated Successfully", this, Toast::Info);
        return;
    }

    // Voltage: save directly
    else if (command == "v-calibration") {
        QJsonObject modifiedResponse = response;
        modifiedResponse["command"] = "reboot-calibration";

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(modifiedResponse).toJson(QJsonDocument::Indented));
            Toast::show("Voltage Calibrated successfully", this, Toast::Info);
        }
        return;
    }
    
    else if (command == "i-calibration") {
        QJsonArray channels;

        // 1 Load existing channels if file exists
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            if (err.error == QJsonParseError::NoError) {
                QJsonObject currentObj = doc.object()["current"].toObject();
                if (currentObj.contains("channels") && currentObj["channels"].isArray()) {
                    channels = currentObj["channels"].toArray();
                }
            }
            file.close();
        }

        // 2 Extract the new current object from response
        QJsonObject newCurrent = response["current"].toObject();
        int id = newCurrent["ID"].toInt();
        bool found = false;

        // 3 Merge or append
        for (int i = 0; i < channels.size(); ++i) {
            QJsonObject ch = channels[i].toObject();
            if (ch["ID"].toInt() == id) {
                channels[i] = newCurrent;  // replace existing
                found = true;
                break;
            }
        }
        if (!found) {
            channels.append(newCurrent);  // append new ID
        }

        // 4 Save back to file with channels array
        QJsonObject finalObj{
            {"command", "reboot-calibration"},
            {"current", QJsonObject{{"channels", channels}}}
        };

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(finalObj).toJson(QJsonDocument::Indented));
            file.close();
            Toast::show("Current Calibrated successfully", this, Toast::Info);
        }
    }
}
