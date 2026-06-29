#include "mainwindow.h"
#include <QApplication>
#include <QtGlobal>
#include <cstdio>

// Фільтр повідомлень: глушить нешкідливі попередження плагіна xcb
// (BadWindow / TranslateCoords), що інколи виникають при відкритті/закритті
// діалогів та випадних списків на X11. На роботу вони не впливають,
// але засмічують лог. Решта повідомлень проходить як зазвичай.
static void messageFilter(QtMsgType type, const QMessageLogContext &context,
                          const QString &msg)
{
    Q_UNUSED(context);
    if (msg.contains(QStringLiteral("QXcbConnection")) &&
        (msg.contains(QStringLiteral("BadWindow")) ||
         msg.contains(QStringLiteral("TranslateCoords")))) {
        return; // ігноруємо саме цей шум
    }

    const QByteArray local = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
        fprintf(stdout, "%s\n", local.constData());
        fflush(stdout);
        break;
    default:
        fprintf(stderr, "%s\n", local.constData());
        break;
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageFilter);

    QApplication a(argc, argv);

    MainWindow w;
    w.resize(1024, 768);   // стартовий розмір вікна
    w.move(200, 100);      // позиція на екрані
    w.show();              // показати у віконному режимі

    return a.exec();
}
