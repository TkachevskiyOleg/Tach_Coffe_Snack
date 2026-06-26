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
#include <QSizePolicy>

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
    bottomTaskbar->setStyleSheet(QStringLiteral(
        "QFrame#bottomTaskbar {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #3d5a80, stop:1 #293241);"
        "  border-top: 2px solid #1d2d44;"
        "}"
        "QFrame#bottomTaskbar QToolButton {"
        "  color: white;"
        "  background: transparent;"
        "  border: none;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
        "QFrame#bottomTaskbar QToolButton:hover {"
        "  background: rgba(255,255,255,0.12);"
        "  border-radius: 6px;"
        "}"
        "QFrame#bottomTaskbar QToolButton:checked {"
        "  background: rgba(255,255,255,0.22);"
        "  border-radius: 6px;"
        "}"
    ));

    auto *barLayout = new QHBoxLayout(bottomTaskbar);
    barLayout->setContentsMargins(16, 6, 16, 6);
    barLayout->setSpacing(12);

    coffeeBtn = makeModuleButton(tr("020"), QStringLiteral(":/Icon/Icon/coffee.svg"), bottomTaskbar);
    snackBtn = makeModuleButton(tr("!=5:8"), QStringLiteral(":/Icon/Icon/snack.svg"), bottomTaskbar);
    waterBtn = makeModuleButton(tr(">40"), QStringLiteral(":/Icon/Icon/water.svg"), bottomTaskbar);

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

    auto *settingsBtn = new QToolButton(bottomTaskbar);
    settingsBtn->setIcon(QIcon(QStringLiteral(":/Icon/Icon/Setting.svg")));
    settingsBtn->setText(tr("0;0HBC20==O"));
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    settingsBtn->setIconSize(QSize(40, 40));
    settingsBtn->setFixedSize(120, 88);
    settingsBtn->setStyle(new CustomToolButtonStyle(5, 0));
    connect(settingsBtn, &QToolButton::clicked, this, [this]() {
        auto *settingsWin = new SettingsWindow(this);
        connect(settingsWin, &SettingsWindow::settingsApplied,
                this, &MainWindow::onSettingsApplied);
        settingsWin->show();
    });

    auto *rightBox = new QWidget(bottomTaskbar);
    auto *rightLayout = new QHBoxLayout(rightBox);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(settingsBtn);

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

    balanceLabel = new QLabel(tr("0;0=A: �0.00"), central);
    balanceLabel->setStyleSheet(QStringLiteral(
        "font-size: 20px; font-weight: 600; color: #1a3a52; padding-right: 8px;"
    ));
    mainLayout->addWidget(balanceLabel, 0, Qt::AlignRight);

    connect(listener, &GpioListener::pulseDetected, coinLogic, &Pulses::handlePulse);
    listener->start();

    connect(coinLogic, &Pulses::moneyUpdated, this, [this](int balance) {
        if (MachineSettings::instance().freeMode()) {
            balanceLabel->hide();
            return;
        }
        balanceLabel->show();
        const double uah = balance / 100.0;
        balanceLabel->setText(tr("0;0=A: �%1").arg(QString::number(uah, 'f', 2)));
    });

    categoryStack = new QStackedWidget(central);
    categoryStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(categoryStack, 1);

    statusLabel = new QLabel(central);
    statusLabel->setStyleSheet(QStringLiteral("color: #555; font-size: 12px; padding: 2px 8px;"));
    mainLayout->addWidget(statusLabel, 0);

    setupBottomTaskbar(central);
    mainLayout->addWidget(bottomTaskbar, 0);

    rebuildAllCategoryPages();

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == 0x2e8a) {
            if (device->connect(info.portName(), 115200))
                statusLabel->setText(tr(";0B0 @>7H8@5==O: %1").arg(info.portName()));
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

QWidget *MainWindow::buildCategoryGridPage(const QVector<ProductItem> &items,
                                           CategoryGridState &state)
{
    state.items = items;
    state.currentPage = 0;
    state.slots.clear();

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
        gs.iconBtn->setIconSize(QSize(300, 220));
        gs.iconBtn->setFixedSize(300, 220);
        gs.iconBtn->setStyleSheet(QStringLiteral(
            "QToolButton { background: white; border: 1px solid #c5cdd8; border-radius: 8px; }"
            "QToolButton:pressed { background: #eef2f7; }"
        ));

        gs.nameLabel = new QLabel(gridHost);
        gs.nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        gs.nameLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));

        gs.priceLabel = new QLabel(gridHost);
        gs.priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        gs.priceLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #2b6cb0;"));

        auto *infoWidget = new QWidget(gridHost);
        infoWidget->setFixedWidth(gs.iconBtn->width());
        auto *infoLayout = new QHBoxLayout(infoWidget);
        infoLayout->setContentsMargins(4, 0, 4, 0);
        infoLayout->setSpacing(5);
        infoLayout->addWidget(gs.nameLabel);
        infoLayout->addWidget(gs.priceLabel);

        gs.cell = new QWidget(gridHost);
        auto *cellLayout = new QVBoxLayout(gs.cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(4);
        cellLayout->addWidget(gs.iconBtn, 0, Qt::AlignCenter);
        cellLayout->addWidget(infoWidget, 0, Qt::AlignCenter);

        const int row = slot / gridColumns;
        const int col = slot % gridColumns;
        grid->addWidget(gs.cell, row, col, Qt::AlignCenter);

        state.slots.append(gs);
    }

    pageLayout->addWidget(gridHost, 1, Qt::AlignTop);
    state.page = page;
    return page;
}

