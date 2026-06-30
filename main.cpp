#include "mainwindow.h"
#include <QApplication>
#include <QtGlobal>
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <execinfo.h>
#include <unistd.h>

// Обробник аварійних сигналів: при краші (SIGSEGV/SIGABRT) друкує стек викликів,
// щоб одразу бачити місце падіння без окремого запуску gdb.
static void crashHandler(int sig)
{
    void *frames[64];
    const int n = backtrace(frames, 64);
    fprintf(stderr, "\n=== CRASH! сигнал %d, стек викликів: ===\n", sig);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    fprintf(stderr, "=== кінець стека ===\n");
    fflush(stderr);
    signal(sig, SIG_DFL);
    raise(sig);
}

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
    // Перехоплюємо аварійні сигнали для друку стека викликів при краші
    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);

    qInstallMessageHandler(messageFilter);

    QApplication a(argc, argv);

    MainWindow w;
    w.resize(1024, 768);   // стартовий розмір вікна
    w.move(200, 100);      // позиція на екрані
    w.show();              // показати у віконному режимі

    return a.exec();
}
