#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "rp2040device.h"
#include "pulses.h"
#include "gpio_listener.h"
#include <QToolButton>
#include <QLabel>
#include <QVector>
#include <QGridLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    RP2040Device *device;   // робота з платою
    Pulses *coinLogic;      // логіка монетника
    GpioListener *listener; // слухач GPIO
    int currentPage;        // поточна сторінка
    const int itemsPerPage = 6; // кількість товарів на сторінку

    QLabel *balanceLabel;
    QVector<QToolButton*> itemButtons;
    QVector<QLabel*> priceLabels;
    QVector<QWidget*> cellWidgets;

    QGridLayout *grid;      // головна сітка для товарів
};

#endif // MAINWINDOW_H
