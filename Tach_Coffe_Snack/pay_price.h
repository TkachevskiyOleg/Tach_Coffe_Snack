// pay_price.h
#pragma once
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>

class Pay_Price : public QDialog {
    Q_OBJECT
public:
    explicit Pay_Price(const QString &name,
                       const QString &price,
                       const QIcon &icon,
                       QWidget *parent = nullptr);

    void updateBalance(int newBalance);   // метод для оновлення балансу
    int getPrice() const { return priceValue; } // геттер для числової ціни

private:
    QLabel *balanceLabel;   // показує баланс
    QPushButton *payBtn;    // кнопка "Оплатити"
    int priceValue;         // числове значення ціни товару
};
