#include "machinesettings.h"
#include <QSettings>
#include <QtGlobal>

MachineSettings &MachineSettings::instance()
{
    static MachineSettings inst;
    return inst;
}

MachineSettings::MachineSettings(QObject *parent)
    : QObject(parent)
{
    initDefaults();
    load();
}

void MachineSettings::initDefaults()
{
    m_coffeeItems = {
        {"coffee_espresso", "Еспресо", "₴15", 1500, 1, ":/Icon/Icon/coffee.svg"},
        {"coffee_americano", "Американо", "₴18", 1800, 2, ":/Icon/Icon/coffee.svg"},
        {"coffee_latte", "Лате", "₴22", 2200, 3, ":/Icon/Icon/coffee.svg"},
    };

    m_waterItems = {
        {"water_still", "Вода негазована", "₴10", 1000, 4, ":/Icon/Icon/water.svg"},
        {"water_sparkling", "Вода газована", "₴12", 1200, 5, ":/Icon/Icon/gaz_water2.svg"},
    };

    m_snackItems = {
        {"snack_main", "Снек Класик", "₴20", 2000, 6, ":/Icon/Icon/snack.svg"},
    };
}

bool MachineSettings::moduleEnabled(ProductCategory category) const
{
    switch (category) {
    case ProductCategory::Coffee: return m_coffeeEnabled;
    case ProductCategory::Water: return m_waterEnabled;
    case ProductCategory::Snacks: return m_snacksEnabled;
    }
    return false;
}

void MachineSettings::setModuleEnabled(ProductCategory category, bool enabled)
{
    switch (category) {
    case ProductCategory::Coffee: m_coffeeEnabled = enabled; break;
    case ProductCategory::Water: m_waterEnabled = enabled; break;
    case ProductCategory::Snacks: m_snacksEnabled = enabled; break;
    }
}

void MachineSettings::setFreeMode(bool enabled)
{
    if (m_freeMode == enabled)
        return;
    m_freeMode = enabled;
    emit settingsChanged();
}

void MachineSettings::setButtonHoldMs(int ms)
{
    m_buttonHoldMs = qBound(100, ms, 30000);
}

void MachineSettings::setSnackDisplayName(const QString &name)
{
    if (m_snackItems.isEmpty())
        return;
    m_snackItems[0].name = name.isEmpty() ? QStringLiteral("Снек Класик") : name;
}

void MachineSettings::setSnackGpio(int gpioChannel)
{
    if (m_snackItems.isEmpty())
        return;
    m_snackItems[0].gpioChannel = qBound(1, gpioChannel, 14);
}

void MachineSettings::setSnackPriceKopiyky(int kopiyky)
{
    if (m_snackItems.isEmpty())
        return;
    m_snackItems[0].priceKopiyky = qMax(0, kopiyky);
    m_snackItems[0].priceText = QStringLiteral("₴%1").arg(kopiyky / 100.0, 0, 'f', 2);
}

void MachineSettings::setCoffeeGpio(int index, int gpioChannel)
{
    if (index >= 0 && index < m_coffeeItems.size())
        m_coffeeItems[index].gpioChannel = qBound(1, gpioChannel, 14);
}

void MachineSettings::setWaterGpio(int index, int gpioChannel)
{
    if (index >= 0 && index < m_waterItems.size())
        m_waterItems[index].gpioChannel = qBound(1, gpioChannel, 14);
}

void MachineSettings::load()
{
    QSettings s(QStringLiteral("Tach"), QStringLiteral("CoffeSnack"));
    m_freeMode = s.value(QStringLiteral("freeMode"), false).toBool();
    m_coffeeEnabled = s.value(QStringLiteral("coffeeEnabled"), true).toBool();
    m_waterEnabled = s.value(QStringLiteral("waterEnabled"), true).toBool();
    m_snacksEnabled = s.value(QStringLiteral("snacksEnabled"), true).toBool();
    m_buttonHoldMs = s.value(QStringLiteral("buttonHoldMs"), 1000).toInt();

    setSnackDisplayName(s.value(QStringLiteral("snackName"), m_snackItems.value(0).name).toString());
    setSnackGpio(s.value(QStringLiteral("snackGpio"), m_snackItems.value(0).gpioChannel).toInt());
    setSnackPriceKopiyky(s.value(QStringLiteral("snackPriceKop"), m_snackItems.value(0).priceKopiyky).toInt());

    for (int i = 0; i < m_coffeeItems.size(); ++i) {
        const QString key = QStringLiteral("coffeeGpio_%1").arg(i);
        setCoffeeGpio(i, s.value(key, m_coffeeItems[i].gpioChannel).toInt());
    }
    for (int i = 0; i < m_waterItems.size(); ++i) {
        const QString key = QStringLiteral("waterGpio_%1").arg(i);
        setWaterGpio(i, s.value(key, m_waterItems[i].gpioChannel).toInt());
    }
}

void MachineSettings::save()
{
    QSettings s(QStringLiteral("Tach"), QStringLiteral("CoffeSnack"));
    s.setValue(QStringLiteral("freeMode"), m_freeMode);
    s.setValue(QStringLiteral("coffeeEnabled"), m_coffeeEnabled);
    s.setValue(QStringLiteral("waterEnabled"), m_waterEnabled);
    s.setValue(QStringLiteral("snacksEnabled"), m_snacksEnabled);
    s.setValue(QStringLiteral("buttonHoldMs"), m_buttonHoldMs);
    s.setValue(QStringLiteral("snackName"), m_snackItems.value(0).name);
    s.setValue(QStringLiteral("snackGpio"), m_snackItems.value(0).gpioChannel);
    s.setValue(QStringLiteral("snackPriceKop"), m_snackItems.value(0).priceKopiyky);

    for (int i = 0; i < m_coffeeItems.size(); ++i)
        s.setValue(QStringLiteral("coffeeGpio_%1").arg(i), m_coffeeItems[i].gpioChannel);
    for (int i = 0; i < m_waterItems.size(); ++i)
        s.setValue(QStringLiteral("waterGpio_%1").arg(i), m_waterItems[i].gpioChannel);

    s.sync();
    emit settingsChanged();
}
