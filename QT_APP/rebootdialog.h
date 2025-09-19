#ifndef REBOOTDIALOG_H
#define REBOOTDIALOG_H

#include <QDialog>

namespace Ui {
class RebootDialog;
}

class RebootDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RebootDialog(QWidget *parent = nullptr);
    ~RebootDialog();

private:
    Ui::RebootDialog *ui;
};

#endif // REBOOTDIALOG_H
