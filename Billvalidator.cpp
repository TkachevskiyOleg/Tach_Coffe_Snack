#include "Billvalidator.h"

#include <QDebug>
#include <QElapsedTimer>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <cstring>

// ── CCNET константи ──
namespace {
constexpr quint8 SYNC      = 0x02;
constexpr quint8 ADR_BV    = 0x03;  // адреса купюрника в CCNET

// Команди CCNET
constexpr quint8 CMD_RESET       = 0x30;
constexpr quint8 CMD_GET_STATUS  = 0x31;
constexpr quint8 CMD_GET_BILLTAB = 0x41; // Get Bill Table
constexpr quint8 CMD_POLL        = 0x33;
constexpr quint8 CMD_ENABLE_BILL = 0x34; // Enable Bill Types
constexpr quint8 CMD_STACK       = 0x35; // Stack (зарахувати з ескроу)
constexpr quint8 CMD_RETURN      = 0x36; // Return (повернути з ескроу)
constexpr quint8 CMD_IDENT       = 0x37;
constexpr quint8 CMD_ACK_BYTE    = 0x00; // тіло ACK-пакета

// Коди стану в POLL-відповіді
constexpr quint8 ST_POWER_UP      = 0x10;
constexpr quint8 ST_POWER_UP_BV   = 0x11;
constexpr quint8 ST_POWER_UP_ESCR = 0x12;
constexpr quint8 ST_INITIALIZE    = 0x13;
constexpr quint8 ST_IDLING        = 0x14;
constexpr quint8 ST_ACCEPTING     = 0x15;
constexpr quint8 ST_STACKING      = 0x17;
constexpr quint8 ST_RETURNING     = 0x18;
constexpr quint8 ST_DISABLED      = 0x19;
constexpr quint8 ST_HOLDING       = 0x1A;
constexpr quint8 ST_BUSY          = 0x1B;
constexpr quint8 ST_REJECTING     = 0x1C; // далі йде байт причини
constexpr quint8 ST_BILL_STACKED  = 0x81; // + байт типу купюри
constexpr quint8 ST_ESCROW        = 0x80; // + байт типу купюри
constexpr quint8 ST_BILL_RETURNED = 0x82;
constexpr quint8 ST_DROP_CASSETTE_OUT = 0x42;
constexpr quint8 ST_DROP_CASSETTE_FULL = 0x41;
}

BillValidator::BillValidator(const QString &portName, QObject *parent)
    : QThread(parent), m_portName(portName)
{
    m_billKopiyky.resize(24);
    m_billKopiyky.fill(0);
}

BillValidator::~BillValidator()
{
    requestInterruption();
    wait(2000);
    closePort();
}

// ───────────────────────── Порт ─────────────────────────

bool BillValidator::openPort()
{
    m_fd = ::open(m_portName.toLocal8Bit().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        emit errorOccurred(tr("Не вдалося відкрити порт %1").arg(m_portName));
        return false;
    }

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    if (tcgetattr(m_fd, &tio) != 0) {
        emit errorOccurred(tr("tcgetattr помилка"));
        closePort();
        return false;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, B9600);
    cfsetospeed(&tio, B9600);

    // CCNET: 9600, 8 біт даних, БЕЗ parity, 1 стоп-біт (8N1)
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~PARENB;    // без parity
    tio.c_cflag &= ~CSTOPB;    // 1 стоп-біт

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tio) != 0) {
        emit errorOccurred(tr("tcsetattr помилка"));
        closePort();
        return false;
    }
    tcflush(m_fd, TCIOFLUSH);
    qDebug() << "[BV] CCNET порт відкрито:" << m_portName << "fd=" << m_fd;
    return true;
}

void BillValidator::closePort()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

