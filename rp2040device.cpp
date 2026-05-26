#include "rp2040device.h"
#include <QDebug>

namespace {
constexpr char kStartByte = char(0xAA);
constexpr char kEndByte = char(0x55);
constexpr char kAckOk = char(0xBB);
constexpr char kAckErr = char(0xEE);
}

RP2040Device::RP2040Device(QObject *parent)
    : QObject(parent)
{
    ackTimer = new QTimer(this);
    ackTimer->setSingleShot(true);
}

bool RP2040Device::connect(const QString &portName, int baudRate)
{
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);

    if (serial.open(QIODevice::ReadWrite)) {
        QObject::connect(&serial, &QSerialPort::readyRead,
                         this, &RP2040Device::readData);
        qDebug() << "Connected to RP2040 on" << portName;
        return true;
    }

    qDebug() << "Failed to open port" << portName;
    return false;
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

RP2040Device::DispenseResult RP2040Device::sendDispenseCommand(int gpioChannel,
                                                               int holdMs,
                                                               int timeoutMs)
{
    if (!serial.isOpen())
        return DispenseResult::PortClosed;

    gpioChannel = qBound(1, gpioChannel, 14);
    holdMs = qBound(100, holdMs, 30000);

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
    ackTimer->start(timeoutMs);

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

    if (!data.isEmpty())
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
