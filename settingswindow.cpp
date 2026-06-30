#include "settingswindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QPalette>
#include <QColor>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(tr("Налаштування"));
    resize(480, 560);

    // Окреме вікно з власним фоном, модальне поверх головного.
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose);
    // Непрозорий фон (інакше видно вміст головного вікна за діалогом).
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0xf0, 0xf2, 0xf5));
    setPalette(pal);

    MachineSettings &cfg = MachineSettings::instance();

    // Копіюємо всі товари у робочий буфер (редагуємо буфер, застосовуємо при «Зберегти»)
    bufCoffee = cfg.coffeeItems();
    bufWater  = cfg.waterItems();
    bufSnack  = cfg.snackItems();

    auto *root = new QVBoxLayout(this);

    // --- Режим роботи ---
    auto *modeGroup = new QGroupBox(tr("Режим роботи"), this);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    freeModeCheck = new QCheckBox(tr("Безкоштовний режим (без оплати)"), modeGroup);
    modeLayout->addWidget(freeModeCheck);
    root->addWidget(modeGroup);

    // --- Модулі автомата ---
    auto *modulesGroup = new QGroupBox(tr("Модулі автомата"), this);
    auto *modulesLayout = new QVBoxLayout(modulesGroup);
    coffeeCheck = new QCheckBox(tr("Кава"), modulesGroup);
    waterCheck  = new QCheckBox(tr("Вода"), modulesGroup);
    snacksCheck = new QCheckBox(tr("Снеки"), modulesGroup);
    modulesLayout->addWidget(coffeeCheck);
    modulesLayout->addWidget(waterCheck);
    modulesLayout->addWidget(snacksCheck);
    root->addWidget(modulesGroup);

    // --- Редагування товару ---
    auto *editGroup = new QGroupBox(tr("Налаштування товару"), this);
    editGroupBox = editGroup;
    auto *editForm = new QFormLayout(editGroup);

    moduleCombo = new QComboBox(editGroup);
    moduleLabel = new QLabel(tr("Модуль:"), editGroup);
    editForm->addRow(moduleLabel, moduleCombo);

    countSpin = new QSpinBox(editGroup);
    countSpin->setRange(1, 50);
    editForm->addRow(tr("Кількість товарів:"), countSpin);

    productCombo = new QComboBox(editGroup);
    editForm->addRow(tr("Товар:"), productCombo);

    nameEdit = new QLineEdit(editGroup);
    editForm->addRow(tr("Назва:"), nameEdit);

    priceSpin = new QDoubleSpinBox(editGroup);
    priceSpin->setRange(0, 9999);
    priceSpin->setDecimals(2);
    priceSpin->setSuffix(QStringLiteral(" ₴"));
    editForm->addRow(tr("Ціна:"), priceSpin);

    gpioSpin = new QSpinBox(editGroup);
    gpioSpin->setRange(1, 14);
    editForm->addRow(tr("Канал GPIO (1–14):"), gpioSpin);

    // Тривалість роботи виходу саме для цього товару
    itemHoldSpin = new QDoubleSpinBox(editGroup);
    itemHoldSpin->setRange(0.1, 30.0);
    itemHoldSpin->setDecimals(1);
    itemHoldSpin->setSingleStep(0.5);
    itemHoldSpin->setSuffix(QStringLiteral(" с"));
    editForm->addRow(tr("Час видачі товару:"), itemHoldSpin);

    // Поле, специфічне для води
    sparklingLabel = new QLabel(tr("Тип води:"), editGroup);
    sparklingCheck = new QCheckBox(tr("Газована"), editGroup);
    editForm->addRow(sparklingLabel, sparklingCheck);

    root->addWidget(editGroup);

    // --- Кнопки ---
    auto *btnRow = new QHBoxLayout();
    auto *saveBtn = new QPushButton(tr("Зберегти"), this);
    auto *cancelBtn = new QPushButton(tr("Скасувати"), this);
    btnRow->addStretch();
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    // --- Початкові значення ---
    freeModeCheck->setChecked(cfg.freeMode());
    coffeeCheck->setChecked(cfg.moduleEnabled(ProductCategory::Coffee));
    waterCheck->setChecked(cfg.moduleEnabled(ProductCategory::Water));
    snacksCheck->setChecked(cfg.moduleEnabled(ProductCategory::Snacks));

    // --- Сигнали ---
    connect(saveBtn, &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsWindow::onCancel);
    connect(moduleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onModuleChanged);
    connect(productCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onProductChanged);
    connect(countSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsWindow::onCountChanged);
    // Перемикання модулів автомата оновлює список доступних модулів у товарах
    connect(coffeeCheck, &QCheckBox::toggled, this, &SettingsWindow::onEnabledModulesChanged);
    connect(waterCheck,  &QCheckBox::toggled, this, &SettingsWindow::onEnabledModulesChanged);
    connect(snacksCheck, &QCheckBox::toggled, this, &SettingsWindow::onEnabledModulesChanged);

    // Перше заповнення: спершу список модулів, потім товари
    rebuildModuleCombo();
}

