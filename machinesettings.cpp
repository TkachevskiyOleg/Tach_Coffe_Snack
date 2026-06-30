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
    m_coffeeItems.clear();
    for (int i = 0; i < 18; ++i) {
        m_coffeeItems.append({
            QStringLiteral("coffee_%1").arg(i + 1),
            QStringLiteral("Кава %1").arg(i + 1),
            QStringLiteral("₴%1").arg(15 + (i % 5) * 2),
            (15 + (i % 5) * 2) * 100,
            (i % 14) + 1,
            QStringLiteral(":/Icon/Icon/coffee.svg"),
            false,
        });
    }

    m_waterItems.clear();
    for (int i = 0; i < 18; ++i) {
        const bool sparkling = (i % 2 == 1);
        m_waterItems.append({
            QStringLiteral("water_%1").arg(i + 1),
            QStringLiteral("Вода %1").arg(i + 1),
            QStringLiteral("₴%1").arg(10 + (i % 4)),
            (10 + (i % 4)) * 100,
            ((i + 3) % 14) + 1,
            sparkling ? QStringLiteral(":/Icon/Icon/gaz_water2.svg")
                      : QStringLiteral(":/Icon/Icon/water.svg"),
            sparkling,
        });
    }

    m_snackItems.clear();
    for (int i = 0; i < kOutputCount; ++i) {
        m_snackItems.append({
            QStringLiteral("snack_%1").arg(i + 1),
            QStringLiteral("Снек %1").arg(i + 1),
            QStringLiteral("₴20"),
            2000,
            (i % kOutputCount) + 1,
            QStringLiteral(":/Icon/Icon/snack.svg"),
            false,
        });
    }
}

const char *MachineSettings::categoryKey(ProductCategory category)
{
    switch (category) {
    case ProductCategory::Coffee: return "coffee";
    case ProductCategory::Water:  return "water";
    case ProductCategory::Snacks: return "snack";
    }
    return "snack";
}

QString MachineSettings::priceTextFromKopiyky(int kopiyky)
{
    return QStringLiteral("₴%1").arg(kopiyky / 100.0, 0, 'f', 2);
}

QVector<ProductItem> &MachineSettings::itemsMutable(ProductCategory category)
{
    switch (category) {
    case ProductCategory::Coffee: return m_coffeeItems;
    case ProductCategory::Water:  return m_waterItems;
    case ProductCategory::Snacks: return m_snackItems;
    }
    return m_snackItems;
}

const QVector<ProductItem> &MachineSettings::items(ProductCategory category) const
{
    switch (category) {
    case ProductCategory::Coffee: return m_coffeeItems;
    case ProductCategory::Water:  return m_waterItems;
    case ProductCategory::Snacks: return m_snackItems;
    }
    return m_snackItems;
}

