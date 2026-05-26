#include "mainwindow.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QIcon>
#include <QLabel>
#include "settingswindow.h"
#include <QProxyStyle>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QDebug>
#include "pay_price.h"
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QFrame>

class CustomToolButtonStyle : public QProxyStyle {
public:
    CustomToolButtonStyle(int topShift, int bottomShift)
        : m_topShift(topShift), m_bottomShift(bottomShift) {}

    void drawControl(ControlElement element,
                     const QStyleOption *option,
                     QPainter *painter,
                     const QWidget *widget) const override
    {
        if (element == CE_ToolButtonLabel) {
            const auto *toolbutton =
                qstyleoption_cast<const QStyleOptionToolButton *>(option);
            if (toolbutton) {
                QStyleOptionToolButton tbOpt(*toolbutton);
                tbOpt.rect.adjust(0, m_topShift, 0, -m_bottomShift);
                QProxyStyle::drawControl(element, &tbOpt, painter, widget);
                return;
            }
        }
        QProxyStyle::drawControl(element, option, painter, widget);
    }

private:
    int m_topShift;
    int m_bottomShift;
};

static QToolButton *makeNavButton(const QString &text, const QString &iconPath, QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setIcon(QIcon(iconPath));
    btn->setText(text);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setIconSize(QSize(48, 48));
    btn->setFixedSize(120, 100);
    btn->setStyle(new CustomToolButtonStyle(7, 0));
    btn->setCheckable(true);
    return btn;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      device(new RP2040Device(this)),
      coinLogic(new Pulses(this)),
      listener(new GpioListener(this))
{
    setWindowTitle(tr("Tach Coffee & Snack"));
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f4f6f8; }"
        "QLabel#balanceLabel { font-size: 22px; font-weight: 600; color: #1a3a52; }"
        "QLabel#statusLabel { font-size: 16px; color: #555; min-height: 24px; }"
    ));

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    mainLayout->setSpacing(12);

    auto *topRow = new QHBoxLayout();
    balanceLabel = new QLabel(tr("Баланс: ₴0.00"), this);
    balanceLabel->setObjectName(QStringLiteral("balanceLabel"));
    statusLabel = new QLabel(this);
    statusLabel->setObjectName(QStringLiteral("statusLabel"));
    topRow->addWidget(statusLabel);
    topRow->addStretch();
    topRow->addWidget(balanceLabel);
    mainLayout->addLayout(topRow);

    connect(listener, &GpioListener::pulseDetected, coinLogic, &Pulses::handlePulse);
    listener->start();

    connect(coinLogic, &Pulses::moneyUpdated, this, [this](int balance) {
        if (MachineSettings::instance().freeMode()) {
            balanceLabel->hide();
            return;
        }
        balanceLabel->show();
        const double uah = balance / 100.0;
        balanceLabel->setText(tr("Баланс: ₴%1").arg(QString::number(uah, 'f', 2)));
    });

    categoryStack = new QStackedWidget(this);
    MachineSettings &cfg = MachineSettings::instance();

    categoryStack->addWidget(buildCategoryPage(cfg.coffeeItems(), QStringLiteral("#6f4e37")));
    categoryStack->addWidget(buildCategoryPage(cfg.waterItems(), QStringLiteral("#2b7bbb")));
    categoryStack->addWidget(buildCategoryPage(cfg.snackItems(), QStringLiteral("#c45c26")));

    mainLayout->addWidget(categoryStack, 1);

    coffeeBtn = makeNavButton(tr("Кава"), QStringLiteral(":/Icon/Icon/coffee.svg"), this);
    snackBtn = makeNavButton(tr("Снеки"), QStringLiteral(":/Icon/Icon/snack.svg"), this);
    waterBtn = makeNavButton(tr("Вода"), QStringLiteral(":/Icon/Icon/water.svg"), this);

    connect(coffeeBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Coffee);
    });
    connect(snackBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Snacks);
    });
    connect(waterBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Water);
    });

    auto *heightBtn = new QToolButton(this);
    auto *bottomBtn = new QToolButton(this);
    heightBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/height.svg")));
    bottomBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/bottom.svg")));
    for (QToolButton *b : {heightBtn, bottomBtn}) {
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setIconSize(QSize(80, 80));
        b->setFixedSize(100, 100);
        b->hide();
    }

    auto *helpBtn = new QToolButton(this);
    helpBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/Help.svg")));
    helpBtn->setText(tr("Допомога"));
    helpBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    helpBtn->setIconSize(QSize(48, 48));
    helpBtn->setFixedSize(140, 100);
    helpBtn->setStyle(new CustomToolButtonStyle(7, 0));

    auto *settingsBtn = new QToolButton(this);
    settingsBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/Setting.svg")));
    settingsBtn->setText(tr("Налаштування"));
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    settingsBtn->setIconSize(QSize(48, 48));
    settingsBtn->setFixedSize(140, 100);
    settingsBtn->setStyle(new CustomToolButtonStyle(7, 0));

    connect(settingsBtn, &QToolButton::clicked, this, [this]() {
        auto *settingsWin = new SettingsWindow(this);
        connect(settingsWin, &SettingsWindow::settingsApplied,
                this, &MainWindow::onSettingsApplied);
        settingsWin->show();
    });

    bottomLeftPanel = new QWidget(this);
    auto *leftLayout = new QHBoxLayout(bottomLeftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);
    leftLayout->addWidget(coffeeBtn);
    leftLayout->addWidget(snackBtn);
    leftLayout->addWidget(waterBtn);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->addWidget(bottomLeftPanel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(helpBtn);
    bottomLayout->addWidget(settingsBtn);
    mainLayout->addLayout(bottomLayout);

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == 0x2e8a) {
            if (device->connect(info.portName(), 115200))
                statusLabel->setText(tr("Плата розширення: підключено (%1)").arg(info.portName()));
            else
                statusLabel->setText(tr("Плата розширення: порт не відкрито"));
            break;
        }
    }

    applyMachineSettings();
}

