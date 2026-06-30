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
#include <QEventLoop>
#include <QTimer>
#include <QSerialPortInfo>
#include <QSizePolicy>
#include <QResizeEvent>

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

static QToolButton *makeModuleButton(const QString &text, const QString &iconPath, QWidget *parent)
{
    auto *btn = new QToolButton(parent);
    btn->setIcon(QIcon(iconPath));
    btn->setText(text);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setIconSize(QSize(48, 48));
    btn->setFixedSize(110, 88);
    btn->setStyle(new CustomToolButtonStyle(5, 0));
    btn->setCheckable(true);

    // Коли кнопку ховаємо через setVisible(false) (модуль вимкнено в
    // налаштуваннях), вона за замовчуванням перестає займати місце в
    // layout — через це leftBox звужується/розширюється і центральний
    // блок зі стрілками (pageUpBtn/pageDownBtn) "стрибає" вбік.
    // retainSizeWhenHidden змушує layout завжди резервувати під неї
    // місце, навіть коли вона невидима — геометрія панелі лишається
    // стабільною.
    QSizePolicy sp = btn->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    btn->setSizePolicy(sp);

    return btn;
}

MainWindow::CategoryGridState &MainWindow::gridState(ProductCategory category)
{
    switch (category) {
    case ProductCategory::Coffee: return m_coffeeGrid;
    case ProductCategory::Water: return m_waterGrid;
    case ProductCategory::Snacks: return m_snacksGrid;
    }
    return m_snacksGrid;
}

