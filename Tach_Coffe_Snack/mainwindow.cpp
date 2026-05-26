#include "mainwindow.h"
#include <QGridLayout>
#include <QPushButton>
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
#include "pulses.h"
#include "pay_price.h"
#include <QDialog>
#include "gpio_listener.h"
#include <gpiod.h>
#include <QSerialPortInfo>

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
            const QStyleOptionToolButton *toolbutton =
                qstyleoption_cast<const QStyleOptionToolButton *>(option);

            if (toolbutton) {
                QStyleOptionToolButton tbOpt(*toolbutton);
                tbOpt.iconSize = tbOpt.iconSize;
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      device(new RP2040Device(this)),
      coinLogic(new Pulses(this)),
      listener(new GpioListener(this)),
      currentPage(0)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    grid = new QGridLayout();
    grid->setSpacing(10);

    balanceLabel = new QLabel("Баланс: ₴0.00", this);
    mainLayout->addWidget(balanceLabel, 0, Qt::AlignRight);

    connect(listener, &GpioListener::pulseDetected, coinLogic, &Pulses::handlePulse);
    listener->start();

    connect(coinLogic, &Pulses::moneyUpdated, this, [this](int balance){
        double balanceInUAH = balance / 100.0;
        balanceLabel->setText(QString("Баланс: ₴%1").arg(QString::number(balanceInUAH, 'f', 2)));
    });


    // цикл для створення товарів
    for (int i = 0; i < 50; ++i) {
        QToolButton *btn = new QToolButton();
        btn->setIcon(QIcon(":/Icon/Icon/snack.svg"));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(520, 380));
        btn->setFixedSize(520, 380);

        QString snackName = QString("Снек %1").arg(i+1);
        QString snackPrice = "₴20";

        QLabel *nameLabel = new QLabel(snackName);
        nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        QLabel *priceLabel = new QLabel(snackPrice);
        priceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        itemButtons.append(btn);
        priceLabels.append(priceLabel);

        QWidget *infoWidget = new QWidget();
        infoWidget->setFixedWidth(btn->width());
        QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);
        infoLayout->setContentsMargins(0,0,0,0);
        infoLayout->setSpacing(5);
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(priceLabel);

        QWidget *cellWidget = new QWidget();
        QVBoxLayout *cellLayout = new QVBoxLayout(cellWidget);
        cellLayout->setContentsMargins(0,0,0,0);
        cellLayout->setSpacing(2);
        cellLayout->addWidget(btn, 0, Qt::AlignCenter);
        cellLayout->addWidget(infoWidget, 0, Qt::AlignCenter);
        cellWidget->setFixedWidth(btn->width());

        int row = i / 3;
        int col = i % 3;
        grid->addWidget(cellWidget, row, col);
        cellWidgets.append(cellWidget);

        // обробка кліку по товару
        connect(btn, &QToolButton::clicked, this, [this, snackName, snackPrice, btn, i]() {
            Pay_Price payDialog(snackName, snackPrice, btn->icon(), this);
            payDialog.setWindowTitle("Оплата");

            connect(coinLogic, &Pulses::moneyUpdated, &payDialog, [&payDialog](int newBalance){
                payDialog.updateBalance(newBalance);
            });

            payDialog.exec();
            if (payDialog.result() == QDialog::Accepted) {
                if (coinLogic->deduct(payDialog.getPrice())) {
                    qDebug() << "Оплата підтверджена для:" << snackName << snackPrice;
                    coinLogic->resetBalance();

                    QByteArray packet;
                    packet.append(char(0xAA));
                    packet.append(char(i+1));
                    packet.append(char(0x55));

                    device->sendMessage(packet);
                }
            }

        });
    }

    // функція показу сторінки
    auto showPage = [this](int page){
        int start = page * itemsPerPage;
        int end = qMin(start + itemsPerPage, itemButtons.size());

        for (int i = 0; i < cellWidgets.size(); ++i) {
            bool visible = (i >= start && i < end);
            cellWidgets[i]->setVisible(visible);
        }
    };

    // кнопки навігації
    QToolButton *heightBtn = new QToolButton();
    QToolButton *bottomBtn = new QToolButton();

    connect(heightBtn, &QToolButton::clicked, this, [this, showPage](){
        if (currentPage > 0) {
            currentPage--;
            showPage(currentPage);
        }
    });

    connect(bottomBtn, &QToolButton::clicked, this, [this, showPage](){
        if ((currentPage+1) * itemsPerPage < itemButtons.size()) {
            currentPage++;
            showPage(currentPage);
        }
    });

    showPage(currentPage);

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier() == 0x2e8a) { // VID RP2040
            QString portName = info.portName();
            if (device->connect(portName, 115200)) {
                device->sendMessage("Hello RP2040\n");
            } else {
                qDebug() << "Не вдалося відкрити порт:" << portName;
            }
            break;
        }
    }


    // Нижня панель
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(20);
    bottomLayout->setContentsMargins(0,0,0,0);

    // Кнопка "Кава"
    QToolButton *coffeeBtn = new QToolButton();
    coffeeBtn->setIcon(QIcon(":/Icon/Icon/coffee.svg"));
    coffeeBtn->setText("Кава");
    coffeeBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    coffeeBtn->setIconSize(QSize(48, 48));
    coffeeBtn->setFixedSize(120, 100);
    coffeeBtn->setStyle(new CustomToolButtonStyle(7, 0));

    // Кнопка "Снеки"
    QToolButton *snackBtn = new QToolButton();
    snackBtn->setIcon(QIcon(":/Icon/Icon/snack.svg"));
    snackBtn->setText("Снеки");
    snackBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    snackBtn->setIconSize(QSize(48, 48));
    snackBtn->setFixedSize(120, 100);
    snackBtn->setStyle(new CustomToolButtonStyle(7, 0));


    // Кнопка "Вода"
    QToolButton *waterBtn = new QToolButton();
    waterBtn->setIcon(QIcon(":/Icon/Icon/water.svg"));
    waterBtn->setText("Вода");
    waterBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    waterBtn->setIconSize(QSize(48, 48));
    waterBtn->setFixedSize(120, 100);
    waterBtn->setStyle(new CustomToolButtonStyle(7, 0));

    // Кнопка "Height" (листати вгору)
    heightBtn->setIcon(QIcon(":/Icon/Icon/height.svg"));
    heightBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    heightBtn->setIconSize(QSize(162, 162));
    heightBtn->setFixedSize(100, 100);
    heightBtn->setStyle(new CustomToolButtonStyle(6, 0));

    // Кнопка "Bottom" (листати вниз)
    bottomBtn->setIcon(QIcon(":/Icon/Icon/bottom.svg"));
    bottomBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    bottomBtn->setIconSize(QSize(162, 162));
    bottomBtn->setFixedSize(100, 100);
    bottomBtn->setStyle(new CustomToolButtonStyle(6, 0));

    // Кнопка "Допомога"
    QToolButton *helpBtn = new QToolButton();
    helpBtn->setIcon(QIcon(":/Icon/Icon/Help.svg"));
    helpBtn->setText("Допомога");
    helpBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    helpBtn->setIconSize(QSize(48, 48));
    helpBtn->setFixedSize(140, 100);
    helpBtn->setStyle(new CustomToolButtonStyle(7, 0));

    // Кнопка "Налаштування"
    QToolButton *settingsBtn = new QToolButton();
    settingsBtn->setIcon(QIcon(":/Icon/Icon/Setting.svg"));
    settingsBtn->setText("Налаштування");
    settingsBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    settingsBtn->setIconSize(QSize(48, 48));
    settingsBtn->setFixedSize(140, 100);
    settingsBtn->setStyle(new CustomToolButtonStyle(7, 0));

    connect(settingsBtn, &QToolButton::clicked, this, [this]() {
        SettingsWindow *settingsWin = new SettingsWindow(this);
        settingsWin->show();
    });

    // Лівий блок
    QHBoxLayout *leftLayout = new QHBoxLayout();
    leftLayout->setSpacing(10);
    leftLayout->addWidget(coffeeBtn);
    leftLayout->addWidget(snackBtn);
    leftLayout->addWidget(waterBtn);

    // Центральний блок (навігація)
    QHBoxLayout *navLayout = new QHBoxLayout();
    navLayout->setSpacing(10);
    navLayout->addWidget(heightBtn);
    navLayout->addWidget(bottomBtn);

    // Правий блок
    QHBoxLayout *rightLayout = new QHBoxLayout();
    rightLayout->setSpacing(10);
    rightLayout->addWidget(helpBtn);
    rightLayout->addWidget(settingsBtn);

    // Формуємо головний layout
    bottomLayout->addLayout(leftLayout, 0);
    bottomLayout->addStretch();
    bottomLayout->addLayout(navLayout, 0);
    bottomLayout->addStretch();
    bottomLayout->addLayout(rightLayout, 0);

    // Вирівнювання блоків
    bottomLayout->setAlignment(leftLayout, Qt::AlignLeft);
    bottomLayout->setAlignment(navLayout, Qt::AlignCenter);
    bottomLayout->setAlignment(rightLayout, Qt::AlignRight);

    // Додаємо нижню панель у головний layout
    mainLayout->addLayout(grid);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(bottomLayout);



}

MainWindow::~MainWindow()
{
    if (listener) {
        listener->requestInterruption();
        listener->wait(); // чекаємо завершення потоку
        qDebug() << "GPIO listener stopped";
    }
}
