#include "login.h"
#include "ui_login.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QStyle>
#include "utils.h"
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonObject>
#include <QToolButton>
#include <QMessageBox>
#include <QMessageBox>
#include <QJsonDocument>
#include "encryptionhelper.h"

LoginWidget::LoginWidget(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    QToolButton *toggleButton = new QToolButton(ui->passwordLineEdit);
    toggleButton->setCursor(Qt::ArrowCursor);
    toggleButton->setIcon(QIcon(":/icons/eye.png"));
    toggleButton->setIconSize(QSize(24, 24));
    toggleButton->setStyleSheet(R"(
        QToolButton {
            background: transparent;
            border: none;
            padding: 10px;
        }
        QToolButton:hover {
            background: transparent;
        }
        QToolButton:pressed {
            background: transparent;
        }
        QToolButton:focus {
            background: transparent;
            outline: none;
        }
    )");
    toggleButton->setFocusPolicy(Qt::StrongFocus);

    int frameWidth = ui->passwordLineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = QSize(60, 60);
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

void LoginWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ui->pushButton->setFocusPolicy(Qt::NoFocus);
    setFocus(Qt::OtherFocusReason);
    if (keyboard && keyboard->isVisible()) {
        keyboard->hide();
    }
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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray encryptedData = file.readAll();
        file.close();
        QByteArray decryptedData = EncryptionHelper::decrypt(encryptedData, Utils::getEncryptionKey());
        QJsonDocument doc = QJsonDocument::fromJson(decryptedData);
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

void LoginWidget::on_pushButton_clicked()
{
    emit loadChannels();
}

// void LoginWidget::on_diagBtn_clicked()
// {
//     emit loadDiagnotics();
// }