MainWindow::~MainWindow()
{
    if (listener) {
        listener->requestInterruption();
        listener->wait();
    }
}

QWidget *MainWindow::buildCategoryPage(const QVector<ProductItem> &items,
                                         const QString &accentColor)
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    for (const ProductItem &item : items) {
        auto *card = new QFrame(page);
        card->setFixedSize(560, 420);
        card->setStyleSheet(QStringLiteral(
            "QFrame {"
            "  background: white;"
            "  border-radius: 20px;"
            "  border: 2px solid %1;"
            "}"
        ).arg(accentColor));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(8);
        cardLayout->setContentsMargins(24, 20, 24, 20);

        auto *iconBtn = new QToolButton(card);
        iconBtn->setIcon(QIcon(item.iconPath));
        iconBtn->setIconSize(QSize(280, 200));
        iconBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        iconBtn->setFixedSize(300, 220);
        iconBtn->setStyleSheet(QStringLiteral("border: none; background: transparent;"));

        auto *nameLabel = new QLabel(item.name, card);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet(QStringLiteral(
            "font-size: 28px; font-weight: 700; color: #222;"
        ));

        auto *priceLabel = new QLabel(item.priceText, card);
        priceLabel->setAlignment(Qt::AlignCenter);
        priceLabel->setStyleSheet(QStringLiteral(
            "font-size: 22px; color: %1; font-weight: 600;"
        ).arg(accentColor));

        cardLayout->addWidget(iconBtn, 0, Qt::AlignCenter);
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(priceLabel);

        connect(iconBtn, &QToolButton::clicked, this, [this, item]() {
            handleProductClick(item);
        });

        layout->addWidget(card, 0, Qt::AlignCenter);
    }

    return page;
}

