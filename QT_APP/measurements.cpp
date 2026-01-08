#include <QDir>
#include <QFile>
#include <QDebug>
#include "toast.h"
#include "utils.h"
#include <QKeyEvent>
#include <QJsonArray>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include "factdialog.h"
#include "measurements.h"
#include "ui_measurements.h"
#include "encryptionhelper.h"


#define CT_RATIO    "config/ct_ratio_settings.json"
#define I_OFFSET    "config/i_offset_settings.json"
#define V_OFFSET    "config/v_offset_settings.json"


Measurements::Measurements(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Measurements)
{
    ui->setupUi(this);
    ui->header_label->setText("CT RATIO SETTINGS");
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

    validator = new QDoubleValidator(0.00, 2000.00, 2, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::C);

    for (QLineEdit *edit : edits) {
        edit->setValidator(validator);
        edit->installEventFilter(this);
        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            edit->clearFocus();
            keyboard->hide();
        });
    }
    this->installEventFilter(this);

    on_load_btn_clicked(true);
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
            if (lineEdit->text() == "0.00")
            lineEdit->clear();
            keyboard->setTarget(lineEdit);
            Utils::keySound();
            keyboard->show();
            lineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            QString text = lineEdit->text();
            if (text.isEmpty()) {
                lineEdit->setText("0.00");
            } else {
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    lineEdit->setText(QString::number(value, 'f', 1)); // 1 decimal
                } else {
                    lineEdit->setText("0.00");
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void Measurements::on_back_1_clicked()
{
    emit measureExit();
}


void Measurements::on_load_btn_clicked(bool checked)
{
    ui->back_1->setFocusPolicy(Qt::NoFocus);
    setFocus(Qt::OtherFocusReason);
    if (isRatioActivated)
    {
        for (QLabel *label : signEdits) {
            label->setText("%");
        }
        QFile file(CT_RATIO);
        EncryptionHelper::check_and_encrypt_files(CT_RATIO, "CT_RATIO", checked, this);
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0.00");
                if (!checked) Toast::show("CT_Ratio file not found", this, Toast::Warning);
            return;
        }

        QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(CT_RATIO, Utils::getEncryptionKey());
        if (decryptedData.isEmpty()) {
            if (!checked) Toast::show("Failed to decrypt CT_RATIO settings (wrong password or corrupted file)", this, Toast::Error);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
        if (doc.isNull() || !doc.isObject()) {
            QMessageBox::critical(this, "CT-Ratio Settings", "Invalid JSON format in CT Ratio settings file.");
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
        QFile file(I_OFFSET);
        EncryptionHelper::check_and_encrypt_files(I_OFFSET, "I_OFFSET", checked, this);
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0.00");
                if (!checked) Toast::show("I-OFFSET file not found", this, Toast::Warning);
            return;
        }

        QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(I_OFFSET, Utils::getEncryptionKey());
        if (decryptedData.isEmpty()) {
            if (!checked) Toast::show("Failed to decrypt I_OFFSET settings (wrong password or corrupted file)", this, Toast::Error);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
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
        QFile file(V_OFFSET);
        EncryptionHelper::check_and_encrypt_files(V_OFFSET, "V_OFFSET", checked, this);
        if (!file.open(QIODevice::ReadOnly)) {
            for (QLineEdit *edit : edits)
                edit->setText("0.00");
                if (!checked) Toast::show("V-OFFSET file not found", this, Toast::Warning);
            return;
        }

        QByteArray decryptedData = EncryptionHelper::readAndDecryptFile(V_OFFSET, Utils::getEncryptionKey());
        if (decryptedData.isEmpty()) {
            if (!checked) Toast::show("Failed to decrypt V_OFFSET settings (wrong password or corrupted file)", this, Toast::Error);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
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
    else
        return;
        
    if(!checked)
    {
        QString loadFile = isRatioActivated ? "CT-Ratio" : isIOffsetActivated ? "I-Offset" : "V-Offset";
        Toast::show(QString("[%1] loaded successfully.").arg(loadFile), this, Toast::Info);
    }
}


bool Measurements::areSettingsUnchanged(const QJsonObject &current, const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists())
        return false;  // If no file, settings must be new

    if (!file.open(QIODevice::ReadOnly))
        return false;  // Cannot read → assume changed

    QByteArray bytes = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;  // Corrupt → treat as changed

    QJsonObject existing = doc.object();

    // Compare deeply
    return (QJsonDocument(current).toJson(QJsonDocument::Compact) ==
            QJsonDocument(existing).toJson(QJsonDocument::Compact));
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

    auto parseDoubleSafe = [](const QString &str){
        bool ok; double v = str.toDouble(&ok); return ok ? v : 0.0;
    };

    QJsonObject channelJson;
    for (int i = 0; i < channels.size(); ++i)
        channelJson[QString("channel-%1").arg(i + 1)] = parseDoubleSafe(channels[i]);

    if (isRatioActivated)
        processSettings("ct_ratio", channelJson, "set-ct-ratio", false, true);

    else if (isIOffsetActivated)
        processSettings("i_offset", channelJson, "set-i-offset", false, true);

    else if (isVOffsetActivated)
        processSettings("v_offset", channelJson, "set-v-offset", false, true);
}


void Measurements::processSettings(const QString &key,
                                   const QJsonObject &settingsObj,
                                   const QString &command,
                                   bool updateUI,
                                   bool emitDeviceCommand)
{
    QString filePath = QString("config/%1_settings.json").arg(key);

    QString title = key;
    title.replace("_", " ");
    title[0] = title[0].toUpper();

    // Build final nested object
    QJsonObject data;
    data[key] = settingsObj;
    if (!command.isEmpty())
        data["command"] = command;

    // // Compare with existing file
    // if (areSettingsUnchanged(data, filePath))
    // {
    //     Toast::show(QString("[%1] Settings unchanged → not saving.").arg(title),
    //                 this, Toast::Warning);

        // Even on unchanged, UI might still need updating (load case)
        if (updateUI)
        {
            for (int i = 1; i <= 12; ++i)
            {
                if (auto edit = findChild<QLineEdit*>(QString("ch%1").arg(i)))
                    edit->setText(QString::number(settingsObj[QString("channel-%1").arg(i)].toDouble(), 'f', 2));
            }
        }
    //     return;
    // }

    // Save to file
    QDir dir(CONFIG_ADMIN_PATH);
    if (!dir.exists()) dir.mkpath(".");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        Toast::show(QString("[%1] Failed to save settings.").arg(title),
                    this, Toast::Error);
        return;
    }

    QJsonDocument doc(data);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    QByteArray encryptedData = EncryptionHelper::encrypt(jsonData, Utils::getEncryptionKey());
    file.write(encryptedData);
    file.close();

    Toast::show(QString("[%1] Settings saved to file.").arg(title),
                this, Toast::Info);

    // Update UI if requested
    if (updateUI)
    {
        for (int i = 1; i <= 12; ++i)
        {
            if (auto edit = findChild<QLineEdit*>(QString("ch%1").arg(i)))
                edit->setText(QString::number(settingsObj[QString("channel-%1").arg(i)].toDouble(), 'f', 2));
        }
    }
    enableButton(false);
    emit requestSerialCommand(QJsonDocument(data).toJson(QJsonDocument::Compact));

    if (emitDeviceCommand)
    {
        if (key == "ct_ratio") emit ct_ratio_changed(data);
        if (key == "i_offset") emit i_offset_changed(data);
        if (key == "v_offset") emit v_offset_changed(data);
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


void Measurements::ration_btn_clicked()
{
    ui->header_label->setText("CT RATIO SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    validator->setBottom(0.00);
    validator->setTop(2000.00);
    // ui->ration_btn->setFocus();
    isRatioActivated = true;
    isVOffsetActivated = false;
    isIOffsetActivated = false;
    on_load_btn_clicked(true);
}


void Measurements::i_off_btn_clicked()
{
    ui->header_label->setText("I-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    validator->setBottom(-2000.00);
    validator->setTop(2000.00);
    isRatioActivated = false;
    isVOffsetActivated = false;
    isIOffsetActivated = true;
    on_load_btn_clicked(true);
}


void Measurements::v_off_btn_clicked()
{
    ui->header_label->setText("V-OFFSET SETTINGS");
    ui->verticalStackedWidget->setCurrentIndex(0);
    validator->setBottom(-2000.00);
    validator->setTop(2000.00);
    isRatioActivated = false;
    isVOffsetActivated = true;
    isIOffsetActivated = false;
    on_load_btn_clicked(true);
}


void Measurements::parse_load_store_generic(const QJsonObject &payload, const QString &key)
{
    QJsonArray dataArray = payload["data"].toArray();
    if (dataArray.isEmpty()) return;

    QJsonObject inner = dataArray.first().toObject();
    QJsonObject settingsObj = inner[key].toObject();
    QString command = inner["command"].toString();
    if (settingsObj.isEmpty() || command.isEmpty()) return;

    processSettings(key, settingsObj, command, true, false);
}


void Measurements::parse_load_store_ctRatio(const QJsonObject &payload)
{
    parse_load_store_generic(payload, "ct_ratio");
}

void Measurements::parse_load_store_vOffset(const QJsonObject &payload)
{
    parse_load_store_generic(payload, "v_offset");
}

void Measurements::parse_load_store_iOffset(const QJsonObject &payload)
{
    parse_load_store_generic(payload, "i_offset");
}


void Measurements::send_measurement_data()
{
    QJsonObject measurement_json = Utils::readJsonFile(CT_RATIO);
    if (!measurement_json.isEmpty())
        emit ct_ratio_changed(measurement_json);
    
    measurement_json = Utils::readJsonFile(V_OFFSET);
    if (!measurement_json.isEmpty())
        emit v_offset_changed(measurement_json);

    measurement_json = Utils::readJsonFile(I_OFFSET);
    if (!measurement_json.isEmpty())
        emit i_offset_changed(measurement_json);
}

void Measurements::enableButton(bool state)
{
    ui->back_1->setEnabled(state);
    ui->save_btn->setEnabled(state);
    ui->load_btn->setEnabled(state);
}
