#include "mainwindow.h"
#include <QObject>
#include <QWidget>
#include <QApplication>
#include <QFontDatabase>
#include <QInputMethod>
#include "buttonSound.h"
#include "buildinfo.h"
#define BUILD_VERSION "2.1.2"


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
    MainWindow::setInstance(&w);
    w.show();
    return a.exec();
}
