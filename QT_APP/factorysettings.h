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
    explicit FactorySettings(QWidget *parent = nullptr);
    ~FactorySettings();

signals:
    void factoryExit();

private slots:
    void on_pushButton_clicked();

private:
    Ui::FactorySettings *ui;
};

#endif // FACTORYSETTINGS_H
