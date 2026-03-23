#pragma once

#include "protocoltypes.h"

class ProtocolCodec
{
public:
    static QByteArray buildSetHeating(bool enabled);
    static QByteArray buildSetTemperatures(qint32 t1, qint32 t2, qint32 t3, qint32 t4);
    static QByteArray buildReadActualTemperatures();
    static QByteArray buildSetShutdownDelay(qint32 seconds);
    static QByteArray buildSetShutdownEnabled(bool enabled);

    static QString bytesToHex(const QByteArray &bytes);
    static DeviceState decodeDeviceState(const QByteArray &data, bool *ok = nullptr, QString *error = nullptr);

private:
    static QByteArray buildFrame(Command cmd, const QByteArray &data);
    static quint8 calculateChecksum(const QByteArray &data);
    static void appendInt32LE(QByteArray &buffer, qint32 value);
    static qint32 readInt32LE(const QByteArray &buffer, int offset);
    static quint16 readUInt16LE(const QByteArray &buffer, int offset);
};
