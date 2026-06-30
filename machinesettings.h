#pragma once

#include <QObject>
#include <QString>
#include <QVector>

enum class ProductCategory {
    Coffee = 0,
    Water = 1,
    Snacks = 2
};

struct ProductItem {
    QString id;
    QString name;
    QString priceText;
    int priceKopiyky = 0;
    int gpioChannel = 1;
    QString iconPath;
    bool sparkling = false;   // лише для води: газована/негазована
    int holdMs = 5000;        // тривалість утримання виходу для цього товару (мс)
};

class MachineSettings : public QObject {
    Q_OBJECT
public:
    static MachineSettings &instance();

    bool freeMode() const { return m_freeMode; }
    void setFreeMode(bool enabled);

    bool moduleEnabled(ProductCategory category) const;
    void setModuleEnabled(ProductCategory category, bool enabled);

    int buttonHoldMs() const { return m_buttonHoldMs; }
    void setButtonHoldMs(int ms);

    const QVector<ProductItem> &coffeeItems() const { return m_coffeeItems; }
    const QVector<ProductItem> &waterItems() const { return m_waterItems; }
    const QVector<ProductItem> &snackItems() const { return m_snackItems; }

    // Універсальний доступ за категорією
    const QVector<ProductItem> &items(ProductCategory category) const;
    int itemCount(ProductCategory category) const;

    // Редагування будь-якого товару за індексом
    void setItemName(ProductCategory category, int index, const QString &name);
    void setItemGpio(ProductCategory category, int index, int gpioChannel);
    void setItemPriceKopiyky(ProductCategory category, int index, int kopiyky);
    void setItemSparkling(ProductCategory category, int index, bool sparkling);
    void setItemHoldMs(ProductCategory category, int index, int holdMs);

    // Кількість фізичних виходів плати розширення (каналів видачі).
    static constexpr int kOutputCount = 14;

    // Зміна кількості товарів у категорії (1..50)
    void resizeCategory(ProductCategory category, int newCount);

    void load();
    void save();

signals:
    void settingsChanged();

private:
    explicit MachineSettings(QObject *parent = nullptr);
    void initDefaults();

    QVector<ProductItem> &itemsMutable(ProductCategory category);
    ProductItem makeDefaultItem(ProductCategory category, int index) const;
    static QString priceTextFromKopiyky(int kopiyky);
    static const char *categoryKey(ProductCategory category);

    bool m_freeMode = false;
    bool m_coffeeEnabled = true;
    bool m_waterEnabled = true;
    bool m_snacksEnabled = true;
    int m_buttonHoldMs = 5000;

    QVector<ProductItem> m_coffeeItems;
    QVector<ProductItem> m_waterItems;
    QVector<ProductItem> m_snackItems;
};
