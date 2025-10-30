#include "measurements.h"
#include "ui_measurements.h"
#include "factdialog.h"
#include "rebootdialog.h"
#include "utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include "calibrationdialog.h"

Measurements::Measurements(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Measurements)
{
    ui->setupUi(this);
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->ration_btn->setFocus();
    ui->verticalStackedWidget->setCurrentIndex(0);
    isRatioActivated = true;

    keyboard = new NumKeyboard(this);
    keyboard->move(100, 180);
    keyboard->hide();

    edits = {
        ui->ch1, ui->ch2, ui->ch3, ui->ch4, ui->ch5, ui->ch6,
        ui->ch7, ui->ch8, ui->ch9, ui->ch10, ui->ch11, ui->ch12
    };

    signEdits = {
        ui->s1, ui->s2, ui->s3, ui->s4, ui->s5, ui->s6,
        ui->s7, ui->s8, ui->s9, ui->s10, ui->s11, ui->s12
    };

    for (QLabel *label : signEdits) {
        label->setText("%");
    }

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);
        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            edit->clearFocus();
            keyboard->hide();
        });
    }
    this->installEventFilter(this);

    on_load_btn_clicked();
}

Measurements::~Measurements()
{
    delete ui;
}

bool Measurements::eventFilter(QObject *obj, QEvent *event)
{
    // Then: handle QLineEdit focus/keyboard logic
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

void Measurements::on_back_1_clicked()
{
    emit measureExit();
}


void Measurements::on_back_2_clicked()
{
    emit measureExit();
}


void Measurements::on_pushButton_13_clicked()
{
    FactDialog dialog(this);
    dialog.setLabel(std::string("Please make sure to input the correct \n nominal voltage and reboot the system."));
    dialog.exec();
}


void Measurements::on_pushButton_14_clicked()
{
    FactDialog dialog(this);
    dialog.setLabel(std::string("Please make sure to input the correct \n nominal current and reboot the system."));
    dialog.exec();
}


void Measurements::on_reboot_btn_clicked()
{
    RebootDialog reboot(this);
    reboot.exec();
}


void Measurements::on_ration_btn_clicked()
{
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->ration_btn->setFocus();
    isRatioActivated = true;
    isVOffsetActivated = false;
    isIOffsetActivated = false;
    on_load_btn_clicked();
}


void Measurements::on_i_off_btn_clicked()
{
    ui->header_label->setText("I-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->i_off_btn->setFocus();
    isRatioActivated = false;
    isVOffsetActivated = false;
    isIOffsetActivated = true;
    on_load_btn_clicked();
}


void Measurements::on_v_off_btn_clicked()
{
    ui->header_label->setText("V-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    ui->v_off_btn->setFocus();
    isRatioActivated = false;
    isVOffsetActivated = true;
    isIOffsetActivated = false;
    on_load_btn_clicked();
}


void Measurements::on_cal_btn_clicked()
{
    ui->verticalStackedWidget->setCurrentIndex(1);
    ui->cal_btn->setFocus();
}


void Measurements::on_load_btn_clicked()
{
    if (isRatioActivated)
    {
        for (QLabel *label : signEdits) {
            label->setText("%");
        }
        QFile file("config/ct_ratio_settings.json");
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0");
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull() || !doc.isObject()) {
            QMessageBox::critical(this, "CT Ratio Settings", "Invalid JSON format in CT Ratio settings file.");
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("ct_ratio") || !obj["ct_ratio"].isObject()) {
            QMessageBox::critical(this, "CT Ratio Settings", "Missing 'ct_ratio' object in settings file.");
            return;
        }

        QJsonObject ratioSettings = obj["ct_ratio"].toObject();
        for (int i = 1; i <= 12; ++i) {
            QString key = QString("channel-%1").arg(i);
            if (QLineEdit *edit = this->findChild<QLineEdit*>(QString("ch%1").arg(i))) {
                double val = ratioSettings[key].toDouble();
                edit->setText(QString::number(val, 'f', 2));
            }
        }
    }
    else if (isIOffsetActivated)
    {
        for (QLabel *label : signEdits) {
            label->setText("mA");
        }
        QFile file("config/i_offset_settings.json");
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0");
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull() || !doc.isObject()) {
            QMessageBox::critical(this, "I-Offset Settings", "Invalid JSON format in I-Offset settings file.");
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("i_offset") || !obj["i_offset"].isObject()) {
            QMessageBox::critical(this, "I-Offset Settings", "Missing 'i_offset' object in settings file.");
            return;
        }

        QJsonObject iOffsetSettings = obj["i_offset"].toObject();
        for (int i = 1; i <= 12; ++i) {
            QString key = QString("channel-%1").arg(i);
            if (QLineEdit *edit = this->findChild<QLineEdit*>(QString("ch%1").arg(i))) {
                double val = iOffsetSettings[key].toDouble();
                edit->setText(QString::number(val, 'f', 2));
            }
        }
    }
    else if (isVOffsetActivated)
    {
        for (QLabel *label : signEdits) {
            label->setText("V");
        }
        QFile file("config/v_offset_settings.json");
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0");
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull() || !doc.isObject()) {
            QMessageBox::critical(this, "V-Offset Settings", "Invalid JSON format in V-Offset settings file.");
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("v_offset") || !obj["v_offset"].isObject()) {
            QMessageBox::critical(this, "V-Offset Settings", "Missing 'v_offset' object in settings file.");
            return;
        }

        QJsonObject vOffsetSettings = obj["v_offset"].toObject();
        for (int i = 1; i <= 12; ++i) {
            QString key = QString("channel-%1").arg(i);
            if (QLineEdit *edit = this->findChild<QLineEdit*>(QString("ch%1").arg(i))) {
                double val = vOffsetSettings[key].toDouble();
                edit->setText(QString::number(val, 'f', 2));
            }
        }
    }
}


