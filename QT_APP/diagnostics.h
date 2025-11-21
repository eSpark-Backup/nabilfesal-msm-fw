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
    void on_moveRight_clicked();
    void on_moveLeft_clicked();

signals:
    void diagExit();

private:
    Ui::Diagnostics *ui;
    int index = 0;
};

#endif // DIAGNOSTICS_H
