#ifndef FACTORYSETTINGS_H
#define FACTORYSETTINGS_H

#include <QWidget>

namespace Ui {
class FactorySettings;
}

class FactorySettings : public QWidget
{
    Q_OBJECT

public:
    explicit FactorySettings(QString hardwareVersion, QString softwareVersion, QWidget *parent = nullptr);
    ~FactorySettings();

signals:
    void factoryExit();
    void loadMeasurements();
    void loadFactSettings();

private slots:
    void on_pushButton_clicked();

    void on_measurement_clicked();

    void on_system_clicked();

private:
    Ui::FactorySettings *ui;
};

#endif // FACTORYSETTINGS_H
