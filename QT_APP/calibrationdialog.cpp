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
    keyboard = new NumKeyboard(this);
    keyboard->move(100, 100);
    keyboard->hide();

    ui->phase_A_input->installEventFilter(this);
    ui->phase_B_input->installEventFilter(this);
    ui->phase_C_input->installEventFilter(this);

    QRegularExpression rx(R"(^-?\d+(\.\d+)?$)");
    QValidator *validator = new QRegularExpressionValidator(rx, this);
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
            keyboard->setTarget(lineEdit);
            Utils::keySound();
            keyboard->show();
            lineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                keyboard->hide();
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
        gain["ID"] = ui->comboBox->currentIndex() + 1;
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
   ui->comboBox->setEnabled(isEnable);
   isCurrentData = isEnable;
}

void CalibrationDialog::parse_and_save_response(const QJsonObject &response)
{
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

    // Voltage: save directly
    if (command == "v-calibration") {
        QJsonObject modifiedResponse = response;
        modifiedResponse["command"] = "reboot-calibration";

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QJsonDocument(modifiedResponse).toJson(QJsonDocument::Indented));
            Toast::show("Voltage Calibrated successfully", this, Toast::Info);
        }
        return;
    }
    // Current: load + merge by ID
    QJsonArray channels;
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        auto doc = QJsonDocument::fromJson(file.readAll(), &err);
        if (err.error == QJsonParseError::NoError)
            channels = doc["current"].toObject()["channels"].toArray();
        file.close();
    }

    QJsonObject newCh = response["current"].toObject();
    int id = newCh["ID"].toInt();
    bool found = false;

    // ✅ iterate by index to safely replace entries
    for (int i = 0; i < channels.size(); ++i) {
        QJsonObject ch = channels[i].toObject();
        if (ch["ID"].toInt() == id) {
            channels[i] = newCh;  // replace safely
            found = true;
            break;
        }
    }
    if (!found)
        channels.append(newCh);

    QJsonObject finalObj{
        {"command", "reboot-calibration"},
        {"current", QJsonObject{{"channels", channels}}}
    };

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(finalObj).toJson(QJsonDocument::Indented));
        Toast::show("Current Calibrated successfully",  this, Toast::Info);
    }
}