ProductCategory SettingsWindow::currentCategory() const
{
    return activeCategory;
}

QVector<ProductItem> &SettingsWindow::buffer(ProductCategory cat)
{
    switch (cat) {
    case ProductCategory::Coffee: return bufCoffee;
    case ProductCategory::Water:  return bufWater;
    case ProductCategory::Snacks: return bufSnack;
    }
    return bufSnack;
}

void SettingsWindow::reloadProductList()
{
    const ProductCategory cat = currentCategory();
    const QVector<ProductItem> &v = buffer(cat);

    // Снеки не можуть перевищувати кількість фізичних виходів плати (по одному
    // виходу на снек). Кава/Вода можуть мати більше позицій (вони на спільних
    // каналах). Тому для снеків верхня межа = kOutputCount, для решти — 50.
    const int maxCount = (cat == ProductCategory::Snacks)
                             ? MachineSettings::kOutputCount : 50;

    // показати поточну кількість товарів модуля, не викликаючи onCountChanged
    countSpin->blockSignals(true);
    countSpin->setRange(1, maxCount);
    countSpin->setValue(qMin(v.size(), maxCount));
    countSpin->blockSignals(false);

    productCombo->blockSignals(true);
    productCombo->clear();
    for (int i = 0; i < v.size(); ++i)
        productCombo->addItem(QStringLiteral("%1. %2").arg(i + 1).arg(v[i].name), i);

    currentProductIndex = v.isEmpty() ? -1 : 0;
    productCombo->setCurrentIndex(currentProductIndex);
    productCombo->blockSignals(false);
}

void SettingsWindow::loadProductIntoForm()
{
    const ProductCategory cat = currentCategory();
    const QVector<ProductItem> &v = buffer(cat);
    if (currentProductIndex < 0 || currentProductIndex >= v.size())
        return;

    loadingForm = true;
    const ProductItem &it = v[currentProductIndex];
    nameEdit->setText(it.name);
    priceSpin->setValue(it.priceKopiyky / 100.0);
    gpioSpin->setValue(it.gpioChannel);
    itemHoldSpin->setValue(it.holdMs / 1000.0);

    // Поля, специфічні для модуля
    const bool isWater = (cat == ProductCategory::Water);
    sparklingLabel->setVisible(isWater);
    sparklingCheck->setVisible(isWater);
    if (isWater)
        sparklingCheck->setChecked(it.sparkling);

    loadingForm = false;
}

void SettingsWindow::commitFormToBuffer()
{
    if (loadingForm)
        return;
    const ProductCategory cat = currentCategory();
    QVector<ProductItem> &v = buffer(cat);
    if (currentProductIndex < 0 || currentProductIndex >= v.size())
        return;

    ProductItem &it = v[currentProductIndex];
    it.name = nameEdit->text().trimmed().isEmpty()
        ? it.name : nameEdit->text().trimmed();
    it.priceKopiyky = qRound(priceSpin->value() * 100);
    it.priceText = QStringLiteral("₴%1").arg(it.priceKopiyky / 100.0, 0, 'f', 2);
    it.gpioChannel = gpioSpin->value();
    it.holdMs = qRound(itemHoldSpin->value() * 1000);
    if (cat == ProductCategory::Water) {
        it.sparkling = sparklingCheck->isChecked();
        it.iconPath = it.sparkling
            ? QStringLiteral(":/Icon/Icon/gaz_water2.svg")
            : QStringLiteral(":/Icon/Icon/water.svg");
    }
}

void SettingsWindow::rebuildModuleCombo()
{
    // Запам'ятати поточний обраний модуль, щоб за можливості зберегти вибір
    ProductCategory prev = ProductCategory::Snacks;
    bool hadSelection = (moduleCombo->count() > 0);
    if (hadSelection)
        prev = static_cast<ProductCategory>(moduleCombo->currentData().toInt());

    moduleCombo->blockSignals(true);
    moduleCombo->clear();

    // Додаємо лише увімкнені модулі (за станом галочок «Модулі автомата»)
    if (coffeeCheck->isChecked())
        moduleCombo->addItem(tr("Кава"), static_cast<int>(ProductCategory::Coffee));
    if (waterCheck->isChecked())
        moduleCombo->addItem(tr("Вода"), static_cast<int>(ProductCategory::Water));
    if (snacksCheck->isChecked())
        moduleCombo->addItem(tr("Снеки"), static_cast<int>(ProductCategory::Snacks));

    // Відновити попередній вибір, якщо він ще доступний
    int restoreIdx = 0;
    if (hadSelection) {
        for (int i = 0; i < moduleCombo->count(); ++i) {
            if (moduleCombo->itemData(i).toInt() == static_cast<int>(prev)) {
                restoreIdx = i;
                break;
            }
        }
    }
    if (moduleCombo->count() > 0)
        moduleCombo->setCurrentIndex(restoreIdx);

    moduleCombo->blockSignals(false);

    const int enabledCount = moduleCombo->count();

    // Явно фіксуємо поточну категорію з фактично обраного елемента комбо
    if (enabledCount > 0)
        activeCategory = static_cast<ProductCategory>(
            moduleCombo->itemData(restoreIdx).toInt());

    // Якщо модуль лише один — список вибору зайвий, ховаємо рядок «Модуль»
    moduleCombo->setVisible(enabledCount > 1);
    if (moduleLabel)
        moduleLabel->setVisible(enabledCount > 1);

    // Якщо жоден модуль не ввімкнено — ховаємо всю форму редагування товару
    if (editGroupBox)
        editGroupBox->setVisible(enabledCount > 0);

    if (enabledCount > 0) {
        reloadProductList();
        loadProductIntoForm();
    }
}

