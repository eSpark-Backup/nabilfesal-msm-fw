#ifndef NUMKEYBOARD_H
#define NUMKEYBOARD_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
class NumKeyboard;
}

class NumKeyboard : public QWidget
{
    Q_OBJECT

public:
    explicit NumKeyboard(QWidget *parent = nullptr);
    ~NumKeyboard();
    void setTarget(QLineEdit *lineEdit);

private slots:
    void numberClicked();
    void on_enter_clicked();

    void on_back_clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QPoint dragStartPos;
private:
    Ui::NumKeyboard *ui;
    QLineEdit *targetLineEdit;
    QList<QPushButton*> numberButtons;
};

#endif // NUMKEYBOARD_H
