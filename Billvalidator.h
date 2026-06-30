#pragma once

#include <QThread>
#include <QByteArray>
#include <QVector>
#include <QMutex>
#include <QString>

// CCNET-master для купюрника CashCode VU(MVU). Адреса купюрника 0x03.
// CCNET — простий послідовний протокол: UART 9600 8N1 (БЕЗ 9-го біта й parity).
// Структура пакета:
//   0x02  [ADR]  [LNG]  [DATA...]  [CRC_LO]  [CRC_HI]
//     0x02 — SYNC, ADR — адреса (0x03 для купюрника),
//     LNG  — повна довжина пакета (разом із SYNC/ADR/LNG/CRC),
//     CRC16 (поліном 0x08408, як у CCNET).
//
// Цикл: RESET → (ACK) → POLL до "Initialize/Idle" → Get Bill Table →
//       Enable Bill Types → циклічний POLL. Прийняті купюри -> billAccepted().
class BillValidator : public QThread {
    Q_OBJECT
public:
    explicit BillValidator(const QString &portName = QStringLiteral("/dev/serial0"),
                           QObject *parent = nullptr);
    ~BillValidator() override;

    void run() override;

    // Безпечні з інших потоків команди (ставлять прапорці, виконуються в циклі)
    void requestStack();   // зарахувати купюру з ескроу у касету
    void requestReturn();  // повернути купюру з ескроу
    void requestEnable();  // дозволити прийом купюр
    void requestDisable(); // заборонити прийом купюр

signals:
    void billAccepted(int amountKopiyky);   // купюру зараховано (копійки)
    void billInEscrow(int amountKopiyky);    // купюра в ескроу, очікує рішення
    void statusChanged(const QString &text);
    void errorOccurred(const QString &text);
    void readyChanged(bool ready);

private:
    // ── Порт ──
    bool openPort();
    void closePort();

    // ── CCNET кадри ──
    // Зібрати й надіслати пакет: SYNC+ADR+LNG+data+CRC16
    bool sendPacket(const QByteArray &payload);
    // Надіслати «голу» команду без даних
    bool sendCmd(quint8 cmd, const QByteArray &data = QByteArray());
    // Прочитати один повний пакет-відповідь. Повертає DATA (без SYNC/ADR/LNG/CRC),
    // перевіривши CRC. Порожньо — якщо нічого/помилка.
    QByteArray readPacket(int timeoutMs = 200);
    // Надіслати ACK у відповідь на дані купюрника
    bool sendAck();

    static quint16 crc16(const QByteArray &data);

    // ── Послідовність ──
    bool doReset();
    bool getBillTable();
    bool enableBills(bool enable);
    void pollLoop();
    void handlePollByte(quint8 code);

    int billTypeToKopiyky(int billTypeIndex) const;

    QString m_portName;
    int m_fd = -1;

    QVector<int> m_billKopiyky;   // номінал кожного з 24 типів у копійках
    bool m_ready = false;
    bool m_enabled = false;

    // Маска дозволених типів купюр (біт i = тип i). Розкладається в байти
    // у правильному CCNET-порядку при відправці Enable.
    quint32 m_enableBits = 0;

    QMutex m_mutex;
    bool m_wantStack = false;
    bool m_wantReturn = false;
    bool m_wantEnable = true;
    bool m_wantDisable = false;
    quint8 m_lastPollState = 0xFF;  // для логування лише при зміні стану
};
