#include "calibrationdialog.h"
#include "ui_calibrationdialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "utils.h"
#include <QRegularExpressionValidator>

CalibrationDialog::CalibrationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CalibrationDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    keyboard = new NumKeyboard(this);
    keyboard->move(100, 100);
    keyboard->hide();

    ui->v_input->installEventFilter(this);
    ui->a_input->installEventFilter(this);

    QRegularExpression rx(R"(^-?\d+(\.\d+)?$)");
    QValidator *validator = new QRegularExpressionValidator(rx, this);
    ui->v_input->setValidator(validator);
    ui->a_input->setValidator(validator);

    connect(ui->v_input, &QLineEdit::editingFinished, this, [=]() {
        ui->v_input->clearFocus();
        keyboard->hide();
    });

    connect(ui->a_input, &QLineEdit::editingFinished, this, [=]() {
        ui->a_input->clearFocus();
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

void CalibrationDialog::on_pushButton_clicked()
{
    auto voltage = ui->v_input->text().toDouble();
    auto current = ui->a_input->text().toDouble();

    QJsonObject gain;
    gain["voltage"] = voltage;
    gain["current"] = current;

    QJsonObject rootObj;
    rootObj["gain"] = gain;
    rootObj["command"] = "set-gain";

    emit requestSerialCommand(QString::fromUtf8(QJsonDocument(rootObj).toJson(QJsonDocument::Compact)));
    this->close();
}

