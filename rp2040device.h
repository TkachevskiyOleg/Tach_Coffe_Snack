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

    // true, поки послідовний порт реально відкрито. MainWindow звіряється з
    // цим перед кожною спробою (пере)підключення, щоб не намагатись
    // відкрити вже відкритий порт.
    bool isConnected() const { return serial.isOpen(); }

    void sendMessage(const QByteArray &msg);

    enum class DispenseResult {
        Success,
        Error,
        Timeout,
        PortClosed
    };
    Q_ENUM(DispenseResult)

    DispenseResult sendDispenseCommand(int gpioChannel, int holdMs, int timeoutMs = 5000);

    // Запитати поточний стан сервісної кнопки GPIO15 (плата відповість маркером).
    void requestSettingsButtonState();

signals:
    void messageReceived(const QByteArray &msg);
    void dispenseFinished(RP2040Device::DispenseResult result);
    void settingsButtonChanged(bool pressed);  // GPIO15: натиснуто/відпущено
    // Емітується, коли зв'язок із платою обірвався (фізичне відключення
    // кабелю, помилка порту тощо). MainWindow ловить цей сигнал і починає
    // знову періодично шукати плату для перепідключення.
    void disconnected();

private slots:
    void readData();
    // Обробка будь-якої помилки QSerialPort (зокрема ResourceError при
    // фізичному висмикуванні кабелю) — закриває порт і повідомляє про
    // втрату зв'язку, щоб MainWindow міг ініціювати перепідключення.
    void handleSerialError(QSerialPort::SerialPortError error);

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
