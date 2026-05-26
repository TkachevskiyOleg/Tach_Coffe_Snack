#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.resize(1024, 768);   // стартовий розмір вікна
    w.move(200, 100);      // позиція на екрані
    w.show();              // показати у віконному режимі

    return a.exec();
}
