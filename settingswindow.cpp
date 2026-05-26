#include "settingswindow.h"
#include "machinesettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Налаштування"));
    resize(520, 640);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *container = new QWidget();
    scroll->setWidget(container);

    auto *layout = new QVBoxLayout(container);

    auto *modeGroup = new QGroupBox(tr("Режим роботи"), container);
    auto *modeLayout = new QVBoxLayout(modeGroup);
    freeModeCheck = new QCheckBox(tr("Безкоштовний режим (без оплати)"), modeGroup);
    modeLayout->addWidget(freeModeCheck);
    layout->addWidget(modeGroup);

    auto *modulesGroup = new QGroupBox(tr("Модулі автомата"), container);
    auto *modulesLayout = new QVBoxLayout(modulesGroup);
    coffeeCheck = new QCheckBox(tr("Кава"), modulesGroup);
    waterCheck = new QCheckBox(tr("Вода"), modulesGroup);
    snacksCheck = new QCheckBox(tr("Снеки"), modulesGroup);
    modulesLayout->addWidget(coffeeCheck);
    modulesLayout->addWidget(waterCheck);
    modulesLayout->addWidget(snacksCheck);
    auto *modulesHint = new QLabel(
        tr("Увімкніть лише потрібні модулі. Нижнє меню і екран показуватимуть тільки їх."),
        modulesGroup);
    modulesHint->setWordWrap(true);
    modulesHint->setStyleSheet(QStringLiteral("color: #666; font-size: 12px;"));
    modulesLayout->addWidget(modulesHint);
    layout->addWidget(modulesGroup);

    auto *gpioGroup = new QGroupBox(tr("Плата розширення (GPIO 1–14)"), container);
    gpioSection = gpioGroup;
    auto *gpioLayout = new QVBoxLayout(gpioGroup);

    auto *holdRow = new QHBoxLayout();
    holdRow->addWidget(new QLabel(tr("Час утримання виходу (мс):"), gpioGroup));
    holdMsSpin = new QSpinBox(gpioGroup);
    holdMsSpin->setRange(100, 30000);
    holdMsSpin->setSingleStep(100);
    holdRow->addWidget(holdMsSpin);
    gpioLayout->addLayout(holdRow);

    buildGpioRows();
    for (QSpinBox *spin : coffeeGpioSpins) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Кава — %1").arg(spin->property("label").toString()), gpioGroup));
        row->addStretch();
        row->addWidget(spin);
        gpioLayout->addLayout(row);
    }
    for (QSpinBox *spin : waterGpioSpins) {
        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Вода — %1").arg(spin->property("label").toString()), gpioGroup));
        row->addStretch();
        row->addWidget(spin);
        gpioLayout->addLayout(row);
    }

    auto *snackForm = new QFormLayout();
    snackNameEdit = new QLineEdit(gpioGroup);
    snackPriceSpin = new QDoubleSpinBox(gpioGroup);
    snackPriceSpin->setRange(0, 999);
    snackPriceSpin->setDecimals(2);
    snackPriceSpin->setSuffix(QStringLiteral(" ₴"));
    snackGpioSpin = new QSpinBox(gpioGroup);
    snackGpioSpin->setRange(1, 14);
    snackForm->addRow(tr("Назва снеку:"), snackNameEdit);
    snackForm->addRow(tr("Ціна:"), snackPriceSpin);
    snackForm->addRow(tr("Канал GPIO:"), snackGpioSpin);
    gpioLayout->addLayout(snackForm);
    layout->addWidget(gpioGroup);

    auto *root = new QVBoxLayout(this);
    root->addWidget(scroll);

    auto *btnRow = new QHBoxLayout();
    auto *saveBtn = new QPushButton(tr("Зберегти"), this);
    auto *cancelBtn = new QPushButton(tr("Скасувати"), this);
    btnRow->addStretch();
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(cancelBtn);
    root->addLayout(btnRow);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsWindow::onCancel);

    MachineSettings &cfg = MachineSettings::instance();
    freeModeCheck->setChecked(cfg.freeMode());
    coffeeCheck->setChecked(cfg.moduleEnabled(ProductCategory::Coffee));
    waterCheck->setChecked(cfg.moduleEnabled(ProductCategory::Water));
    snacksCheck->setChecked(cfg.moduleEnabled(ProductCategory::Snacks));
    holdMsSpin->setValue(cfg.buttonHoldMs());

  if (!cfg.snackItems().isEmpty()) {
        const ProductItem &snack = cfg.snackItems().first();
        snackNameEdit->setText(snack.name);
        snackPriceSpin->setValue(snack.priceKopiyky / 100.0);
        snackGpioSpin->setValue(snack.gpioChannel);
    }

    for (int i = 0; i < coffeeGpioSpins.size() && i < cfg.coffeeItems().size(); ++i)
        coffeeGpioSpins[i]->setValue(cfg.coffeeItems()[i].gpioChannel);
    for (int i = 0; i < waterGpioSpins.size() && i < cfg.waterItems().size(); ++i)
        waterGpioSpins[i]->setValue(cfg.waterItems()[i].gpioChannel);
}

void SettingsWindow::buildGpioRows()
{
    MachineSettings &cfg = MachineSettings::instance();
    for (const ProductItem &item : cfg.coffeeItems()) {
        auto *spin = new QSpinBox(gpioSection);
        spin->setRange(1, 14);
        spin->setProperty("label", item.name);
        coffeeGpioSpins.append(spin);
    }
    for (const ProductItem &item : cfg.waterItems()) {
        auto *spin = new QSpinBox(gpioSection);
        spin->setRange(1, 14);
        spin->setProperty("label", item.name);
        waterGpioSpins.append(spin);
    }
}

void SettingsWindow::onSave()
{
    MachineSettings &cfg = MachineSettings::instance();
    cfg.setFreeMode(freeModeCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Coffee, coffeeCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Water, waterCheck->isChecked());
    cfg.setModuleEnabled(ProductCategory::Snacks, snacksCheck->isChecked());
    cfg.setButtonHoldMs(holdMsSpin->value());
    cfg.setSnackDisplayName(snackNameEdit->text().trimmed());
    cfg.setSnackPriceKopiyky(qRound(snackPriceSpin->value() * 100));
    cfg.setSnackGpio(snackGpioSpin->value());

    for (int i = 0; i < coffeeGpioSpins.size(); ++i)
        cfg.setCoffeeGpio(i, coffeeGpioSpins[i]->value());
    for (int i = 0; i < waterGpioSpins.size(); ++i)
        cfg.setWaterGpio(i, waterGpioSpins[i]->value());

    cfg.save();
    emit settingsApplied();
    close();
}

void SettingsWindow::onCancel()
{
    close();
}
