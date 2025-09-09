#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include "msmkeyboard.h"

namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

signals:
    void adminLoginSuccess();
    void factoryLoginSuccess();
private slots:
    void on_loginButton_clicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::LoginWidget *ui;
    MSMKeyboard *keyboard;
};

#endif // LOGINWIDGET_H
