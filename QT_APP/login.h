#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

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
    void loginSuccess();
private slots:
    void on_loginButton_clicked();

private:
    Ui::LoginWidget *ui;
};

#endif // LOGINWIDGET_H
