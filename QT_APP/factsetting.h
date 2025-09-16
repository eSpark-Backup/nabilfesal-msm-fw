#ifndef FACTSETTING_H
#define FACTSETTING_H

#include <QWidget>

namespace Ui {
class FactSetting;
}

class FactSetting : public QWidget
{
    Q_OBJECT

public:
    explicit FactSetting(QWidget *parent = nullptr);
    ~FactSetting();
signals:
    void usbExport();
    void factoryExit();
private slots:
    void on_next_btn_clicked();

    void on_back_btn_clicked();

    void on_exp_btn_clicked();

    void on_exit_btn_clicked();

    void on_reboot_btn_clicked();

private:
    Ui::FactSetting *ui;
};

#endif // FACTSETTING_H
