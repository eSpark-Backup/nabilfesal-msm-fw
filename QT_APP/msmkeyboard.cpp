#include "msmkeyboard.h"
#include "ui_msmkeyboard.h"
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

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

    characterButtons = {
        ui->ch1, ui->ch2, ui->ch3, ui->ch4, ui->ch5, ui->ch6
    };

    for (QPushButton *button : numberButtons) {
        connect(button, &QPushButton::clicked, this, &MSMKeyboard::numberClicked);
    }

    for (QPushButton *button : letterButtons) {
        connect(button, &QPushButton::clicked, this, &MSMKeyboard::letterClicked);
    }

    for (QPushButton *button : characterButtons) {
        connect(button, &QPushButton::clicked, this, &MSMKeyboard::letterClicked);
    }

    // Sync key_input <-> targetLineEdit
    connect(ui->key_input, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (targetLineEdit) {
            targetLineEdit->blockSignals(true);
            targetLineEdit->setText(text);
            targetLineEdit->setCursorPosition(ui->key_input->cursorPosition());
            targetLineEdit->blockSignals(false);
        }
    });

    connect(ui->key_input, &QLineEdit::cursorPositionChanged, this, [this](int, int) {
        if (targetLineEdit) {
            targetLineEdit->setCursorPosition(ui->key_input->cursorPosition());
        }
    });

    setAttribute(Qt::WA_TranslucentBackground);
}

MSMKeyboard::~MSMKeyboard()
{
    delete ui;
}

void MSMKeyboard::setTarget(QLineEdit *lineEdit)
{
    targetLineEdit = lineEdit;

    if (targetLineEdit) {
        ui->key_input->setText(targetLineEdit->text());
        ui->key_input->setEchoMode(targetLineEdit->echoMode());
        ui->key_input->setCursorPosition(targetLineEdit->cursorPosition());

        // Keep target in sync if user edits it directly
        connect(targetLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
            ui->key_input->blockSignals(true);
            ui->key_input->setText(text);
            ui->key_input->setCursorPosition(targetLineEdit->cursorPosition());
            ui->key_input->blockSignals(false);
        });
    } else {
        ui->key_input->clear();
    }
}

QLineEdit* MSMKeyboard::getKeyInput() const
{
    return ui->key_input;
}

void MSMKeyboard::numberClicked()
{
    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();
    if (text == "&&") text = "&";

    ui->key_input->insert(text);
    showPopup(clickedButton, text);
}

void MSMKeyboard::letterClicked()
{
    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();
    ui->key_input->insert(text);

    showPopup(clickedButton, text);
}

void MSMKeyboard::on_backspace_clicked()
{
    int pos = ui->key_input->cursorPosition();
    QString text = ui->key_input->text();
    if (pos > 0) {
        text.remove(pos - 1, 1);
        ui->key_input->setText(text);
        ui->key_input->setCursorPosition(pos - 1);
    }
}

void MSMKeyboard::on_enter_clicked()
{
    if (targetLineEdit) emit targetLineEdit->editingFinished();
}

void MSMKeyboard::on_enter_2_clicked()
{
    ui->key_input->insert(" ");
}

void MSMKeyboard::on_shift_clicked()
{
    shiftEnabled = !shiftEnabled;

    for (QPushButton *button : letterButtons) {
        QString currentText = button->text();
        button->setText(shiftEnabled ? currentText.toUpper() : currentText.toLower());
    }

    QList<QString> symbols = {"!", "@", "#", "$", "%", "^", "&&", "*", "(", ")"};
    int i = 0;
    for (QPushButton *button : numberButtons) {
        button->setText(shiftEnabled ? symbols[i] : QString::number(i));
        i++;
    }

    QList<QString> shiftSymbols = {"<", ">", "_", "?", ":", "\\"};
    QList<QString> normalSymbols = {",", ".", "-", "/", ";", "'"};
    int index = 0;
    for (QPushButton *button : characterButtons) {
        button->setText(shiftEnabled ? shiftSymbols[index] : normalSymbols[index]);
        index++;
    }
}

void MSMKeyboard::on_hide_clicked()
{
    if (targetLineEdit) emit targetLineEdit->editingFinished();
}

void MSMKeyboard::showPopup(QPushButton *button, const QString &text)
{
    QLabel *popup = new QLabel(this);
    popup->setText(text);
    popup->setAlignment(Qt::AlignCenter);

    QFont font = button->font();
    font.setPointSize(font.pointSize() + 12);
    font.setBold(true);
    popup->setFont(font);

    QPoint buttonPos = button->mapTo(this, QPoint(0,0));
    popup->setGeometry(buttonPos.x(), buttonPos.y() - button->height(),
                       button->width(), button->height());

    popup->setStyleSheet("background: white; border: 1px solid black; border-radius: 4px;");
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->show();

    QTimer::singleShot(300, popup, &QLabel::close);
}
