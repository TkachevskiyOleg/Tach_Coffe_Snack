#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVector>

class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onSave();
    void onCancel();

private:
    void buildGpioRows();

    QCheckBox *freeModeCheck = nullptr;
    QCheckBox *coffeeCheck = nullptr;
    QCheckBox *waterCheck = nullptr;
    QCheckBox *snacksCheck = nullptr;
    QSpinBox *holdMsSpin = nullptr;
    QLineEdit *snackNameEdit = nullptr;
    QDoubleSpinBox *snackPriceSpin = nullptr;
    QSpinBox *snackGpioSpin = nullptr;

    QVector<QSpinBox *> coffeeGpioSpins;
    QVector<QSpinBox *> waterGpioSpins;
    QWidget *gpioSection = nullptr;
};
