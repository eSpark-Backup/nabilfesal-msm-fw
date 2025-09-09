#include "registration.h"
#include "ui_registration.h"
#include "utils.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QToolButton>
#include <QStyle>

Registration::Registration(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Registration)
{
    ui->setupUi(this);

    QToolButton *toggleButton = new QToolButton(ui->admin_pass);
    toggleButton->setCursor(Qt::ArrowCursor);
    toggleButton->setIcon(QIcon(":/icons/eye.png"));
    toggleButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    toggleButton->setIconSize(QSize(30, 30));
    toggleButton->setFocusPolicy(Qt::StrongFocus);

    // Set button size and position inside the line edit
    int frameWidth = ui->admin_pass->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize = QSize(30, 30);
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
    toggleButton2->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    toggleButton2->setIconSize(QSize(30, 30));
    toggleButton2->setFocusPolicy(Qt::StrongFocus);

    // Set button size and position inside the line edit
    int frameWidth2 = ui->confirm_pass->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    QSize buttonSize2 = QSize(30, 30);
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
    keyboard->move(20, 180);
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

        connect(edit, &QLineEdit::textChanged, this, [=]() {
            QString frameName = getParentFrameName(edit);
            if (frameName == ui->uid_frame->objectName())
            {
                ui->uid_frame->move(9,120);
            }
            else if (frameName == ui->admin_frame->objectName())
            {
                ui->admin_frame->move(9,120);
            }
            else if (frameName == ui->confirm_frame->objectName())
            {
                ui->confirm_frame->move(9,120);
            }
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
                QString frameName = getParentFrameName(lineEdit);
                if (frameName == ui->uid_frame->objectName())
                {
                    ui->uid_frame->move(9,187);
                }
                else if (frameName == ui->admin_frame->objectName())
                {
                    ui->admin_frame->move(9,263);
                }
                else if (frameName == ui->confirm_frame->objectName())
                {
                    ui->confirm_frame->move(9,339);
                }
                keyboard->hide();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

QString Registration::getParentFrameName(QLineEdit *lineEdit)
{
    QWidget *p = lineEdit->parentWidget();
    while (p && !qobject_cast<QFrame*>(p)) {
        p = p->parentWidget();
    }
    QFrame *parentFrame = qobject_cast<QFrame*>(p);
    if (parentFrame) {
        return parentFrame->objectName();  // 👈 frame name from Qt Designer
    }
    return QString(); // empty if no frame found
}

void Registration::on_pushButton_clicked()
{
    QString uid = ui->uid->text().trimmed();
    QString pass = ui->admin_pass->text().trimmed();
    QString confirm = ui->confirm_pass->text().trimmed();

    if (uid.isEmpty() || pass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "Registration", "All fields are required.");
        return;
    }
    
    if (pass != confirm) {
        QMessageBox::warning(this, "Registration", "Passwords do not match.");
        return;
    }

    QFile file("config/project.json");

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Registration", "Failed to open project.json for reading.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, "Registration", "Invalid JSON format: " + parseError.errorString());
        return;
    }

    QJsonObject projectSettings = doc.object();
    projectSettings["uid"] = uid;
    projectSettings["admin"] = pass;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, "Registration", "Failed to open project.json for writing.");
        return;
    }

    QJsonDocument newDoc(projectSettings);
    file.write(newDoc.toJson(QJsonDocument::Indented));
    file.close();

    QMessageBox::information(this, "Registration", "Registration successful!");
    emit registrationComplete();
    this->close();
}

