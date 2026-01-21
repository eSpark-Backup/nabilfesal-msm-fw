#include "registration.h"
#include "ui_registration.h"
#include <QDir>
#include <QFile>
#include <QStyle>
#include <QDebug>
#include "utils.h"
#include "toast.h"
#include <QJsonArray>
#include <QToolButton>
#include <QJsonObject>
#include <QJsonDocument>
#include "encryptionhelper.h"

Registration::Registration(QString hardwareVersion, QString softwareVersion, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Registration)
{
    ui->setupUi(this);
    ui->hf->setText("HW: " + hardwareVersion + "  FW: " + softwareVersion);

    QToolButton *toggleButton = new QToolButton(ui->admin_pass);
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

    // Set button size and position inside the line edit
    int frameWidth = ui->admin_pass->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = QSize(60, 60);
    toggleButton->setFixedSize(buttonSize);
    toggleButton->move(ui->admin_pass->rect().right() - buttonSize.width() - frameWidth,
                       (ui->admin_pass->rect().height() - buttonSize.height()) / 2);

    // Ensure button stays in place when resized
    QMargins margins = ui->admin_pass->textMargins();
    ui->admin_pass->setTextMargins(margins.left(), margins.top(), buttonSize.width() + frameWidth, margins.bottom());

    connect(toggleButton, &QToolButton::clicked, [=]() mutable {
        bool isPasswordHidden = (ui->admin_pass->echoMode() == QLineEdit::Password);
        ui->admin_pass->setEchoMode(isPasswordHidden ? QLineEdit::Normal : QLineEdit::Password);
        toggleButton->setIcon(QIcon(isPasswordHidden ? ":/icons/hidden.png" : ":/icons/eye.png"));
    });

    QToolButton *toggleButton2 = new QToolButton(ui->confirm_pass);
    toggleButton2->setCursor(Qt::ArrowCursor);
    toggleButton2->setIcon(QIcon(":/icons/eye.png"));
    toggleButton2->setIconSize(QSize(24, 24));
    toggleButton2->setStyleSheet(R"(
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
    toggleButton2->setFocusPolicy(Qt::StrongFocus);

    // Set button size and position inside the line edit
    int frameWidth2 = ui->confirm_pass->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize2 = QSize(60, 60);
    toggleButton2->setFixedSize(buttonSize2);
    toggleButton2->move(ui->confirm_pass->rect().right() - buttonSize2.width() - frameWidth2,
                       (ui->confirm_pass->rect().height() - buttonSize2.height()) / 2);

    // Ensure button stays in place when resized
    QMargins margins2 = ui->confirm_pass->textMargins();
    ui->confirm_pass->setTextMargins(margins2.left(), margins2.top(), buttonSize2.width() + frameWidth, margins2.bottom());

    connect(toggleButton2, &QToolButton::clicked, [=]() mutable {
        bool isPasswordHidden = (ui->confirm_pass->echoMode() == QLineEdit::Password);
        ui->confirm_pass->setEchoMode(isPasswordHidden ? QLineEdit::Normal : QLineEdit::Password);
        toggleButton2->setIcon(QIcon(isPasswordHidden ? ":/icons/hidden.png" : ":/icons/eye.png"));
    });

    keyboard = new MSMKeyboard(this);
    keyboard->move(20, 120);
    keyboard->hide();

    QList<QLineEdit*> edits = {
        ui->uid, ui->admin_pass, ui->confirm_pass
    };

    for (QLineEdit *edit : edits) {
        edit->installEventFilter(this);

        connect(edit, &QLineEdit::editingFinished, this, [=]() {
            keyboard->hide();
            edit->clearFocus();
        });
    }
}

Registration::~Registration()
{
    delete ui;
}

bool Registration::eventFilter(QObject *obj, QEvent *event)
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

void Registration::on_pushButton_clicked()
{
    uid = ui->uid->text().trimmed();
    pass = ui->admin_pass->text().trimmed();
    QString confirm = ui->confirm_pass->text().trimmed();

    if (uid.isEmpty() || pass.isEmpty() || confirm.isEmpty()) {
        Toast::show("All fields are required!", this, Toast::Warning);
        return;
    }
    
    if (pass != confirm) {
        Toast::show("Passwords do not match!", this, Toast::Warning);
        return;
    }

    if (!httpsHandler)
    {
        httpsHandler = new HttpsManager(this);

    connect(httpsHandler, &HttpsManager::requestFinished, this, [this](const QJsonObject &response, int httpCode) {

        QFile file(PROJECT_JSON_PATH);

         if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open project.json for reading.";
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError parseError;
        QByteArray decryptedData = data;
        if (EncryptionHelper::isEncrypted(file))
            decryptedData = EncryptionHelper::readAndDecryptFile(PROJECT_JSON_PATH, Utils::getEncryptionKey());
        QJsonDocument doc = QJsonDocument::fromJson(decryptedData, &parseError);
        if (doc.isNull() || !doc.isObject()) {
            qDebug() << "Invalid JSON format: " << parseError.errorString();
            return;
        }

        QJsonObject projectSettings = doc.object();
        projectSettings["uid"] = uid;
        projectSettings["admin"] = pass;

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qDebug() << "Failed to open project.json for writing.";
            return;
        }

        QJsonDocument newDoc(projectSettings);
        file.write(newDoc.toJson(QJsonDocument::Compact));
        file.close();

        Toast::show("Registration successful!", this, Toast::Info);
        emit requestResp(response, httpCode);
        emit registrationComplete();
        this->close();
        
        }, Qt::UniqueConnection);
    }

    httpsHandler->connect_https(this, uid, pass);
}

