#pragma once

#include "protocoltypes.h"

#include <QObject>

class ProtocolParser : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolParser(QObject *parent = nullptr);

    void appendData(const QByteArray &data);
    void reset();

signals:
    void frameParsed(const ParsedFrame &frame);
    void parseError(const QString &message, const QByteArray &rawFrame);

private:
    QByteArray m_buffer;

    void parseBuffer();
    quint8 calculateChecksum(const QByteArray &data) const;
};
