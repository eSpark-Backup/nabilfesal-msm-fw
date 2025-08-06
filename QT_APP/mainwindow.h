#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "./login.h"
#include "./channels.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    void updateProgress();

private:
    Ui::MainWindow *ui;
    LoginWidget *login;
    ChannelsHome *channels;
    int progressValue;
};
#endif // MAINWINDOW_H
