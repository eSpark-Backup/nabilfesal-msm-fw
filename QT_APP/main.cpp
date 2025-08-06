#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>
#include <QInputMethod>
#include <QByteArray>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QApplication a(argc, argv);

    int fontId = QFontDatabase::addApplicationFont(":/icons/gilroy-semibold.ttf");
    QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont gilroy(family);
    QApplication::setFont(gilroy);

    MainWindow w;
    w.show();
    return a.exec();
}