void Measurements::on_save_btn_clicked()
{
    QStringList channels = {
        ui->ch1->text().trimmed(),  ui->ch2->text().trimmed(),
        ui->ch3->text().trimmed(),  ui->ch4->text().trimmed(),
        ui->ch5->text().trimmed(),  ui->ch6->text().trimmed(),
        ui->ch7->text().trimmed(),  ui->ch8->text().trimmed(),
        ui->ch9->text().trimmed(),  ui->ch10->text().trimmed(),
        ui->ch11->text().trimmed(), ui->ch12->text().trimmed()
    };

    auto parseDoubleSafe = [](const QString &str) -> double {
        bool ok = false;
        double val = str.toDouble(&ok);
        return ok ? val : 0.0;
    };

    if (isRatioActivated)
    {
        QJsonObject ratioSettings;
        for (int i = 0; i < channels.size(); ++i) {
            ratioSettings[QString("channel-%1").arg(i + 1)] = parseDoubleSafe(channels[i]);
        }

        QJsonObject data;
        data["ct_ratio"] = ratioSettings;
        data["command"]  = "set-ct-ratio";

        QDir dir("config");
        if (!dir.exists()) dir.mkpath(".");

        QFile file("config/ct_ratio_settings.json");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, "CT Ratio Settings", "Failed to save CT Ratio settings to a file.");
            return;
        }

        QByteArray jsonBytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
        file.write(jsonBytes);
        file.close();

        emit requestSerialCommand(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
    }
    else if (isIOffsetActivated)
    {
        QJsonObject iOffsetSettings;
        for (int i = 0; i < channels.size(); ++i) {
            iOffsetSettings[QString("channel-%1").arg(i + 1)] = parseDoubleSafe(channels[i]);
        }

        QJsonObject data;
        data["i_offset"] = iOffsetSettings;
        data["command"]  = "set-i-offset";

        QDir dir("config");
        if (!dir.exists()) dir.mkpath(".");

        QFile file("config/i_offset_settings.json");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, "I-Offset Settings", "Failed to save I-Offset settings to a file.");
            return;
        }

        QByteArray jsonBytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
        file.write(jsonBytes);
        file.close();

        emit requestSerialCommand(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
    }
    else if (isVOffsetActivated)
    {
        QJsonObject vOffsetSettings;
        for (int i = 0; i < channels.size(); ++i) {
            vOffsetSettings[QString("channel-%1").arg(i + 1)] = parseDoubleSafe(channels[i]);
        }

        QJsonObject data;
        data["v_offset"] = vOffsetSettings;
        data["command"]  = "set-v-offset";

        QDir dir("config");
        if (!dir.exists()) dir.mkpath(".");

        QFile file("config/v_offset_settings.json");
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QMessageBox::critical(this, "V-Offset Settings", "Failed to save V-Offset settings to a file.");
            return;
        }

        QByteArray jsonBytes = QJsonDocument(data).toJson(QJsonDocument::Indented);
        file.write(jsonBytes);
        file.close();

        emit requestSerialCommand(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
    }
}


void Measurements::on_toolButton_clicked()
{
    ui->ch1->setFocus();
    keyboard->setTarget(ui->ch1);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_2_clicked()
{
    ui->ch2->setFocus();
    keyboard->setTarget(ui->ch2);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_3_clicked()
{
    ui->ch3->setFocus();
    keyboard->setTarget(ui->ch3);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_4_clicked()
{
    ui->ch4->setFocus();
    keyboard->setTarget(ui->ch4);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_5_clicked()
{
    ui->ch5->setFocus();
    keyboard->setTarget(ui->ch5);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_6_clicked()
{
    ui->ch6->setFocus();
    keyboard->setTarget(ui->ch6);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_7_clicked()
{
    ui->ch7->setFocus();
    keyboard->setTarget(ui->ch7);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_8_clicked()
{
    ui->ch8->setFocus();
    keyboard->setTarget(ui->ch8);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_9_clicked()
{
    ui->ch9->setFocus();
    keyboard->setTarget(ui->ch9);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_10_clicked()
{
    ui->ch10->setFocus();
    keyboard->setTarget(ui->ch10);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_11_clicked()
{
    ui->ch11->setFocus();
    keyboard->setTarget(ui->ch11);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_toolButton_12_clicked()
{
    ui->ch12->setFocus();
    keyboard->setTarget(ui->ch12);
    Utils::keySound();
    keyboard->show();
}


void Measurements::on_pushButton_clicked()
{
    CalibrationDialog calDialog(this);
    connect(&calDialog, &CalibrationDialog::requestSerialCommand, this, &Measurements::requestSerialCommand);
    calDialog.exec();
    disconnect(&calDialog, &CalibrationDialog::requestSerialCommand, this, &Measurements::requestSerialCommand);
}

