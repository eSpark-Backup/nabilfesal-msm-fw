#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <QWidget>
#include "msmkeyboard.h"


namespace Ui {
class Registration;
}

class Registration : public QWidget
{
    Q_OBJECT

public:
    explicit Registration(QWidget *parent = nullptr);
    ~Registration();

signals:
    void registrationComplete();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_pushButton_clicked();

private:
    QString getParentFrameName(QLineEdit *lineEdit);

private:
    Ui::Registration *ui;
    MSMKeyboard *keyboard;
};

#endif // REGISTRATION_H
