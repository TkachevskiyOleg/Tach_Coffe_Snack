#ifndef RP2040DEVICE_H
#define RP2040DEVICE_H

#include <QSerialPort>
#include <QObject>

class RP2040Device : public QObject {
    Q_OBJECT
public:
    explicit RP2040Device(QObject *parent = nullptr);
    bool connect(const QString &portName = "/dev/ttyACM0", int baudRate = 115200);

    // Тепер приймає QByteArray для відправки сирих байтів
    void sendMessage(const QByteArray &msg);

signals:
    void messageReceived(const QByteArray &msg);

private slots:
    void readData();

private:
    QSerialPort serial;
};

#endif // RP2040DEVICE_H
