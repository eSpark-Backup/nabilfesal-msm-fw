#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QInputMethod>
#include "buttonSound.h"

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
    return a.exec();
}
