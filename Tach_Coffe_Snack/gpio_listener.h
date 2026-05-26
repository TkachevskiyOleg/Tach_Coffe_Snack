#pragma once
#include <QThread>
#include <gpiod.h>

class GpioListener : public QThread {
    Q_OBJECT
public:
    explicit GpioListener(QObject *parent = nullptr);
    void run() override;

signals:
    void pulseDetected(int line); // сигнал у головний потік

private:
    gpiod_chip *chip = nullptr;
    gpiod_line *line21 = nullptr;
    gpiod_line *line26 = nullptr;
};
