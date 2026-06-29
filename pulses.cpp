#include "pulses.h"
#include <QDebug>

Pulses::Pulses(QObject *parent)
    : QObject(parent),
      pulseCount(0),
      balance(0),
      deviceLocked(false)
{
}

void Pulses::handlePulse(int line) {
    if (deviceLocked) return;

    pulseCount++;
    qDebug() << "handlePulse: GPIO" << line << "pulseCount =" << pulseCount;

    static std::map<int,int> coinMap = {
        {1, 50},     // 50 коп
        {2, 100},    // 1 грн
        {4, 200},    // 2 грн
        {20, 500},   // 5 грн
        {40, 1000}   // 10 грн
    };

    // Якщо кількість імпульсів співпала з номіналом — зараховуємо монету
    auto it = coinMap.find(pulseCount);
    if (it != coinMap.end()) {
        balance += it->second;
        qDebug() << "Розпізнано як монету на" << it->second/100.0 << "грн. Баланс:" << balance;
        emit moneyUpdated(balance);
        pulseCount = 0; // обнуляємо після повної серії
    }
}

bool Pulses::deduct(int price) {
    if (balance >= price) {
        balance -= price;
        qDebug() << "Списано:" << price << "копійок. Новий баланс:" << balance;
        emit moneyUpdated(balance);
        return true;
    }
    return false;
}

void Pulses::resetBalance() {
    balance = 0;
    qDebug() << "Баланс обнулено";
    emit moneyUpdated(balance);
}

void Pulses::addKopiyky(int kopiyky) {
    if (kopiyky <= 0)
        return;
    balance += kopiyky;
    qDebug() << "Купюрник додав:" << kopiyky << "копійок. Баланс:" << balance;
    emit moneyUpdated(balance);
}
