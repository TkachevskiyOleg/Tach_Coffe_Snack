#include "pay_price.h"
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSize>

Pay_Price::Pay_Price(const QString &name, const QString &price, const QIcon &icon, QWidget *parent)
    : QDialog{parent}
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Іконка товару
    QToolButton *iconBtn = new QToolButton();
    iconBtn->setIcon(icon);
    iconBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    iconBtn->setIconSize(QSize(134, 204));
    iconBtn->setFixedSize(134, 204);

    // Назва та ціна
    QLabel *nameLabel = new QLabel("Товар: " + name);
    QLabel *priceLabel = new QLabel("Ціна: " + price);

    // Зберігаємо числову ціну у копійках
    priceValue = static_cast<int>(price.mid(1).toDouble() * 100);

    // Баланс (початкове значення одразу з копійками)
    balanceLabel = new QLabel("Баланс: ₴0.00");

    // Кнопки
    payBtn = new QPushButton("Оплатити");
    payBtn->setEnabled(false);
    QPushButton *closeBtn = new QPushButton("Закрити");

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(payBtn, &QPushButton::clicked, this, &QDialog::accept);

    // Layout
    layout->addWidget(iconBtn, 0, Qt::AlignCenter);
    layout->addWidget(nameLabel);
    layout->addWidget(priceLabel);
    layout->addWidget(balanceLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(payBtn);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
}

void Pay_Price::updateBalance(int newBalance) {
    // newBalance у копійках → переводимо у гривні
    double balanceInUAH = static_cast<double>(newBalance) / 100.0;

    // Форматуємо з двома знаками після коми
    QString formatted = QString::number(balanceInUAH, 'f', 2);
    balanceLabel->setText("Баланс: ₴" + formatted);

    // Кнопка активна, якщо баланс у копійках >= ціна у копійках
    payBtn->setEnabled(newBalance >= priceValue);
}