void SettingsWindow::onEnabledModulesChanged()
{
    // зберегти поточний товар, бо комбо зараз перебудується
    commitFormToBuffer();
    rebuildModuleCombo();
}

void SettingsWindow::onCountChanged(int newCount)
{
    if (loadingForm)
        return;
    // зберегти поточний товар перед зміною розміру
    commitFormToBuffer();

    const ProductCategory cat = currentCategory();
    QVector<ProductItem> &v = buffer(cat);
    newCount = qBound(1, newCount, 50);
    if (newCount == v.size())
        return;

    if (newCount < v.size()) {
        v.resize(newCount);
    } else {
        for (int i = v.size(); i < newCount; ++i)
            v.append(makeBufferDefault(cat, i));
    }

    reloadProductList();
    loadProductIntoForm();
}

ProductItem SettingsWindow::makeBufferDefault(ProductCategory cat, int index) const
{
    ProductItem it;
    it.gpioChannel = (index % 14) + 1;
    switch (cat) {
    case ProductCategory::Coffee:
        it.name = tr("Кава %1").arg(index + 1);
        it.priceKopiyky = 1500;
        it.iconPath = QStringLiteral(":/Icon/Icon/coffee.svg");
        break;
    case ProductCategory::Water:
        it.name = tr("Вода %1").arg(index + 1);
        it.priceKopiyky = 1000;
        it.iconPath = QStringLiteral(":/Icon/Icon/water.svg");
        break;
    case ProductCategory::Snacks:
        it.name = tr("Снек %1").arg(index + 1);
        it.priceKopiyky = 2000;
        it.iconPath = QStringLiteral(":/Icon/Icon/snack.svg");
        break;
    }
    it.priceText = QStringLiteral("₴%1").arg(it.priceKopiyky / 100.0, 0, 'f', 2);
    return it;
}

void SettingsWindow::onModuleChanged(int comboIndex)
{
    // зберегти поточний товар у СТАРУ категорію перед перемиканням
    commitFormToBuffer();
    // оновити активну категорію з нового вибору комбо
    if (comboIndex >= 0 && comboIndex < moduleCombo->count())
        activeCategory = static_cast<ProductCategory>(
            moduleCombo->itemData(comboIndex).toInt());
    reloadProductList();
    loadProductIntoForm();
}

void SettingsWindow::onProductChanged(int comboIndex)
{
    // зберегти попередній товар
    commitFormToBuffer();
    currentProductIndex = comboIndex;
    loadProductIntoForm();
}

void SettingsWindow::onSave()
{
    // зафіксувати останні зміни форми
    commitFormToBuffer();

    MachineSettings &cfg = MachineSettings::instance();
    cfg.setFreeMode(freeModeCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Coffee, coffeeCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Water, waterCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Snacks, snacksCheck->isChecked());

    // Переносимо весь буфер у MachineSettings
    const struct { ProductCategory cat; const QVector<ProductItem> *buf; } sets[] = {
        { ProductCategory::Coffee, &bufCoffee },
        { ProductCategory::Water,  &bufWater  },
        { ProductCategory::Snacks, &bufSnack  },
    };
    for (const auto &s : sets) {
        // спершу підганяємо кількість товарів у MachineSettings під буфер
        cfg.resizeCategory(s.cat, s.buf->size());
        for (int i = 0; i < s.buf->size(); ++i) {
            const ProductItem &it = s.buf->at(i);
            cfg.setItemName(s.cat, i, it.name);
            cfg.setItemPriceKopiyky(s.cat, i, it.priceKopiyky);
            cfg.setItemGpio(s.cat, i, it.gpioChannel);
            cfg.setItemHoldMs(s.cat, i, it.holdMs);
            if (s.cat == ProductCategory::Water)
                cfg.setItemSparkling(s.cat, i, it.sparkling);
        }
    }

    cfg.save();
    emit settingsApplied();
    close();
}

void SettingsWindow::onCancel()
{
    close();
}