void MainWindow::setupBottomTaskbar(QWidget *central)
{
    bottomTaskbar = new QFrame(central);
    bottomTaskbar->setObjectName(QStringLiteral("bottomTaskbar"));
    bottomTaskbar->setFixedHeight(110);
    bottomTaskbar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto *barLayout = new QHBoxLayout(bottomTaskbar);
    barLayout->setContentsMargins(16, 6, 16, 6);
    barLayout->setSpacing(12);

    coffeeBtn = makeModuleButton(tr("Кава"), QStringLiteral(":/Icon/Icon/coffee.svg"), bottomTaskbar);
    snackBtn = makeModuleButton(tr("Снеки"), QStringLiteral(":/Icon/Icon/snack.svg"), bottomTaskbar);
    waterBtn = makeModuleButton(tr("Вода"), QStringLiteral(":/Icon/Icon/water.svg"), bottomTaskbar);

    connect(coffeeBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Coffee);
    });
    connect(snackBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Snacks);
    });
    connect(waterBtn, &QToolButton::clicked, this, [this]() {
        switchCategory(ProductCategory::Water);
    });

    auto *leftBox = new QWidget(bottomTaskbar);
    auto *leftLayout = new QHBoxLayout(leftBox);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);
    leftLayout->addWidget(coffeeBtn);
    leftLayout->addWidget(snackBtn);
    leftLayout->addWidget(waterBtn);

    pageUpBtn = new QToolButton(bottomTaskbar);
    pageDownBtn = new QToolButton(bottomTaskbar);
    pageUpBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/height.svg")));
    pageDownBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/bottom.svg")));
    for (QToolButton *b : {pageUpBtn, pageDownBtn}) {
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setIconSize(QSize(72, 72));
        b->setFixedSize(88, 88);
        b->setStyle(new CustomToolButtonStyle(4, 0));
    }
    connect(pageUpBtn, &QToolButton::clicked, this, &MainWindow::pageUp);
    connect(pageDownBtn, &QToolButton::clicked, this, &MainWindow::pageDown);

    auto *centerBox = new QWidget(bottomTaskbar);
    auto *centerLayout = new QHBoxLayout(centerBox);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(16);
    centerLayout->addWidget(pageUpBtn);
    centerLayout->addWidget(pageDownBtn);

    m_settingsBtn = new QToolButton(bottomTaskbar);
    m_settingsBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/Setting.svg")));
    m_settingsBtn->setText(tr("Налаштування"));
    m_settingsBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_settingsBtn->setIconSize(QSize(40, 40));
    m_settingsBtn->setFixedSize(120, 88);
    m_settingsBtn->setStyle(new CustomToolButtonStyle(5, 0));
    m_settingsBtn->setVisible(false);

    // Та сама причина, що й для кнопок модулів вище: кнопка налаштувань
    // ховається/показується залежно від стану GPIO15, і без цього
    // rightBox теж би змінював ширину й зміщував стрілки гортання.
    QSizePolicy spSettings = m_settingsBtn->sizePolicy();
    spSettings.setRetainSizeWhenHidden(true);
    m_settingsBtn->setSizePolicy(spSettings);

    connect(m_settingsBtn, &QToolButton::clicked, this, [this]() {
        auto *settingsWin = new SettingsWindow(this);
        connect(settingsWin, &SettingsWindow::settingsApplied,
                this, &MainWindow::onSettingsApplied);

        const QRect mainGeom = this->frameGeometry();
        const QSize winSize = settingsWin->sizeHint().isValid()
                                  ? settingsWin->sizeHint()
                                  : settingsWin->size();
        settingsWin->move(
            mainGeom.center().x() - winSize.width() / 2,
            mainGeom.center().y() - winSize.height() / 2);
        settingsWin->show();
    });

    auto *rightBox = new QWidget(bottomTaskbar);
    auto *rightLayout = new QHBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_settingsBtn);

    barLayout->addWidget(leftBox, 0, Qt::AlignLeft | Qt::AlignVCenter);
    barLayout->addStretch(1);
    barLayout->addWidget(centerBox, 0, Qt::AlignCenter);
    barLayout->addStretch(1);
    barLayout->addWidget(rightBox, 0, Qt::AlignRight | Qt::AlignVCenter);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      device(new RP2040Device(this)),
      coinLogic(new Pulses(this)),
      listener(new GpioListener(this))
{
    setWindowTitle(tr("Tach Coffee & Snack"));
    setStyleSheet(QStringLiteral("QMainWindow { background: #e8eaed; }"));

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 0);
    mainLayout->setSpacing(6);

    balanceLabel = new QLabel(tr("Баланс: ₴0.00"), central);
    balanceLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 600; color: #1a3a52; padding-right: 8px;"
    ));
    mainLayout->addWidget(balanceLabel, 0, Qt::AlignRight);

    connect(listener, &GpioListener::pulseDetected, coinLogic, &Pulses::handlePulse);
    listener->start();

    billValidator = new BillValidator(QStringLiteral("/dev/serial0"), this);
    connect(billValidator, &BillValidator::billAccepted,
            coinLogic, &Pulses::addKopiyky, Qt::QueuedConnection);
    connect(billValidator, &BillValidator::statusChanged, this,
            [this](const QString &t) { if (statusLabel) statusLabel->setText(t); },
            Qt::QueuedConnection);
    connect(billValidator, &BillValidator::errorOccurred, this,
            [this](const QString &t) { if (statusLabel) statusLabel->setText(t); },
            Qt::QueuedConnection);
    billValidator->start();

    connect(coinLogic, &Pulses::moneyUpdated, this, [this](int balance) {
        if (MachineSettings::instance().freeMode()) {
            balanceLabel->hide();
            return;
        }
        balanceLabel->show();
        const double uah = balance / 100.0;
        balanceLabel->setText(tr("Баланс: ₴%1").arg(QString::number(uah, 'f', 2)));
    });

    categoryStack = new QStackedWidget(central);
    categoryStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(categoryStack, 1);

    statusLabel = new QLabel(central);
    statusLabel->setStyleSheet(QStringLiteral("color: #555; font-size: 12px; padding: 2px 8px;"));
    mainLayout->addWidget(statusLabel, 0);

    setupBottomTaskbar(central);
    mainLayout->addWidget(bottomTaskbar, 0);

    connect(device, &RP2040Device::settingsButtonChanged,
            this, [this](bool pressed) {
                if (m_settingsBtn)
                    m_settingsBtn->setVisible(pressed);
            }, Qt::QueuedConnection);

    // Коли плата відключається (фізично висмикнули кабель, помилка порту
    // тощо) — повідомляємо в статус-рядок. Сам перезапуск пошуку нічого
    // спеціального тут не потребує: m_deviceScanTimer і так працює постійно
    // у фоні й сам підхопить плату, щойно вона знову з'явиться в системі.
    connect(device, &RP2040Device::disconnected, this, [this]() {
        if (statusLabel)
            statusLabel->setText(tr("Плата розширення відключена — очікую підключення…"));
    }, Qt::QueuedConnection);

    rebuildAllCategoryPages();

    // Раніше плату шукали один-єдиний раз тут-таки в конструкторі: якщо її
    // не було на старті (або кабель пізніше висмикнули й встромили назад),
    // додаток про неї більше не дізнавався. Тепер замість одноразового
    // пошуку — таймер, що раз на 2 секунди перевіряє наявність плати й сам
    // підключається/перепідключається, коли вона з'являється.
    m_deviceScanTimer = new QTimer(this);
    connect(m_deviceScanTimer, &QTimer::timeout, this, &MainWindow::tryConnectDevice);
    m_deviceScanTimer->start(2000);
    tryConnectDevice();   // спробувати одразу, не чекаючи першого тіку таймера

    applyMachineSettings();
}

