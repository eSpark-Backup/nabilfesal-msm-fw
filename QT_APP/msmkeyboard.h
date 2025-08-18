#ifndef MSMKEYBOARD_H
#define MSMKEYBOARD_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
class MSMKeyboard;
}

class MSMKeyboard : public QWidget
{
    Q_OBJECT

public:
    explicit MSMKeyboard(QWidget *parent = nullptr);
    ~MSMKeyboard();
    void setTarget(QLineEdit *lineEdit);

private slots:
    void numberClicked();
    void letterClicked();
    void on_backspace_clicked();
    void on_enter_clicked();
    void on_enter_2_clicked();
    void on_shift_clicked();

    void on_hide_clicked();

private:
    Ui::MSMKeyboard *ui;
    QLineEdit *targetLineEdit;
    bool shiftEnabled;
    QList<QPushButton*> letterButtons;
    QList<QPushButton*> numberButtons;
};

#endif // MSMKEYBOARD_H
