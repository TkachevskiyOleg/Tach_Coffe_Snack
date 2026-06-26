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
#include <QVector>
#include <QFrame>

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
    struct GridSlot {
        QWidget *cell = nullptr;
        QToolButton *iconBtn = nullptr;
        QLabel *nameLabel = nullptr;
        QLabel *priceLabel = nullptr;
    };

    struct CategoryGridState {
        QWidget *page = nullptr;
        QVector<GridSlot> slots;
        QVector<ProductItem> items;
        int currentPage = 0;
    };

    QWidget *buildCategoryGridPage(const QVector<ProductItem> &items,
                                   CategoryGridState &state);
    void showGridPage(ProductCategory category);
    void pageUp();
    void pageDown();
    void rebuildAllCategoryPages();
    void handleProductClick(const ProductItem &item);
    void switchCategory(ProductCategory category);
    void updateModuleButtons();
    void setActiveModuleButton(QToolButton *active);
    void setupBottomTaskbar(QWidget *central);

    CategoryGridState &gridState(ProductCategory category);

    RP2040Device *device;
    Pulses *coinLogic;
    GpioListener *listener;

    QLabel *balanceLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QStackedWidget *categoryStack = nullptr;
    QFrame *bottomTaskbar = nullptr;

    QToolButton *coffeeBtn = nullptr;
    QToolButton *waterBtn = nullptr;
    QToolButton *snackBtn = nullptr;
    QToolButton *pageUpBtn = nullptr;
    QToolButton *pageDownBtn = nullptr;

    CategoryGridState m_coffeeGrid;
    CategoryGridState m_waterGrid;
    CategoryGridState m_snacksGrid;

    static constexpr int itemsPerPage = 6;
    static constexpr int gridColumns = 3;

    ProductCategory currentCategory = ProductCategory::Snacks;
};

#endif // MAINWINDOW_H
