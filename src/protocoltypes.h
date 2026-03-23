#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QtGlobal>

struct SerialConfig
{
    QString portName;
    qint32 baudRate = 9600;
    qint32 dataBits = 8;
    qint32 parity = 0;
    qint32 stopBits = 1;
    qint32 flowControl = 0;
};

enum class Command : quint8
{
    SetHeating = 0x00,
    SetTemperatures = 0x02,
    ReadActualTemperatures = 0x04,
    SetShutdownDelay = 0x05,
    SetShutdownEnabled = 0x07,
    ReportAll = 0x21,
};

struct ParsedFrame
{
    QByteArray raw;
    quint8 cmd = 0;
    QByteArray data;
    bool valid = false;
    QString error;
};

struct DeviceState
{
    bool heatingEnabled = false;
    qint32 shutdownDelaySec = 0;
    bool shutdownEnabled = false;
    quint16 shutdownRemainingRaw = 0x00FF;
    qint32 setTemps[4] = {0, 0, 0, 0};
    double actualTemps[4] = {0.0, 0.0, 0.0, 0.0};
    QDateTime updatedAt;
};
