#pragma once
#include <QWidget>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QObject>

class InactivityWatcher : public QObject {
    Q_OBJECT
public:
    InactivityWatcher(QWidget *rootWindow, QObject *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *event) override;

public slots:
    void wakeUp();

private slots:
    void goScreenOff();
    void resetTimer();

private:
    QWidget *root;
    QWidget *overlay;
    QTimer *inactivityTimer;
};
