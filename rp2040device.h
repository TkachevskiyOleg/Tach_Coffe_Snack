#ifndef RP2040DEVICE_H
#define RP2040DEVICE_H

#include <QSerialPort>
#include <QObject>
#include <QEventLoop>
#include <QTimer>

class RP2040Device : public QObject {
    Q_OBJECT
public:
    explicit RP2040Device(QObject *parent = nullptr);
    bool connect(const QString &portName = "/dev/ttyACM0", int baudRate = 115200);

    void sendMessage(const QByteArray &msg);

    enum class DispenseResult {
        Success,
        Error,
        Timeout,
        PortClosed
    };
    Q_ENUM(DispenseResult)

    DispenseResult sendDispenseCommand(int gpioChannel, int holdMs, int timeoutMs = 5000);

signals:
    void messageReceived(const QByteArray &msg);
    void dispenseFinished(RP2040Device::DispenseResult result);

private slots:
    void readData();

private:
    void appendRx(const QByteArray &data);
    DispenseResult parseRxBuffer();

    QSerialPort serial;
    QByteArray rxBuffer;
    bool waitingAck = false;
    QEventLoop *ackLoop = nullptr;
    QTimer *ackTimer = nullptr;
    DispenseResult lastResult = DispenseResult::Timeout;
};

#endif // RP2040DEVICE_H
