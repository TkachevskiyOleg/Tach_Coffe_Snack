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

    void setSnackDisplayName(const QString &name);
    void setSnackGpio(int gpioChannel);
    void setSnackPriceKopiyky(int kopiyky);

    void setCoffeeGpio(int index, int gpioChannel);
    void setWaterGpio(int index, int gpioChannel);

    void load();
    void save();

signals:
    void settingsChanged();

private:
    explicit MachineSettings(QObject *parent = nullptr);
    void initDefaults();

    bool m_freeMode = false;
    bool m_coffeeEnabled = true;
    bool m_waterEnabled = true;
    bool m_snacksEnabled = true;
    int m_buttonHoldMs = 1000;

    QVector<ProductItem> m_coffeeItems;
    QVector<ProductItem> m_waterItems;
    QVector<ProductItem> m_snackItems;
};
