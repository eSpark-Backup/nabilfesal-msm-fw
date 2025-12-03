#include "inactivitywatcher.h"

InactivityWatcher::InactivityWatcher(QWidget *rootWindow, QObject *parent)
        : QObject(parent), root(rootWindow)
{
    inactivityTimer = new QTimer(this);
    inactivityTimer->setInterval(2 * 60 * 1000); // 2 min
    inactivityTimer->setSingleShot(true);

    connect(inactivityTimer, &QTimer::timeout, this, &InactivityWatcher::goScreenOff);
    inactivityTimer->start();

    // Overlay (black screen)
    overlay = new QWidget(nullptr);   // top-level, not child of root
    overlay->setStyleSheet("background-color:black;");
    overlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    overlay->setGeometry(screenRect);
    overlay->hide();
}

bool InactivityWatcher::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:       // <--- add this
    case QEvent::KeyPress:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:     // <--- add this
        resetTimer();
        if (overlay->isVisible()) {
            overlay->hide();   // wake up
            return true;       // eat first event so UI doesn’t get accidental input
        }
        break;
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void InactivityWatcher::wakeUp() {
    resetTimer();
    if (overlay->isVisible())
        overlay->hide();
}


void InactivityWatcher::goScreenOff() {
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    overlay->setGeometry(screenRect);
    overlay->showFullScreen();
}


void InactivityWatcher::resetTimer() {
    if (inactivityTimer->isActive())
        inactivityTimer->stop();
    inactivityTimer->start();
}