void MainWindow::tryConnectDevice()
{
    if (device->isConnected())
        return;   // вже підключені — нема чого шукати

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == 0x2e8a) {
            if (device->connect(info.portName(), 115200)) {
                statusLabel->setText(tr("Плата розширення: %1").arg(info.portName()));
                QTimer::singleShot(400, this, [this]() {
                    device->requestSettingsButtonState();
                });
            }
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    if (listener) {
        listener->requestInterruption();
        listener->wait();
    }
    if (billValidator) {
        billValidator->requestInterruption();
        billValidator->wait(2000);
    }
}

QWidget *MainWindow::buildCategoryGridPage(const QVector<ProductItem> &items,
                                           CategoryGridState &state)
{
    state.items = items;
    state.currentPage = 0;
    state.cells.clear();

    auto *page = new QWidget();
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(4, 4, 4, 4);
    pageLayout->setSpacing(0);

    auto *gridHost = new QWidget(page);
    gridHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *grid = new QGridLayout(gridHost);
    grid->setSpacing(12);
    grid->setContentsMargins(0, 0, 0, 0);

    for (int slot = 0; slot < itemsPerPage; ++slot) {
        GridSlot gs;

        gs.iconBtn = new QToolButton(gridHost);
        gs.iconBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        gs.iconBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        gs.iconBtn->setMinimumSize(120, 90);
        gs.iconBtn->setIconSize(QSize(220, 160));
        gs.iconBtn->setStyleSheet(QStringLiteral(
            "QToolButton { background: white; border: 1px solid #c5cdd8; border-radius: 8px; }"
            "QToolButton:pressed { background: #eef2f7; }"
            "QToolButton:disabled { background: #f0f0f0; border-color: #ddd; }"
        ));

        gs.nameLabel = new QLabel(gridHost);
        gs.nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        gs.nameLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));

        gs.priceLabel = new QLabel(gridHost);
        gs.priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        gs.priceLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #2b6cb0;"));

        auto *infoWidget = new QWidget(gridHost);
        infoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto *infoLayout = new QHBoxLayout(infoWidget);
        infoLayout->setContentsMargins(4, 0, 4, 0);
        infoLayout->setSpacing(5);
        infoLayout->addWidget(gs.nameLabel);
        infoLayout->addWidget(gs.priceLabel);

        gs.cell = new QWidget(gridHost);
        gs.cell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto *cellLayout = new QVBoxLayout(gs.cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(4);
        cellLayout->addWidget(gs.iconBtn, 1);
        cellLayout->addWidget(infoWidget, 0);

        const int row = slot / gridColumns;
        const int col = slot % gridColumns;
        grid->addWidget(gs.cell, row, col);

        state.cells.append(gs);
    }

    for (int c = 0; c < gridColumns; ++c)
        grid->setColumnStretch(c, 1);
    const int rows = (itemsPerPage + gridColumns - 1) / gridColumns;
    for (int r = 0; r < rows; ++r)
        grid->setRowStretch(r, 1);

    pageLayout->addWidget(gridHost, 1);
    state.page = page;
    return page;
}

