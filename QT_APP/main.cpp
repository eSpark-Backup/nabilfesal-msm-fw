#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QInputMethod>
#include "buttonSound.h"
#include <QScreen>
#include "buildinfo.h"
#define BUILD_VERSION "2.1.0"

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
        connect(&AwsMqttManager::instance(), &AwsMqttManager::settingsChanged, this, &InactivityWatcher::wakeUp);
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

public slots:
    void wakeUp() {
        resetTimer();
        if (overlay->isVisible())
            overlay->hide();
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
    qDebug().noquote() << "===================================================================================================";
    qDebug().noquote() << "                                         Build Information";
    qDebug().noquote() << "===================================================================================================";
    qDebug().noquote() << "Version:         " << BUILD_VERSION;
    qDebug().noquote() << "Build date:      " << BUILD_DATE << "time:" << BUILD_TIME;
    qDebug().noquote() << "Developer:       " << BUILD_AUTHOR;
    qDebug().noquote() << "Branch:          " << GIT_BRANCH;
    qDebug().noquote() << "Commit Hash:     " << GIT_COMMIT_HASH;
    qDebug().noquote() << "===================================================================================================";

    qRegisterMetaType<std::array<EnergyData, 13>>("std::array<EnergyData, 13>");
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
