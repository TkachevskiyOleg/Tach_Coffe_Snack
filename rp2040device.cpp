#include "rp2040device.h"
#include <QDebug>

namespace {
constexpr char kStartByte = char(0xAA);
constexpr char kEndByte = char(0x55);
constexpr char kAckOk = char(0xBB);
constexpr char kAckErr = char(0xEE);
constexpr unsigned char kBtnDown = 0xC1;  // сервісна кнопка натиснута
constexpr unsigned char kBtnUp   = 0xC0;  // сервісна кнопка відпущена
}

RP2040Device::RP2040Device(QObject *parent)
    : QObject(parent)
{
    ackTimer = new QTimer(this);
    ackTimer->setSingleShot(true);

    // Підключаємо сигнали ОДИН РАЗ тут, а не всередині connect(): об'єкт
    // serial живе протягом усього часу роботи додатка й лише перевідкривається
    // на новому порту при перепідключенні плати. Якби ці connect() були в
    // методі connect(), кожне перепідключення додавало б ще один дублікат
    // з'єднання сигналу — readData() викликався б по кілька разів на подію.
    QObject::connect(&serial, &QSerialPort::readyRead,
                     this, &RP2040Device::readData);
    QObject::connect(&serial, &QSerialPort::errorOccurred,
                     this, &RP2040Device::handleSerialError);
}

bool RP2040Device::connect(const QString &portName, int baudRate)
{
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);

    if (serial.open(QIODevice::ReadWrite)) {
        qDebug() << "Connected to RP2040 on" << portName;
        return true;
    }

    qDebug() << "Failed to open port" << portName;
    return false;
}

void RP2040Device::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    // Будь-яка помилка порту (найчастіше ResourceError — фізичне
    // відключення кабелю) означає, що зв'язок із платою втрачено.
    // Закриваємо порт явно: isConnected() одразу почне повертати false,
    // і MainWindow зможе періодично пробувати знайти й підключити плату
    // знову, щойно вона з'явиться в системі.
    qDebug() << "RP2040 помилка порту:" << error << serial.errorString();
    if (serial.isOpen())
        serial.close();

    // Якщо саме в цей момент очікували ACK на команду видачі — не лишаємо
    // handleProductClick() вічно чекати на нездійсненну подію.
    if (waitingAck) {
        lastResult = DispenseResult::PortClosed;
        waitingAck = false;
        if (ackLoop)
            ackLoop->quit();
    }

    emit disconnected();
}

void RP2040Device::sendMessage(const QByteArray &msg)
{
    if (serial.isOpen()) {
        serial.write(msg);
        serial.flush();
        qDebug() << "Sent packet:" << msg.toHex();
    } else {
        qDebug() << "Port not open, cannot send message!";
    }
}

void RP2040Device::requestSettingsButtonState()
{
    // Надсилаємо запит 0xA0 — плата відповість маркером BTN_DOWN/BTN_UP.
    // Потрібно при підключенні, щоб дізнатися стан кнопки, навіть якщо її
    // вже тримають з моменту старту.
    QByteArray q;
    q.append(char(0xA0));
    sendMessage(q);
}

RP2040Device::DispenseResult RP2040Device::sendDispenseCommand(int gpioChannel,
                                                               int holdMs,
                                                               int timeoutMs)
{
    if (!serial.isOpen())
        return DispenseResult::PortClosed;

    gpioChannel = qBound(1, gpioChannel, 14);
    holdMs = qBound(100, holdMs, 30000);

    // Таймаут має бути більшим за час утримання виходу: навіть якщо плата
    // надішле підтвердження вже після завершення видачі, Qt має дочекатись.
    // Беремо щонайбільше з переданого timeoutMs та (holdMs + 3000 мс запасу).
    const int effectiveTimeout = qMax(timeoutMs, holdMs + 3000);

    QByteArray packet;
    packet.append(kStartByte);
    packet.append(char(gpioChannel));
    packet.append(char((holdMs >> 8) & 0xFF));
    packet.append(char(holdMs & 0xFF));
    packet.append(kEndByte);

    rxBuffer.clear();
    waitingAck = true;
    lastResult = DispenseResult::Timeout;

    QEventLoop loop;
    ackLoop = &loop;

    QObject::connect(ackTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    ackTimer->start(effectiveTimeout);

    sendMessage(packet);

    loop.exec();

    ackTimer->stop();
    waitingAck = false;
    ackLoop = nullptr;

    emit dispenseFinished(lastResult);
    return lastResult;
}

void RP2040Device::readData()
{
    const QByteArray data = serial.readAll();
    emit messageReceived(data);
    qDebug() << "Received from RP2040:" << data.toHex();

    if (data.isEmpty())
        return;

    // Вибираємо маркери стану сервісної кнопки (GPIO15) — вони можуть
    // приходити будь-коли, незалежно від видачі.
    for (char b : data) {
        const unsigned char ub = static_cast<unsigned char>(b);
        if (ub == kBtnDown)
            emit settingsButtonChanged(true);
        else if (ub == kBtnUp)
            emit settingsButtonChanged(false);
    }

    appendRx(data);
}

void RP2040Device::appendRx(const QByteArray &data)
{
    rxBuffer.append(data);

    if (!waitingAck)
        return;

    const DispenseResult r = parseRxBuffer();
    if (r == DispenseResult::Success || r == DispenseResult::Error) {
        lastResult = r;
        if (ackLoop)
            ackLoop->quit();
    }
}

RP2040Device::DispenseResult RP2040Device::parseRxBuffer()
{
    for (char b : rxBuffer) {
        if (b == kAckOk)
            return DispenseResult::Success;
        if (b == kAckErr)
            return DispenseResult::Error;
    }
    return DispenseResult::Timeout;
}