int MachineSettings::itemCount(ProductCategory category) const
{
    return items(category).size();
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

void MachineSettings::setItemName(ProductCategory category, int index, const QString &name)
{
    QVector<ProductItem> &v = itemsMutable(category);
    if (index < 0 || index >= v.size())
        return;
    v[index].name = name.isEmpty()
        ? QStringLiteral("%1 %2").arg(QString::fromUtf8(categoryKey(category))).arg(index + 1)
        : name;
}

void MachineSettings::setItemGpio(ProductCategory category, int index, int gpioChannel)
{
    QVector<ProductItem> &v = itemsMutable(category);
    if (index < 0 || index >= v.size())
        return;
    v[index].gpioChannel = qBound(1, gpioChannel, 14);
}

void MachineSettings::setItemPriceKopiyky(ProductCategory category, int index, int kopiyky)
{
    QVector<ProductItem> &v = itemsMutable(category);
    if (index < 0 || index >= v.size())
        return;
    v[index].priceKopiyky = qMax(0, kopiyky);
    v[index].priceText = priceTextFromKopiyky(v[index].priceKopiyky);
}

void MachineSettings::setItemSparkling(ProductCategory category, int index, bool sparkling)
{
    QVector<ProductItem> &v = itemsMutable(category);
    if (index < 0 || index >= v.size())
        return;
    v[index].sparkling = sparkling;
    // оновлюємо іконку для води відповідно до типу
    if (category == ProductCategory::Water) {
        v[index].iconPath = sparkling
            ? QStringLiteral(":/Icon/Icon/gaz_water2.svg")
            : QStringLiteral(":/Icon/Icon/water.svg");
    }
}

void MachineSettings::setItemHoldMs(ProductCategory category, int index, int holdMs)
{
    QVector<ProductItem> &v = itemsMutable(category);
    if (index < 0 || index >= v.size())
        return;
    v[index].holdMs = qBound(100, holdMs, 30000);
}

ProductItem MachineSettings::makeDefaultItem(ProductCategory category, int index) const
{
    ProductItem it;
    const QString key = QString::fromUtf8(categoryKey(category));
    it.id = QStringLiteral("%1_%2").arg(key).arg(index + 1);
    it.gpioChannel = (index % kOutputCount) + 1;
    switch (category) {
    case ProductCategory::Coffee:
        it.name = QStringLiteral("Кава %1").arg(index + 1);
        it.priceKopiyky = 1500;
        it.iconPath = QStringLiteral(":/Icon/Icon/coffee.svg");
        break;
    case ProductCategory::Water:
        it.name = QStringLiteral("Вода %1").arg(index + 1);
        it.priceKopiyky = 1000;
        it.iconPath = QStringLiteral(":/Icon/Icon/water.svg");
        it.sparkling = false;
        break;
    case ProductCategory::Snacks:
        it.name = QStringLiteral("Снек %1").arg(index + 1);
        it.priceKopiyky = 2000;
        it.iconPath = QStringLiteral(":/Icon/Icon/snack.svg");
        break;
    }
    it.priceText = priceTextFromKopiyky(it.priceKopiyky);
    return it;
}

void MachineSettings::resizeCategory(ProductCategory category, int newCount)
{
    QVector<ProductItem> &v = itemsMutable(category);
    // Снеки — не більше за кількість фізичних виходів (по виходу на снек).
    const int maxCount = (category == ProductCategory::Snacks) ? kOutputCount : 50;
    newCount = qBound(1, newCount, maxCount);
    if (newCount == v.size())
        return;
    if (newCount < v.size()) {
        v.resize(newCount);
    } else {
        for (int i = v.size(); i < newCount; ++i)
            v.append(makeDefaultItem(category, i));
    }
}

void MachineSettings::load()
{
    QSettings s(QStringLiteral("Tach"), QStringLiteral("CoffeSnack"));

    // Версія схеми збереження. Якщо у сховищі стара версія (або її немає) —
    // ігноруємо застарілі ключі й беремо дефолти, щоб не підхоплювати сміття
    // від попередніх версій програми (напр. назви, що "переїхали" між модулями).
    const int SCHEMA_VERSION = 3;
    const int storedVersion = s.value(QStringLiteral("schemaVersion"), 0).toInt();
    if (storedVersion < SCHEMA_VERSION) {
        // Стара/відсутня схема — чистимо все й лишаємо дефолти з initDefaults().
        s.clear();
        s.sync();
        return;
    }

    m_freeMode = s.value(QStringLiteral("freeMode"), false).toBool();
    m_coffeeEnabled = s.value(QStringLiteral("coffeeEnabled"), true).toBool();
    m_waterEnabled = s.value(QStringLiteral("waterEnabled"), true).toBool();
    m_snacksEnabled = s.value(QStringLiteral("snacksEnabled"), true).toBool();
    m_buttonHoldMs = s.value(QStringLiteral("buttonHoldMs"), 5000).toInt();

    const ProductCategory cats[] = {
        ProductCategory::Coffee, ProductCategory::Water, ProductCategory::Snacks
    };
    for (ProductCategory cat : cats) {
        QVector<ProductItem> &v = itemsMutable(cat);
        const QString key = QString::fromUtf8(categoryKey(cat));
        // Кількість товарів цієї категорії теж зберігаємо
        const int savedCount = s.value(QStringLiteral("%1_count").arg(key), v.size()).toInt();
        resizeCategory(cat, savedCount);
        for (int i = 0; i < v.size(); ++i) {
            const QString base = QStringLiteral("%1_%2_").arg(key).arg(i);
            v[i].name = s.value(base + QStringLiteral("name"), v[i].name).toString();
            v[i].gpioChannel = qBound(1, s.value(base + QStringLiteral("gpio"), v[i].gpioChannel).toInt(), 14);
            const int kop = s.value(base + QStringLiteral("priceKop"), v[i].priceKopiyky).toInt();
            v[i].priceKopiyky = qMax(0, kop);
            v[i].priceText = priceTextFromKopiyky(v[i].priceKopiyky);
            // тривалість утримання виходу для цього товару
            v[i].holdMs = qBound(100,
                s.value(base + QStringLiteral("holdMs"), v[i].holdMs).toInt(), 30000);
            if (cat == ProductCategory::Water) {
                v[i].sparkling = s.value(base + QStringLiteral("sparkling"), v[i].sparkling).toBool();
                v[i].iconPath = v[i].sparkling
                    ? QStringLiteral(":/Icon/Icon/gaz_water2.svg")
                    : QStringLiteral(":/Icon/Icon/water.svg");
            }
        }
    }
}

void MachineSettings::save()
{
    QSettings s(QStringLiteral("Tach"), QStringLiteral("CoffeSnack"));
    s.setValue(QStringLiteral("schemaVersion"), 3);
    s.setValue(QStringLiteral("freeMode"), m_freeMode);
    s.setValue(QStringLiteral("coffeeEnabled"), m_coffeeEnabled);
    s.setValue(QStringLiteral("waterEnabled"), m_waterEnabled);
    s.setValue(QStringLiteral("snacksEnabled"), m_snacksEnabled);
    s.setValue(QStringLiteral("buttonHoldMs"), m_buttonHoldMs);

    const ProductCategory cats[] = {
        ProductCategory::Coffee, ProductCategory::Water, ProductCategory::Snacks
    };
    for (ProductCategory cat : cats) {
        const QVector<ProductItem> &v = items(cat);
        const QString key = QString::fromUtf8(categoryKey(cat));

        // прибираємо старі ключі цієї категорії, щоб не лишалось "хвостів"
        // від попередньої (більшої) кількості товарів
        s.remove(key);  // видаляє всю групу key/...
        s.setValue(QStringLiteral("%1_count").arg(key), v.size());

        for (int i = 0; i < v.size(); ++i) {
            const QString base = QStringLiteral("%1_%2_").arg(key).arg(i);
            s.setValue(base + QStringLiteral("name"), v[i].name);
            s.setValue(base + QStringLiteral("gpio"), v[i].gpioChannel);
            s.setValue(base + QStringLiteral("priceKop"), v[i].priceKopiyky);
            s.setValue(base + QStringLiteral("holdMs"), v[i].holdMs);
            if (cat == ProductCategory::Water)
                s.setValue(base + QStringLiteral("sparkling"), v[i].sparkling);
        }
    }

    s.sync();
    emit settingsChanged();
}
