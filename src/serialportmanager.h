#pragma once

#include "protocolparser.h"
#include "protocoltypes.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QSerialPort;
QT_END_NAMESPACE

class SerialPortManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortManager(QObject *parent = nullptr);
    ~SerialPortManager() override;

    bool openPort(const SerialConfig &config);
    void closePort();
    bool isOpen() const;
    bool sendBytes(const QByteArray &bytes);

signals:
    void rawBytesReceived(const QByteArray &bytes);
    void frameReceived(const ParsedFrame &frame);
    void logMessage(const QString &message);
    void portError(const QString &message);

private:
    QSerialPort *m_serialPort = nullptr;
    ProtocolParser *m_parser = nullptr;

    void connectSignals();
};
