#include "rp2040device.h"
#include <QDebug>

RP2040Device::RP2040Device(QObject *parent) : QObject(parent) {}

bool RP2040Device::connect(const QString &portName, int baudRate) {
    serial.setPortName(portName);
    serial.setBaudRate(baudRate);

    if (serial.open(QIODevice::ReadWrite)) {
        QObject::connect(&serial, &QSerialPort::readyRead,
                         this, &RP2040Device::readData);
        qDebug() << "Connected to RP2040 on" << portName;
        return true;
    } else {
        qDebug() << "Failed to open port" << portName;
        return false;
    }
}

void RP2040Device::sendMessage(const QByteArray &msg) {
    if (serial.isOpen()) {
        serial.write(msg);
        serial.flush();
        qDebug() << "Sent packet:" << msg.toHex();
    } else {
        qDebug() << "Port not open, cannot send message!";
    }
}

void RP2040Device::readData() {
    QByteArray data = serial.readAll();
    emit messageReceived(data);
    qDebug() << "Received from RP2040:" << data.toHex();
}
