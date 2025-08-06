#include "login.h"
#include "ui_login.h"
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    ui->setupUi(this);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_loginButton_clicked()
{
    auto password = ui->passwordLineEdit->text();
    if (password == "espark")
    {
        emit loginSuccess();
    }
    else
    {
        QMessageBox::warning(this, "Login Failed", "Incorrect password. Please try again.");
    }

}
