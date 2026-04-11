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
    explicit LoginWidget(QString hardwareVersion, QString softwareVersion, QWidget *parent = nullptr);
    ~LoginWidget();

signals:
    void adminLoginSuccess();
    void factoryLoginSuccess();
    void usbLoginSuccess();
    void loadChannels();
    void loadDiagnotics();
    
private slots:
    void on_loginButton_pressed();
    void on_pushButton_clicked();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::LoginWidget *ui;
    MSMKeyboard *keyboard;
};

#endif // LOGINWIDGET_H
