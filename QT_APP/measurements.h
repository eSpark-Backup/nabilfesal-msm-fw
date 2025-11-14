#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <QWidget>
#include "numkeyboard.h"
#include <QLabel>
#include <QRegularExpressionValidator>

namespace Ui {
class Measurements;
}

class Measurements : public QWidget
{
    Q_OBJECT

public:
    explicit Measurements(QWidget *parent = nullptr);
    void ration_btn_clicked();
    void i_off_btn_clicked();
    void v_off_btn_clicked();
    ~Measurements();
signals:
    void measureExit();
    void requestSerialCommand(const QString &cmd);
    void ct_ratio_changed(const QJsonObject &ctRatio);
    void v_offset_changed(const QJsonObject &v_offset);
    void i_offset_changed(const QJsonObject &i_offset);

private slots:
    void on_back_1_clicked();
    void on_load_btn_clicked();
    void on_save_btn_clicked();
    void on_toolButton_clicked();
    void on_toolButton_2_clicked();
    void on_toolButton_3_clicked();
    void on_toolButton_4_clicked();
    void on_toolButton_5_clicked();
    void on_toolButton_6_clicked();
    void on_toolButton_7_clicked();
    void on_toolButton_8_clicked();
    void on_toolButton_9_clicked();
    void on_toolButton_10_clicked();
    void on_toolButton_11_clicked();
    void on_toolButton_12_clicked();

public slots:
    void parse_load_store_vOffset(const QJsonObject &payload);
    void parse_load_store_iOffset(const QJsonObject &payload);
    void parse_load_store_ctRatio(const QJsonObject &payload);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void parse_load_store_generic(const QJsonObject &payload, const QString &key);
    bool areSettingsUnchanged(const QJsonObject &current, const QString &filePath);
    void processSettings(const QString &key, const QJsonObject &settingsObj,
                    const QString &command, bool updateUI, bool emitDeviceCommand);
private:
    Ui::Measurements *ui;
    bool isRatioActivated = false;
    bool isVOffsetActivated = false;
    bool isIOffsetActivated = false;
    NumKeyboard *keyboard;
    QList<QLabel*> signEdits;
    QList<QLineEdit*> edits;
    QDoubleValidator *validator = nullptr;
};

#endif // MEASUREMENTS_H
