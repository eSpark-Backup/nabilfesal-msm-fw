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
    void loadAwsMqttsSettings(bool showMessages);
    ~FactSetting();

signals:
    void usbExport();
    void factoryExit();
    void aws_mqtts_load();
    void runFleetProvisioning();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void on_mqtt_load_btn_clicked(bool msgBox = true);
    void on_sftp_load_btn_clicked(bool msgBox = true);
    void on_mqtts_load_btn_clicked(bool msgBox = true);

private slots:
    void on_next_btn_clicked();
    void on_back_btn_clicked();
    void on_exp_btn_clicked();
    void on_exit_btn_clicked();
    void on_reboot_btn_clicked();
    void on_mqtt_save_btn_clicked();
    void on_sftp_save_btn_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_clicked();
    void on_pushButton_5_clicked();
    void on_mqtts_save_btn_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_2_clicked();

private:
    Ui::FactSetting *ui;
    MSMKeyboard *keyboard;
};

#endif // FACTSETTING_H
