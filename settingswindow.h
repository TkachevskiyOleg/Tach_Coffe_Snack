#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>

#include "machinesettings.h"

class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onSave();
    void onCancel();
    void onModuleChanged(int comboIndex);
    void onProductChanged(int comboIndex);
    void onEnabledModulesChanged();   // змінилися галочки «Модулі автомата»
    void onCountChanged(int newCount); // змінилася кількість товарів модуля

private:
    void rebuildModuleCombo();       // показати в списку лише увімкнені модулі
    void reloadProductList();        // заповнити список товарів обраного модуля
    ProductItem makeBufferDefault(ProductCategory cat, int index) const;
    void loadProductIntoForm();      // показати поля поточного товару
    void commitFormToBuffer();       // зберегти поля поточного товару в буфер
    ProductCategory currentCategory() const;

    // Загальні налаштування
    QCheckBox *freeModeCheck = nullptr;
    QCheckBox *coffeeCheck = nullptr;
    QCheckBox *waterCheck = nullptr;
    QCheckBox *snacksCheck = nullptr;
    QDoubleSpinBox *holdSecSpin = nullptr;

    // Вибір товару
    QComboBox *moduleCombo = nullptr;   // Кава / Вода / Снеки
    QLabel *moduleLabel = nullptr;      // мітка рядка «Модуль» (ховається, якщо модуль один)
    QComboBox *productCombo = nullptr;  // конкретний товар обраного модуля
    QGroupBox *editGroupBox = nullptr;  // вся група «Налаштування товару»
    QSpinBox *countSpin = nullptr;      // кількість товарів у поточному модулі

    // Поля редагування товару
    QLineEdit *nameEdit = nullptr;
    QDoubleSpinBox *priceSpin = nullptr;
    QSpinBox *gpioSpin = nullptr;

    // Залежні від модуля поля
    QLabel *sparklingLabel = nullptr;   // лише для води
    QCheckBox *sparklingCheck = nullptr;

    // Робочий буфер: копія всіх товарів, редагується, застосовується в onSave
    QVector<ProductItem> bufCoffee;
    QVector<ProductItem> bufWater;
    QVector<ProductItem> bufSnack;

    int currentProductIndex = -1;
    bool loadingForm = false;
    ProductCategory activeCategory = ProductCategory::Snacks;  // явна поточна категорія

    QVector<ProductItem> &buffer(ProductCategory cat);
};
