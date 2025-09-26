#include "login.h"
#include "ui_login.h"
#include <QMessageBox>
#include <QDebug>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QStyle>
#include "utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFile>

LoginWidget::LoginWidget(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    QToolButton *toggleButton = new QToolButton(ui->passwordLineEdit);
    toggleButton->setCursor(Qt::ArrowCursor);
    toggleButton->setIcon(QIcon(":/icons/eye.png"));
    toggleButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    toggleButton->setIconSize(QSize(50, 50));
    toggleButton->setFocusPolicy(Qt::StrongFocus);

    int frameWidth = ui->passwordLineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = QSize(30, 30);
    toggleButton->setFixedSize(buttonSize);
    toggleButton->move(ui->passwordLineEdit->rect().right() - buttonSize.width() - frameWidth,
                       (ui->passwordLineEdit->rect().height() - buttonSize.height()) / 2);

    QMargins margins = ui->passwordLineEdit->textMargins();
    ui->passwordLineEdit->setTextMargins(margins.left(), margins.top(),
                                         buttonSize.width() + frameWidth, margins.bottom());

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 120);
    keyboard->hide();

    connect(toggleButton, &QToolButton::clicked, [=]() mutable {
        bool isPasswordHidden = (ui->passwordLineEdit->echoMode() == QLineEdit::Password);
        QLineEdit::EchoMode newMode = isPasswordHidden ? QLineEdit::Normal : QLineEdit::Password;

        // Apply to actual line edit
        ui->passwordLineEdit->setEchoMode(newMode);

        // Apply to keyboard’s proxy line edit
        keyboard->getKeyInput()->setEchoMode(newMode);

        // Update icon
        toggleButton->setIcon(QIcon(isPasswordHidden ? ":/icons/hidden.png" : ":/icons/eye.png"));
    });

    connect(ui->passwordLineEdit, &QLineEdit::editingFinished, this, [=]() {
        ui->passwordLineEdit->clearFocus();
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
            keyboard->setTarget(ui->passwordLineEdit);
            Utils::keySound();
            keyboard->show();
            ui->passwordLineEdit->setFocus();
        }
        else if (event->type() == QEvent::FocusOut) {
            if (!keyboard->isAncestorOf(QApplication::focusWidget())) {
                keyboard->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void LoginWidget::on_loginButton_clicked()
{
    QFile file("config/project.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString adminPass = obj.value("admin").toString();
            QString factoryPass = obj.value("factory").toString();
            auto password = ui->passwordLineEdit->text();
            if (password == adminPass) {
                QMessageBox::information(this, "Login Successful", "You are logged in successfully with Admin Settings!");
                ui->passwordLineEdit->clear();
                ui->pass_frame->move(20, 302);
                emit adminLoginSuccess();
            } else if (password == factoryPass) {
                QMessageBox::information(this, "Login Successful", "You are logged in successfully with Factory Settings!");
                ui->passwordLineEdit->clear();
                ui->pass_frame->move(20, 302);
                emit factoryLoginSuccess();
            }
            else {
                QMessageBox::warning(this, "Login Failed", "Incorrect password. Please try again.");
            }
        }
        file.close();
    } else {
        QMessageBox::critical(this, "Error", "Could not open configuration file.");
    }
}
