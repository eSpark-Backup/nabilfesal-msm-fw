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
    setAttribute(Qt::WA_TranslucentBackground);
}

void NumKeyboard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPos = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void NumKeyboard::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragStartPos);
        event->accept();
    }
}

NumKeyboard::~NumKeyboard()
{
    delete ui;
}

void NumKeyboard::setTarget(QLineEdit *lineEdit)
{
    targetLineEdit = lineEdit;
}

void NumKeyboard::numberClicked()
{
    if (!targetLineEdit) return;

    QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
    if (!clickedButton) return;

    QString text = clickedButton->text();
    targetLineEdit->insert(text);

}

void NumKeyboard::on_enter_clicked()
{
    emit targetLineEdit->editingFinished();
}

void NumKeyboard::on_back_clicked()
{
    QString text = targetLineEdit->text();
    text.chop(1);
    targetLineEdit->setText(text);
}

