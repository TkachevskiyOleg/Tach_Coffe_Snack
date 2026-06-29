#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "rp2040device.h"
#include "pulses.h"
#include "gpio_listener.h"
#include "Billvalidator.h"
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

protected:
    void resizeEvent(QResizeEvent *event) override;

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
        QVector<GridSlot> cells;
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
    void setProductsEnabled(bool enabled);   // блокує/розблоковує кнопки товарів
    void updateProductIconSizes();           // масштабує іконки під розмір комірок
    void switchCategory(ProductCategory category);
    void updateModuleButtons();
    void setActiveModuleButton(QToolButton *active);
    void setupBottomTaskbar(QWidget *central);

    CategoryGridState &gridState(ProductCategory category);

    RP2040Device *device;
    Pulses *coinLogic;
    GpioListener *listener;
    BillValidator *billValidator = nullptr;

    QLabel *balanceLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QStackedWidget *categoryStack = nullptr;
    QFrame *bottomTaskbar = nullptr;

    QToolButton *coffeeBtn = nullptr;
    QToolButton *waterBtn = nullptr;
    QToolButton *snackBtn = nullptr;
    QToolButton *pageUpBtn = nullptr;
    QToolButton *pageDownBtn = nullptr;
    QToolButton *m_settingsBtn = nullptr;   // кнопка налаштувань (видима лише при GPIO15)

    CategoryGridState m_coffeeGrid;
    CategoryGridState m_waterGrid;
    CategoryGridState m_snacksGrid;

    static constexpr int itemsPerPage = 6;
    static constexpr int gridColumns = 3;

    ProductCategory currentCategory = ProductCategory::Snacks;

    // true, поки триває видача товару — блокує паралельні видачі
    bool m_dispensing = false;
};

#endif // MAINWINDOW_H
