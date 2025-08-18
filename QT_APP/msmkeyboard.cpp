#include "msmkeyboard.h"
#include "ui_msmkeyboard.h"

MSMKeyboard::MSMKeyboard(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::MSMKeyboard),
    targetLineEdit(nullptr),
    shiftEnabled(false)
{
    ui->setupUi(this);

    // List of number buttons
    numberButtons = {
        ui->n_, ui->n_1, ui->n_2, ui->n_3, ui->n_4,
        ui->n_5, ui->n_6, ui->n_7, ui->n_8, ui->n_9
    };

    letterButtons = {
        ui->key_0, ui->key_1, ui->key_2, ui->key_3, ui->key_4,
        ui->key_5, ui->key_6, ui->key_7, ui->key_8, ui->key_9,
        ui->key_11, ui->key_12, ui->key_13, ui->key_14, ui->key_15,
        ui->key_16, ui->key_17, ui->key_18, ui->key_19, ui->key_21,
        ui->key_22, ui->key_23, ui->key_24, ui->key_25,
        ui->key_26, ui->key_28

    };

    for (QPushButton *button : numberButtons) {
        connect(button, &QPushButton::clicked, this, &MSMKeyboard::numberClicked);
    }

    for (QPushButton *button : letterButtons) {
        connect(button, &QPushButton::clicked, this, &MSMKeyboard::letterClicked);
    }

    setAttribute(Qt::WA_TranslucentBackground);
}

MSMKeyboard::~MSMKeyboard()
{
    delete ui;
}

void MSMKeyboard::setTarget(QLineEdit *lineEdit)
{
    targetLineEdit = lineEdit;
}

void MSMKeyboard::numberClicked()
{
    if (!targetLineEdit) return;

    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();

    if (text == "&&")
        text = "&";

    targetLineEdit->insert(text);

    if (shiftEnabled)
        on_shift_clicked();
}

void MSMKeyboard::letterClicked()
{
    if (!targetLineEdit) return;

    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();

    targetLineEdit->insert(text);

    if (shiftEnabled)
        on_shift_clicked();
}


void MSMKeyboard::on_backspace_clicked()
{
    QString text = targetLineEdit->text();
    text.chop(1);
    targetLineEdit->setText(text);
}


void MSMKeyboard::on_enter_clicked()
{
    emit targetLineEdit->editingFinished();
}


void MSMKeyboard::on_enter_2_clicked()
{
    targetLineEdit->insert(" ");
}


void MSMKeyboard::on_shift_clicked()
{
    shiftEnabled = !shiftEnabled;

    for (QPushButton *button : letterButtons) {
        QString currentText = button->text();
        button->setText(shiftEnabled ? currentText.toUpper() : currentText.toLower());
    }
    QList<QString> symbols = {".", "!", "@", "#", "$", "%", "&&", "*", "_", "-" };
    int i = 0;
    for (QPushButton *button : numberButtons) {
        button->setText(shiftEnabled ? symbols[i] : QString::number(i));
        i++;
    }
}


void MSMKeyboard::on_hide_clicked()
{
    emit targetLineEdit->editingFinished();
}

