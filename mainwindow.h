#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "rp2040device.h"
#include "pulses.h"
#include "gpio_listener.h"
#include "machinesettings.h"
#include <QToolButton>
#include <QLabel>
#include <QStackedWidget>
#include <QWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSettingsApplied();
    void applyMachineSettings();

private:
    QWidget *buildCategoryPage(const QVector<ProductItem> &items, const QString &accentColor);
    void handleProductClick(const ProductItem &item);
    void switchCategory(ProductCategory category);
    void updateModuleButtons();
    void setActiveModuleButton(QToolButton *active);

    RP2040Device *device;
    Pulses *coinLogic;
    GpioListener *listener;

    QLabel *balanceLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QStackedWidget *categoryStack = nullptr;

    QToolButton *coffeeBtn = nullptr;
    QToolButton *waterBtn = nullptr;
    QToolButton *snackBtn = nullptr;

    QWidget *bottomLeftPanel = nullptr;
    ProductCategory currentCategory = ProductCategory::Snacks;
};

#endif // MAINWINDOW_H
