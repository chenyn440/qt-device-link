#include "serialportmanager.h"

#include "protocolcodec.h"

#include <QSerialPort>

SerialPortManager::SerialPortManager(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_parser(new ProtocolParser(this))
{
    connectSignals();
}

SerialPortManager::~SerialPortManager() = default;

bool SerialPortManager::openPort(const SerialConfig &config)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_serialPort->setPortName(config.portName);
    m_serialPort->setBaudRate(config.baudRate);
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(config.dataBits));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(config.parity));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(config.stopBits));
    m_serialPort->setFlowControl(static_cast<QSerialPort::FlowControl>(config.flowControl));

    m_parser->reset();
    const bool ok = m_serialPort->open(QIODevice::ReadWrite);
    if (!ok) {
        emit portError(m_serialPort->errorString());
    } else {
        emit logMessage(QStringLiteral("Port opened: %1").arg(config.portName));
    }
    return ok;
}

void SerialPortManager::closePort()
{
    if (!m_serialPort->isOpen()) {
        return;
    }
    m_serialPort->close();
    m_parser->reset();
    emit logMessage(QStringLiteral("Port closed"));
}

bool SerialPortManager::isOpen() const
{
    return m_serialPort->isOpen();
}

bool SerialPortManager::sendBytes(const QByteArray &bytes)
{
    if (!m_serialPort->isOpen()) {
        emit portError(QStringLiteral("Serial port is not open"));
        return false;
    }

    const qint64 written = m_serialPort->write(bytes);
    const bool ok = written == bytes.size();
    if (!ok) {
        emit portError(QStringLiteral("Failed to send full frame"));
        return false;
    }

    emit logMessage(QStringLiteral("TX: %1").arg(ProtocolCodec::bytesToHex(bytes)));
    return true;
}

void SerialPortManager::connectSignals()
{
    connect(m_serialPort, &QSerialPort::readyRead, this, [this]() {
        const QByteArray bytes = m_serialPort->readAll();
        if (bytes.isEmpty()) {
            return;
        }
        emit rawBytesReceived(bytes);
        emit logMessage(QStringLiteral("RX: %1").arg(ProtocolCodec::bytesToHex(bytes)));
        m_parser->appendData(bytes);
    });

    connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError) {
            return;
        }
        emit portError(m_serialPort->errorString());
    });

    connect(m_parser, &ProtocolParser::frameParsed, this, &SerialPortManager::frameReceived);
    connect(m_parser, &ProtocolParser::parseError, this, [this](const QString &message, const QByteArray &rawFrame) {
        emit logMessage(QStringLiteral("PARSE ERROR: %1 | RAW: %2").arg(message, ProtocolCodec::bytesToHex(rawFrame)));
    });
}
