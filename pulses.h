#pragma once
#include <QObject>
#include <map>

class Pulses : public QObject {
    Q_OBJECT
public:
    explicit Pulses(QObject *parent = nullptr);

    void handlePulse(int line);
    int getBalance() const { return balance; }
    bool deduct(int price);
    void resetBalance();

    void addKopiyky(int kopiyky);

signals:
    void moneyUpdated(int balance);

private:
    int pulseCount;
    int balance;
    bool deviceLocked;
};
