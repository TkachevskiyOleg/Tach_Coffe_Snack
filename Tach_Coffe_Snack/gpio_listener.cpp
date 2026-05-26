#include "gpio_listener.h"
#include <gpiod.h>
#include <QDebug>
#include <poll.h>

GpioListener::GpioListener(QObject *parent)
    : QThread(parent) {}

void GpioListener::run() {
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        qDebug() << "Не вдалося відкрити gpiochip0";
        return;
    }

    line21 = gpiod_chip_get_line(chip, 21);
    line26 = gpiod_chip_get_line(chip, 26);

    if (!line21 || !line26) {
        qDebug() << "Не вдалося отримати лінії GPIO21/26";
        gpiod_chip_close(chip);
        return;
    }

    struct gpiod_line_request_config cfg;
    cfg.consumer = "coin";
    // Ловимо тільки спадний фронт, щоб уникнути подвоєння
    cfg.request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
    cfg.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;

    if (gpiod_line_request(line21, &cfg, 0) < 0)
        qDebug() << "Помилка запиту GPIO21";
    if (gpiod_line_request(line26, &cfg, 0) < 0)
        qDebug() << "Помилка запиту GPIO26";

    int fd21 = gpiod_line_event_get_fd(line21);
    int fd26 = gpiod_line_event_get_fd(line26);

    struct pollfd fds[2];
    fds[0].fd = fd21; fds[0].events = POLLIN;
    fds[1].fd = fd26; fds[1].events = POLLIN;

    struct gpiod_line_event event;

    while (!isInterruptionRequested()) {
        int ret = poll(fds, 2, -1); // блокуюче очікування
        if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                if (gpiod_line_event_read(line21, &event) == 0) {
                    qDebug() << "Pulse detected on GPIO21!";
                    emit pulseDetected(21);
                }
            }
            if (fds[1].revents & POLLIN) {
                if (gpiod_line_event_read(line26, &event) == 0) {
                    qDebug() << "Pulse detected on GPIO26!";
                    emit pulseDetected(26);
                }
            }
        }
    }

    gpiod_chip_close(chip);
}