void MainWindow::showGridPage(ProductCategory category)
{
    CategoryGridState &state = gridState(category);
    const int total = state.items.size();
    const int start = state.currentPage * itemsPerPage;

    for (int slot = 0; slot < state.cells.size(); ++slot) {
        const int index = start + slot;
        GridSlot &gs = state.cells[slot];

        if (index < total) {
            const ProductItem &item = state.items[index];
            gs.cell->show();
            gs.iconBtn->setIcon(QIcon(item.iconPath));
            gs.nameLabel->setText(item.name);
            gs.priceLabel->setText(item.priceText);

            gs.iconBtn->disconnect();
            connect(gs.iconBtn, &QToolButton::clicked, this, [this, item]() {
                handleProductClick(item);
            });
        } else {
            gs.cell->hide();
        }
    }
}

void MainWindow::pageUp()
{
    CategoryGridState &state = gridState(currentCategory);
    if (state.currentPage > 0) {
        --state.currentPage;
        showGridPage(currentCategory);
    }
}

void MainWindow::pageDown()
{
    CategoryGridState &state = gridState(currentCategory);
    const int total = state.items.size();
    const int maxPage = total > 0 ? (total - 1) / itemsPerPage : 0;
    if (state.currentPage < maxPage) {
        ++state.currentPage;
        showGridPage(currentCategory);
    }
}

void MainWindow::rebuildAllCategoryPages()
{
    while (categoryStack->count() > 0) {
        QWidget *w = categoryStack->widget(0);
        categoryStack->removeWidget(w);
        w->deleteLater();
    }

    MachineSettings &cfg = MachineSettings::instance();
    m_coffeeGrid = CategoryGridState();
    m_waterGrid = CategoryGridState();
    m_snacksGrid = CategoryGridState();

    categoryStack->addWidget(buildCategoryGridPage(cfg.coffeeItems(), m_coffeeGrid));
    categoryStack->addWidget(buildCategoryGridPage(cfg.waterItems(), m_waterGrid));
    categoryStack->addWidget(buildCategoryGridPage(cfg.snackItems(), m_snacksGrid));
}