// ───────────────────────── CRC16 (CCNET) ─────────────────────────
// Поліном 0x08408, як у специфікації CCNET.
quint16 BillValidator::crc16(const QByteArray &data)
{
    quint16 crc = 0;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<quint8>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// ───────────────────────── Передача/прийом ─────────────────────────

bool BillValidator::sendPacket(const QByteArray &payload)
{
    if (m_fd < 0)
        return false;

    // payload — це тіло (команда + дані). Зберемо повний пакет.
    QByteArray pkt;
    pkt.append(static_cast<char>(SYNC));
    pkt.append(static_cast<char>(ADR_BV));
    const int lng = 5 + payload.size(); // SYNC+ADR+LNG + payload + CRC(2)
    pkt.append(static_cast<char>(lng & 0xFF));
    pkt.append(payload);
    const quint16 crc = crc16(pkt);
    pkt.append(static_cast<char>(crc & 0xFF));
    pkt.append(static_cast<char>((crc >> 8) & 0xFF));

    tcflush(m_fd, TCIFLUSH);
    const ssize_t n = ::write(m_fd, pkt.constData(), pkt.size());
    tcdrain(m_fd);
    qDebug() << "[BV] TX:" << pkt.toHex(' ');
    return n == pkt.size();
}

bool BillValidator::sendCmd(quint8 cmd, const QByteArray &data)
{
    QByteArray payload;
    payload.append(static_cast<char>(cmd));
    payload.append(data);
    return sendPacket(payload);
}

bool BillValidator::sendAck()
{
    // ACK у CCNET — пакет із тілом 0x00
    QByteArray payload;
    payload.append(static_cast<char>(CMD_ACK_BYTE));
    return sendPacket(payload);
}

QByteArray BillValidator::readPacket(int timeoutMs)
{
    QByteArray raw;
    QByteArray empty;
    if (m_fd < 0)
        return empty;

    QElapsedTimer timer;
    timer.start();
    quint8 buf[128];

    int idle = 0;
    while (timer.elapsed() < timeoutMs) {
        const ssize_t n = ::read(m_fd, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i)
                raw.append(static_cast<char>(buf[i]));
            idle = 0;
            QThread::msleep(2);
        } else {
            QThread::msleep(2);
            idle += 2;
            if (!raw.isEmpty() && idle > 20)
                break;
        }
    }

    if (raw.isEmpty()) {
        qDebug() << "[BV] RX: (тиша)";
        return empty;
    }
    qDebug() << "[BV] RX:" << raw.toHex(' ');

    // Розбір: знайти SYNC 0x02, ADR, LNG, тіло, CRC
    int start = raw.indexOf(static_cast<char>(SYNC));
    if (start < 0 || raw.size() - start < 5) {
        qDebug() << "[BV] RX: немає валідного SYNC/короткий";
        return empty;
    }
    const quint8 adr = static_cast<quint8>(raw[start + 1]);
    const quint8 lng = static_cast<quint8>(raw[start + 2]);
    if (lng < 5 || start + lng > raw.size()) {
        qDebug() << "[BV] RX: некоректна довжина LNG=" << lng;
        return empty;
    }
    const QByteArray pkt = raw.mid(start, lng);
    // перевірка CRC
    const QByteArray body = pkt.left(lng - 2);
    const quint16 gotCrc = static_cast<quint8>(pkt[lng - 2]) |
                           (static_cast<quint8>(pkt[lng - 1]) << 8);
    const quint16 calc = crc16(body);
    if (gotCrc != calc) {
        qDebug() << "[BV] RX: CRC не збігається got=" << QString::number(gotCrc,16)
                 << "calc=" << QString::number(calc,16);
        return empty;
    }
    Q_UNUSED(adr);
    // DATA = тіло без SYNC/ADR/LNG
    return pkt.mid(3, lng - 5);
}

// ───────────────────────── Послідовність ─────────────────────────

bool BillValidator::doReset()
{
    emit statusChanged(tr("Купюрник: RESET"));
    if (!sendCmd(CMD_RESET))
        return false;
    // купюрник відповідає ACK
    readPacket(300);
    return true;
}

bool BillValidator::getBillTable()
{
    emit statusChanged(tr("Купюрник: читання таблиці купюр"));
    if (!sendCmd(CMD_GET_BILLTAB))
        return false;
    const QByteArray d = readPacket(400);
    // Таблиця: 24 записи по 5 байтів: [номінал(1)] [код валюти(3)] [знак(1)]
    if (d.size() < 5) {
        qDebug() << "[BV] Bill table: коротка відповідь" << d.size();
        return false;
    }
    for (int i = 0; i < 24; ++i) {
        const int off = i * 5;
        if (off + 5 > d.size())
            break;
        const quint8 nominal = static_cast<quint8>(d[off]);     // базове число
        const quint8 power   = static_cast<quint8>(d[off + 4]); // множник 10^power
        // номінал у одиницях валюти = nominal * 10^power
        long value = nominal;
        for (int p = 0; p < power; ++p)
            value *= 10;
        // у копійки (гривні мають 2 знаки)
        m_billKopiyky[i] = static_cast<int>(value * 100);
    }
    m_ready = true;
    emit readyChanged(true);
    emit statusChanged(tr("Купюрник готовий (таблиця купюр зчитана)"));
    return true;
}

