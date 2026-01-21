#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <QWidget>
#include "msmkeyboard.h"
#include "httpsManager.h"


namespace Ui {
class Registration;
}

class Registration : public QWidget
{
    Q_OBJECT

public:
    explicit Registration(QString hardwareVersion, QString softwareVersion, QWidget *parent = nullptr);
    ~Registration();

signals:
    void registrationComplete();
    void requestResp(const QJsonObject &response, int httpCode);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_pushButton_clicked();

private:
    Ui::Registration *ui;
    MSMKeyboard *keyboard;
    HttpsManager *httpsHandler = nullptr;
    QString uid, pass;

};

#endif // REGISTRATION_H