void MainWindow::showGridPage(ProductCategory category)
{
    CategoryGridState &state = gridState(category);
    const int total = state.items.size();
    const int start = state.currentPage * itemsPerPage;

    for (int slot = 0; slot < state.slots.size(); ++slot) {
        const int index = start + slot;
        GridSlot &gs = state.slots[slot];

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
        coinLogic->resetBalance();
    }

    statusLabel->setText(tr("840G0: %1&").arg(item.name));
    const auto result = device->sendDispenseCommand(item.gpioChannel, cfg.buttonHoldMs());

    switch (result) {
    case RP2040Device::DispenseResult::Success:
        statusLabel->setText(tr(">B>2>: %1").arg(item.name));
        break;
    case RP2040Device::DispenseResult::Error:
        statusLabel->setText(tr("><8;:0 ?;0B8 @>7H8@5==O"));
        QMessageBox::warning(this, tr("><8;:0"),
                             tr(";0B0 @>7H8@5==O ?>2V4><8;0 ?@> ?><8;:C 2840GV."));
        break;
    case RP2040Device::DispenseResult::PortClosed:
        statusLabel->setText(tr("5<0T 72O7:C 7 ?;0B>N"));
        QMessageBox::warning(this, tr("2O7>:"),
                             tr(">A;V4>2=89 ?>@B 4> RP2040 =5 2V4:@8B89."));
        break;
    case RP2040Device::DispenseResult::Timeout:
        statusLabel->setText(tr("5<0T ?V4B25@465==O 2V4 ?;0B8"));
        QMessageBox::warning(this, tr(""09<0CB"),
                             tr(";0B0 =5 =04VA;0;0 ?V4B25@465==O."));
        break;
    }
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
    if (coffeeBtn) coffeeBtn->setVisible(cfg.moduleEnabled(ProductCategory::Coffee));
    if (waterBtn) waterBtn->setVisible(cfg.moduleEnabled(ProductCategory::Water));
    if (snackBtn) snackBtn->setVisible(cfg.moduleEnabled(ProductCategory::Snacks));

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
        statusLabel->setText(tr("#2V<:=VBL E>G0 1 >48= <>4C;L C =0;0HBC20==OE"));
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
