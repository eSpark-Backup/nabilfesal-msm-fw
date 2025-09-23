#ifndef FACTSETTING_H
#define FACTSETTING_H

#include <QWidget>
#include "msmkeyboard.h"
#include "commonSetting.h"

namespace Ui {
class FactSetting;
}

class FactSetting : public settingsManager
{
    Q_OBJECT

public:
    explicit FactSetting(QWidget *parent = nullptr);
    ~FactSetting();

signals:
    void usbExport();
    void factoryExit();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_next_btn_clicked();
    void on_back_btn_clicked();
    void on_exp_btn_clicked();
    void on_exit_btn_clicked();
    void on_reboot_btn_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_6_clicked();
    void on_lan_load_btn_clicked();
    void on_lan_save_btn_clicked();
    void on_mqtt_load_btn_clicked();
    void on_mqtt_save_btn_clicked();
    void on_sftp_load_btn_clicked();
    void on_sftp_save_btn_clicked();

private:
    QString buttonClickedStyle =
        "background-color: #0D47A1;"
        "color: black;"
        "border: none;"
        "border-radius: 8px;"
        "padding: 6px 20px;";
    QString buttonNormalStyle =
        "background-color: #D9D9D9;"
        "color: black;"
        "border: none;"
        "border-radius: 8px;"
        "padding: 6px 20px;";

    Ui::FactSetting *ui;
    MSMKeyboard *keyboard;
    bool isDhcpEnabled = false;
    bool isStaticEnabled = false;
};

#endif // FACTSETTING_H
