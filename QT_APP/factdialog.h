#ifndef FACTDIALOG_H
#define FACTDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class FactDialog;
}

class FactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FactDialog(QWidget *parent = nullptr);
    ~FactDialog();
    void setLabel(const std::string& data);

private:
    Ui::FactDialog *ui;
};

#endif // FACTDIALOG_H
