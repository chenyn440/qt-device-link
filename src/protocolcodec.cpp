#include "protocolcodec.h"

#include <QStringList>

namespace
{
constexpr char kHeader0 = 0x24;
constexpr char kHeader1 = 0x24;
constexpr char kTail = 0x26;
// The provided field layout sums to 46 bytes even though one example says 0x2C.
// We decode by the explicit field map because it is internally consistent.
constexpr int kReportDataSize = 0x2E;
}

QByteArray ProtocolCodec::buildSetHeating(bool enabled)
{
    QByteArray data;
    appendInt32LE(data, enabled ? 1 : 0);
    return buildFrame(Command::SetHeating, data);
}

QByteArray ProtocolCodec::buildSetTemperatures(qint32 t1, qint32 t2, qint32 t3, qint32 t4)
{
    QByteArray data;
    appendInt32LE(data, t1);
    appendInt32LE(data, t2);
    appendInt32LE(data, t3);
    appendInt32LE(data, t4);
    return buildFrame(Command::SetTemperatures, data);
}

QByteArray ProtocolCodec::buildReadActualTemperatures()
{
    QByteArray data;
    appendInt32LE(data, 0);
    return buildFrame(Command::ReadActualTemperatures, data);
}

QByteArray ProtocolCodec::buildSetShutdownDelay(qint32 seconds)
{
    QByteArray data;
    appendInt32LE(data, seconds);
    return buildFrame(Command::SetShutdownDelay, data);
}

QByteArray ProtocolCodec::buildSetShutdownEnabled(bool enabled)
{
    QByteArray data;
    appendInt32LE(data, enabled ? 1 : 0);
    return buildFrame(Command::SetShutdownEnabled, data);
}

QString ProtocolCodec::bytesToHex(const QByteArray &bytes)
{
    QStringList parts;
    parts.reserve(bytes.size());
    for (unsigned char byte : bytes) {
        parts << QStringLiteral("%1").arg(byte, 2, 16, QLatin1Char('0')).toUpper();
    }
    return parts.join(QLatin1Char(' '));
}

DeviceState ProtocolCodec::decodeDeviceState(const QByteArray &data, bool *ok, QString *error)
{
    DeviceState state;
    bool localOk = true;
    QString localError;

    if (data.size() != kReportDataSize) {
        localOk = false;
        localError = QStringLiteral("0x21 data length mismatch: expected %1, got %2")
                         .arg(kReportDataSize)
                         .arg(data.size());
    } else {
        state.heatingEnabled = readInt32LE(data, 0) != 0;
        state.shutdownDelaySec = readInt32LE(data, 4);
        state.shutdownEnabled = readInt32LE(data, 8) != 0;
        state.shutdownRemainingRaw = readUInt16LE(data, 12);
        state.setTemps[0] = readInt32LE(data, 14);
        state.setTemps[1] = readInt32LE(data, 18);
        state.setTemps[2] = readInt32LE(data, 22);
        state.setTemps[3] = readInt32LE(data, 26);
        state.actualTemps[0] = readInt32LE(data, 30) / 10.0;
        state.actualTemps[1] = readInt32LE(data, 34) / 10.0;
        state.actualTemps[2] = readInt32LE(data, 38) / 10.0;
        state.actualTemps[3] = readInt32LE(data, 42) / 10.0;
        state.updatedAt = QDateTime::currentDateTime();
    }

    if (ok) {
        *ok = localOk;
    }
    if (error) {
        *error = localError;
    }
    return state;
}

QByteArray ProtocolCodec::buildFrame(Command cmd, const QByteArray &data)
{
    QByteArray frame;
    frame.reserve(2 + 1 + 1 + data.size() + 1 + 1);
    frame.append(kHeader0);
    frame.append(kHeader1);
    frame.append(static_cast<char>(data.size()));
    frame.append(static_cast<char>(cmd));
    frame.append(data);
    frame.append(static_cast<char>(calculateChecksum(data)));
    frame.append(kTail);
    return frame;
}

quint8 ProtocolCodec::calculateChecksum(const QByteArray &data)
{
    quint32 sum = 0;
    for (unsigned char byte : data) {
        sum += byte;
    }
    return static_cast<quint8>(sum & 0xFF);
}

void ProtocolCodec::appendInt32LE(QByteArray &buffer, qint32 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
    buffer.append(static_cast<char>((value >> 16) & 0xFF));
    buffer.append(static_cast<char>((value >> 24) & 0xFF));
}

qint32 ProtocolCodec::readInt32LE(const QByteArray &buffer, int offset)
{
    quint32 b0 = static_cast<quint8>(buffer.at(offset));
    quint32 b1 = static_cast<quint8>(buffer.at(offset + 1));
    quint32 b2 = static_cast<quint8>(buffer.at(offset + 2));
    quint32 b3 = static_cast<quint8>(buffer.at(offset + 3));
    quint32 raw = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    return static_cast<qint32>(raw);
}

quint16 ProtocolCodec::readUInt16LE(const QByteArray &buffer, int offset)
{
    quint16 b0 = static_cast<quint8>(buffer.at(offset));
    quint16 b1 = static_cast<quint8>(buffer.at(offset + 1));
    return static_cast<quint16>(b0 | (b1 << 8));
}