void MainWindow::handleProductClick(const ProductItem &item)
{

    if (m_dispensing) {
        statusLabel->setText(tr("Зачекайте, триває видача попереднього товару…"));
        return;
    }

    MachineSettings &cfg = MachineSettings::instance();

    if (!cfg.freeMode()) {
        Pay_Price payDialog(item.name, item.priceText, QIcon(item.iconPath), this);
        connect(coinLogic, &Pulses::moneyUpdated, &payDialog, [&payDialog](int newBalance) {
            payDialog.updateBalance(newBalance);
        });
        payDialog.updateBalance(coinLogic->getBalance());

        if (payDialog.exec() != QDialog::Accepted)
            return;
        if (!coinLogic->deduct(payDialog.getPrice()))
            return;
    }

    m_dispensing = true;
    setProductsEnabled(false);

    statusLabel->setText(tr("Видача: %1…").arg(item.name));
    const int holdMs = item.holdMs;  // тривалість виходу саме цього товару
    const auto result = device->sendDispenseCommand(item.gpioChannel, holdMs);

    switch (result) {
    case RP2040Device::DispenseResult::Success: {
        statusLabel->setText(tr("Видача: %1…").arg(item.name));
        // ACK означає «команду прийнято». Вихід фізично працює ще holdMs —
        // тримаємо замок весь цей час, щоб не дозволити паралельну видачу.
        // Чекаємо без «заморозки» інтерфейсу (балансу, годинника тощо).
        QEventLoop wait;
        QTimer::singleShot(holdMs, &wait, &QEventLoop::quit);
        wait.exec();
        statusLabel->setText(tr("Готово: %1").arg(item.name));
        break;
    }
    case RP2040Device::DispenseResult::Error:
        statusLabel->setText(tr("Помилка плати розширення"));
        QMessageBox::warning(this, tr("Помилка"),
                             tr("Плата розширення повідомила про помилку видачі."));
        break;
    case RP2040Device::DispenseResult::PortClosed:
        statusLabel->setText(tr("Немає зв'язку з платою"));
        QMessageBox::warning(this, tr("Зв'язок"),
                             tr("Послідовний порт до RP2040 не відкритий."));
        break;
    case RP2040Device::DispenseResult::Timeout:
        statusLabel->setText(tr("Немає підтвердження від плати"));
        QMessageBox::warning(this, tr("Таймаут"),
                             tr("Плата не надіслала підтвердження."));
        break;
    }


    m_dispensing = false;
    setProductsEnabled(true);
}

void MainWindow::setProductsEnabled(bool enabled)
{

    CategoryGridState *grids[] = { &m_coffeeGrid, &m_waterGrid, &m_snacksGrid };
    for (CategoryGridState *g : grids) {
        for (GridSlot &gs : g->cells) {
            if (gs.iconBtn)
                gs.iconBtn->setEnabled(enabled);
        }
    }
}

void MainWindow::updateProductIconSizes()
{

    CategoryGridState *grids[] = { &m_coffeeGrid, &m_waterGrid, &m_snacksGrid };
    for (CategoryGridState *g : grids) {
        for (GridSlot &gs : g->cells) {
            if (!gs.iconBtn)
                continue;
            const QSize btn = gs.iconBtn->size();
            const int side = qMax(48, int(qMin(btn.width(), btn.height()) * 0.8));
            gs.iconBtn->setIconSize(QSize(side, side));
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateProductIconSizes();
}

void MainWindow::switchCategory(ProductCategory category)
{
    if (!MachineSettings::instance().moduleEnabled(category))
        return;

    currentCategory = category;
    categoryStack->setCurrentIndex(static_cast<int>(category));
    showGridPage(category);

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
        if (b)
            b->setChecked(b == active);
    }
}

void MainWindow::updateModuleButtons()
{
    MachineSettings &cfg = MachineSettings::instance();

    const bool coffeeOn = cfg.moduleEnabled(ProductCategory::Coffee);
    const bool waterOn  = cfg.moduleEnabled(ProductCategory::Water);
    const bool snacksOn = cfg.moduleEnabled(ProductCategory::Snacks);
    const int enabledCount = (coffeeOn ? 1 : 0) + (waterOn ? 1 : 0) + (snacksOn ? 1 : 0);

    const bool showSwitchers = (enabledCount > 1);
    if (coffeeBtn) coffeeBtn->setVisible(showSwitchers && coffeeOn);
    if (waterBtn)  waterBtn->setVisible(showSwitchers && waterOn);
    if (snackBtn)  snackBtn->setVisible(showSwitchers && snacksOn);

    if (cfg.freeMode())
        balanceLabel->hide();
    else
        balanceLabel->show();

    ProductCategory first = ProductCategory::Snacks;
    if (coffeeOn)
        first = ProductCategory::Coffee;
    else if (waterOn)
        first = ProductCategory::Water;
    else if (snacksOn)
        first = ProductCategory::Snacks;

    const bool anyModule = coffeeOn || waterOn || snacksOn;

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
    rebuildAllCategoryPages();
    applyMachineSettings();
}
