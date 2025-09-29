#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QInputMethod>
#include "buttonSound.h"
#include <QScreen>


class InactivityWatcher : public QObject {
    Q_OBJECT
public:
    InactivityWatcher(QWidget *rootWindow, QObject *parent = nullptr)
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

    bool eventFilter(QObject *obj, QEvent *event) override {
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


private slots:
    void goScreenOff() {
        QRect screenRect = QGuiApplication::primaryScreen()->geometry();
        overlay->setGeometry(screenRect);
        overlay->showFullScreen();
    }


    void resetTimer() {
        if (inactivityTimer->isActive())
            inactivityTimer->stop();
        inactivityTimer->start();
    }


private:
    QWidget *root;
    QWidget *overlay;
    QTimer *inactivityTimer;
};


int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    int fontId = QFontDatabase::addApplicationFont(":/icons/gilroy-semibold.ttf");
    QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont gilroy(family);
    QApplication::setFont(gilroy);

    ButtonSound *sound = new ButtonSound(&a);
    a.installEventFilter(sound);

    MainWindow w;
    w.show();

    InactivityWatcher *watcher = new InactivityWatcher(&w);
    qApp->installEventFilter(watcher);

    return a.exec();
}

#include "main.moc"
