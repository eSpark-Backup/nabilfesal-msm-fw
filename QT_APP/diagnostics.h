#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <QWidget>

namespace Ui {
class Diagnostics;
}

class Diagnostics : public QWidget
{
    Q_OBJECT

public:
    explicit Diagnostics(QWidget *parent = nullptr);
    ~Diagnostics();

private slots:
    void on_pushButton_clicked();

signals:
    void diagExit();

private:
    Ui::Diagnostics *ui;
};

#endif // DIAGNOSTICS_H
