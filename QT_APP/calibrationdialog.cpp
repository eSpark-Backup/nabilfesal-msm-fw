#include "calibrationdialog.h"
#include "ui_calibrationdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include <QRegularExpressionValidator>
#include "measurements.h"

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
}


void CalibrationDialog::on_backPushBtn_clicked()
{
    emit exitScreen();
}


void CalibrationDialog::loadCalibrationScreen(bool isEnable)
{
   ui->comboBox->setEnabled(isEnable);
   isCurrentData = isEnable;
}