void MainWindow::handleProductClick(const ProductItem &item)
{
    MachineSettings &cfg = MachineSettings::instance();
    const bool freeMode = cfg.freeMode();

    if (!freeMode) {
        Pay_Price payDialog(item.name, item.priceText, QIcon(item.iconPath), this);
        connect(coinLogic, &Pulses::moneyUpdated, &payDialog, [&payDialog](int newBalance) {
            payDialog.updateBalance(newBalance);
        });
        payDialog.updateBalance(coinLogic->getBalance());

        if (payDialog.exec() != QDialog::Accepted)
            return;
        if (!coinLogic->deduct(payDialog.getPrice()))
            return;
        coinLogic->resetBalance();
    }

    statusLabel->setText(tr("Видача: %1…").arg(item.name));
    const auto result = device->sendDispenseCommand(
        item.gpioChannel, cfg.buttonHoldMs());

    switch (result) {
    case RP2040Device::DispenseResult::Success:
        statusLabel->setText(tr("Готово: %1").arg(item.name));
        break;
    case RP2040Device::DispenseResult::Error:
        statusLabel->setText(tr("Помилка плати розширення"));
        QMessageBox::warning(this, tr("Помилка"),
                             tr("Плата розширення повідомила про помилку видачі."));
        break;
    case RP2040Device::DispenseResult::PortClosed:
        statusLabel->setText(tr("Немає зв’язку з платою"));
        QMessageBox::warning(this, tr("Зв’язок"),
                             tr("Послідовний порт до RP2040 не відкритий."));
        break;
    case RP2040Device::DispenseResult::Timeout:
        statusLabel->setText(tr("Немає підтвердження від плати"));
        QMessageBox::warning(this, tr("Таймаут"),
                             tr("Плата не надіслала підтвердження. Перевірте прошивку main2."));
        break;
    }
}

void MainWindow::switchCategory(ProductCategory category)
{
    if (!MachineSettings::instance().moduleEnabled(category))
        return;

    currentCategory = category;
    categoryStack->setCurrentIndex(static_cast<int>(category));

    if (category == ProductCategory::Coffee)
        setActiveModuleButton(coffeeBtn);
    else if (category == ProductCategory::Water)
        setActiveModuleButton(waterBtn);
    else
        setActiveModuleButton(snackBtn);
}

void MainWindow::setActiveModuleButton(QToolButton *active)
{
    for (QToolButton *b : {coffeeBtn, waterBtn, snackBtn}) {
        const bool on = (b == active);
        b->setChecked(on);
        b->setStyleSheet(on
            ? QStringLiteral("QToolButton { background: #dce8f5; border-radius: 8px; }")
            : QString());
    }
}

void MainWindow::updateModuleButtons()
{
    MachineSettings &cfg = MachineSettings::instance();
    coffeeBtn->setVisible(cfg.moduleEnabled(ProductCategory::Coffee));
    waterBtn->setVisible(cfg.moduleEnabled(ProductCategory::Water));
    snackBtn->setVisible(cfg.moduleEnabled(ProductCategory::Snacks));

    if (cfg.freeMode())
        balanceLabel->hide();
    else
        balanceLabel->show();

    ProductCategory first = ProductCategory::Snacks;
    if (cfg.moduleEnabled(ProductCategory::Coffee))
        first = ProductCategory::Coffee;
    else if (cfg.moduleEnabled(ProductCategory::Water))
        first = ProductCategory::Water;
    else if (cfg.moduleEnabled(ProductCategory::Snacks))
        first = ProductCategory::Snacks;

    const bool anyModule = cfg.moduleEnabled(ProductCategory::Coffee)
                        || cfg.moduleEnabled(ProductCategory::Water)
                        || cfg.moduleEnabled(ProductCategory::Snacks);

    if (!anyModule) {
        statusLabel->setText(tr("Увімкніть хоча б один модуль у налаштуваннях"));
        return;
    }

    if (cfg.moduleEnabled(currentCategory))
        switchCategory(currentCategory);
    else
        switchCategory(first);
}

void MainWindow::applyMachineSettings()
{
    updateModuleButtons();
}

void MainWindow::onSettingsApplied()
{
    while (categoryStack->count() > 0) {
        QWidget *w = categoryStack->widget(0);
        categoryStack->removeWidget(w);
        w->deleteLater();
    }

    MachineSettings &cfg = MachineSettings::instance();
    categoryStack->insertWidget(0, buildCategoryPage(cfg.coffeeItems(), QStringLiteral("#6f4e37")));
    categoryStack->insertWidget(1, buildCategoryPage(cfg.waterItems(), QStringLiteral("#2b7bbb")));
    categoryStack->insertWidget(2, buildCategoryPage(cfg.snackItems(), QStringLiteral("#c45c26")));

    applyMachineSettings();
}
