#include "numkeyboard.h"
#include "ui_numkeyboard.h"
#include <QMouseEvent>

NumKeyboard::NumKeyboard(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NumKeyboard)
{
    ui->setupUi(this);
    // List of number buttons
    numberButtons = {
        ui->n_, ui->n_1, ui->n_2, ui->n_3, ui->n_4,
        ui->n_5, ui->n_6, ui->n_7, ui->n_8, ui->n_9,
        ui->n_dot, ui->n_minus
    };

    for (QPushButton *button : numberButtons) {
        connect(button, &QPushButton::clicked, this, &NumKeyboard::numberClicked);
    }

     // Sync key_input <-> targetLineEdit
    connect(ui->numpad_input, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (targetLineEdit) {
            targetLineEdit->blockSignals(true);
            targetLineEdit->setText(text);
            targetLineEdit->setCursorPosition(ui->numpad_input->cursorPosition());
            targetLineEdit->blockSignals(false);
        }
    });

    connect(ui->numpad_input, &QLineEdit::cursorPositionChanged, this, [this](int, int) {
        if (targetLineEdit) {
            targetLineEdit->setCursorPosition(ui->numpad_input->cursorPosition());
        }
    });

    setAttribute(Qt::WA_TranslucentBackground);
}

void NumKeyboard::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (parentWidget()) {
        int x = (parentWidget()->width() - width()) / 2;
        int y = parentWidget()->height() - height();
        move(x, y);
    }
}

void NumKeyboard::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);  // do nothing special
}

void NumKeyboard::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);   // do nothing special
}


NumKeyboard::~NumKeyboard()
{
    delete ui;
}


void NumKeyboard::setTarget(QLineEdit *lineEdit)
{
    targetLineEdit = lineEdit;

    if (targetLineEdit) {
        // ① Copy text from target into keyboard preview
        ui->numpad_input->setText(targetLineEdit->text());
        ui->numpad_input->setEchoMode(targetLineEdit->echoMode());
        ui->numpad_input->setCursorPosition(targetLineEdit->cursorPosition());

        // ② Keep preview in sync when target changes externally
        connect(targetLineEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
            ui->numpad_input->blockSignals(true);
            ui->numpad_input->setText(text);
            ui->numpad_input->setCursorPosition(targetLineEdit->cursorPosition());
            ui->numpad_input->blockSignals(false);
        });
    } else {
        ui->numpad_input->clear();
    }
}


void NumKeyboard::numberClicked()
{
    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();
    ui->numpad_input->insert(text);  // now inserts into preview, not directly into the target
}


void NumKeyboard::on_back_clicked()
{
    QString text = ui->numpad_input->text();
    text.chop(1);
    ui->numpad_input->setText(text);
}


void NumKeyboard::on_enter_clicked()
{
    if (targetLineEdit)
        emit targetLineEdit->editingFinished();
}


