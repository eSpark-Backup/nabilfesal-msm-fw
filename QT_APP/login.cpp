#include "login.h"
#include "ui_login.h"
#include <QMessageBox>
#include <QDebug>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QStyle>
#include "utils.h"

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    QToolButton *toggleButton = new QToolButton(ui->passwordLineEdit);
    toggleButton->setCursor(Qt::ArrowCursor);
    toggleButton->setIcon(QIcon(":/icons/eye.png"));
    toggleButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    toggleButton->setIconSize(QSize(50, 50));

    // Set button size and position inside the line edit
    int frameWidth = ui->passwordLineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = QSize(30, 30);
    toggleButton->setFixedSize(buttonSize);
    toggleButton->move(ui->passwordLineEdit->rect().right() - buttonSize.width() - frameWidth,
                       (ui->passwordLineEdit->rect().height() - buttonSize.height()) / 2);

    // Ensure button stays in place when resized
    QMargins margins = ui->passwordLineEdit->textMargins();
    ui->passwordLineEdit->setTextMargins(margins.left(), margins.top(), buttonSize.width() + frameWidth, margins.bottom());

    connect(toggleButton, &QToolButton::clicked, [=]() mutable {
        Utils::keySound();
        bool isPasswordHidden = (ui->passwordLineEdit->echoMode() == QLineEdit::Password);
        ui->passwordLineEdit->setEchoMode(isPasswordHidden ? QLineEdit::Normal : QLineEdit::Password);
        toggleButton->setIcon(QIcon(isPasswordHidden ? ":/icons/hidden.png" : ":/icons/eye.png"));
    });

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 180);
    keyboard->hide();
    ui->pass_frame->move(20, 145);

    connect(ui->passwordLineEdit, &QLineEdit::textChanged, this, [=]() {
        ui->pass_frame->move(20, 145);
    });

    connect(ui->passwordLineEdit, &QLineEdit::editingFinished, this, [=]() {
        ui->pass_frame->move(20, 302);
        keyboard->hide();
    });

    ui->passwordLineEdit->installEventFilter(this);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

bool LoginWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->passwordLineEdit) {
        if (event->type() == QEvent::FocusIn) {
            ui->pass_frame->move(20, 145);
            keyboard->setTarget(ui->passwordLineEdit);
            Utils::keySound();
            keyboard->show();
            ui->passwordLineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                ui->pass_frame->move(20, 302);
                keyboard->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void LoginWidget::on_loginButton_clicked()
{
    auto password = ui->passwordLineEdit->text();
    if (password == "espark") {
        emit loginSuccess();
    } else {
        QMessageBox::warning(this, "Login Failed", "Incorrect password. Please try again.");
    }
}