bool BillValidator::enableBills(bool enable)
{
    // Enable Bill Types: 3 байти маски дозволу типів + 3 байти ескроу-маски.
    QByteArray data;
    const quint8 e = enable ? 0xFF : 0x00;
    data.append(char(e)); data.append(char(e)); data.append(char(e)); // enable
    data.append(char(e)); data.append(char(e)); data.append(char(e)); // escrow
    if (!sendCmd(CMD_ENABLE_BILL, data))
        return false;
    readPacket(200);
    m_enabled = enable;
    emit statusChanged(enable ? tr("Прийом купюр увімкнено")
                              : tr("Прийом купюр вимкнено"));
    return true;
}

int BillValidator::billTypeToKopiyky(int billTypeIndex) const
{
    if (billTypeIndex < 0 || billTypeIndex >= m_billKopiyky.size())
        return 0;
    return m_billKopiyky[billTypeIndex];
}

void BillValidator::handlePollByte(quint8 code)
{
    switch (code) {
    case ST_IDLING:      break; // норма, без спаму
    case ST_BUSY:        break;
    case ST_INITIALIZE:  emit statusChanged(tr("Купюрник ініціалізується")); break;
    case ST_DISABLED:    emit statusChanged(tr("Купюрник вимкнено")); break;
    case ST_ACCEPTING:   emit statusChanged(tr("Перевірка купюри...")); break;
    case ST_STACKING:    emit statusChanged(tr("Купюра укладається")); break;
    case ST_DROP_CASSETTE_OUT:  emit errorOccurred(tr("Касету купюрника знято")); break;
    case ST_DROP_CASSETTE_FULL: emit errorOccurred(tr("Касета купюрника заповнена")); break;
    default: break;
    }
}

void BillValidator::pollLoop()
{
    while (!isInterruptionRequested()) {
        bool doStack=false, doReturn=false, doEnable=false, doDisable=false;
        {
            QMutexLocker lock(&m_mutex);
            doStack = m_wantStack;   m_wantStack = false;
            doReturn = m_wantReturn; m_wantReturn = false;
            doEnable = m_wantEnable; m_wantEnable = false;
            doDisable = m_wantDisable; m_wantDisable = false;
        }
        if (doEnable && !m_enabled)  enableBills(true);
        if (doDisable && m_enabled)  enableBills(false);
        if (doStack)  { sendCmd(CMD_STACK);  readPacket(150); }
        if (doReturn) { sendCmd(CMD_RETURN); readPacket(150); }

        // POLL
        if (sendCmd(CMD_POLL)) {
            const QByteArray d = readPacket(200);
            if (!d.isEmpty()) {
                // d[0] — код стану; для 0x80/0x81 далі йде d[1] = тип купюри
                const quint8 st = static_cast<quint8>(d[0]);

                if (st == ST_ESCROW && d.size() >= 2) {
                    const int type = static_cast<quint8>(d[1]);
                    const int kop = billTypeToKopiyky(type);
                    emit billInEscrow(kop);
                    // автоматично зараховуємо у касету
                    QMutexLocker lock(&m_mutex);
                    m_wantStack = true;
                } else if (st == ST_BILL_STACKED && d.size() >= 2) {
                    const int type = static_cast<quint8>(d[1]);
                    const int kop = billTypeToKopiyky(type);
                    emit billAccepted(kop);
                    emit statusChanged(tr("Прийнято купюру: %1 грн")
                                           .arg(kop / 100.0, 0, 'f', 2));
                } else {
                    handlePollByte(st);
                }
                // підтверджуємо отримання статусу
                sendAck();
            }
        }
        QThread::msleep(150);
    }
}

void BillValidator::run()
{
    if (!openPort())
        return;

    doReset();
    QThread::msleep(300);

    // кілька POLL поки купюрник проходить power-up/initialize
    for (int i = 0; i < 10 && !isInterruptionRequested(); ++i) {
        sendCmd(CMD_POLL);
        readPacket(200);
        QThread::msleep(100);
    }

    getBillTable();      // навіть якщо не вийшло — спробуємо далі
    enableBills(true);

    pollLoop();

    enableBills(false);
    closePort();
}

// ───────────────────────── Команди ─────────────────────────

void BillValidator::requestStack()  { QMutexLocker l(&m_mutex); m_wantStack = true; }
void BillValidator::requestReturn() { QMutexLocker l(&m_mutex); m_wantReturn = true; }
void BillValidator::requestEnable() { QMutexLocker l(&m_mutex); m_wantEnable = true; }
void BillValidator::requestDisable(){ QMutexLocker l(&m_mutex); m_wantDisable = true; }
