#include "protocolparser.h"

#include "protocolcodec.h"

namespace
{
constexpr char kHeader0 = 0x24;
constexpr char kHeader1 = 0x24;
constexpr char kTail = 0x26;
constexpr int kMinFrameSize = 6;
}

ProtocolParser::ProtocolParser(QObject *parent)
    : QObject(parent)
{
}

void ProtocolParser::appendData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    m_buffer.append(data);
    parseBuffer();
}

void ProtocolParser::reset()
{
    m_buffer.clear();
}

void ProtocolParser::parseBuffer()
{
    while (m_buffer.size() >= kMinFrameSize) {
        const int headerIndex = m_buffer.indexOf(QByteArray(1, kHeader0) + QByteArray(1, kHeader1));
        if (headerIndex < 0) {
            m_buffer.clear();
            return;
        }

        if (headerIndex > 0) {
            m_buffer.remove(0, headerIndex);
        }

        if (m_buffer.size() < kMinFrameSize) {
            return;
        }

        const quint8 dataLength = static_cast<quint8>(m_buffer.at(2));
        const int frameSize = 2 + 1 + 1 + dataLength + 1 + 1;
        if (m_buffer.size() < frameSize) {
            return;
        }

        const QByteArray frameBytes = m_buffer.left(frameSize);
        if (frameBytes.at(frameSize - 1) != kTail) {
            emit parseError(QStringLiteral("Frame tail mismatch"), frameBytes);
            m_buffer.remove(0, 1);
            continue;
        }

        const QByteArray data = frameBytes.mid(4, dataLength);
        const quint8 expectedChecksum = calculateChecksum(data);
        const quint8 actualChecksum = static_cast<quint8>(frameBytes.at(frameSize - 2));
        if (expectedChecksum != actualChecksum) {
            emit parseError(
                QStringLiteral("Checksum mismatch: expected %1 got %2")
                    .arg(expectedChecksum, 2, 16, QLatin1Char('0'))
                    .arg(actualChecksum, 2, 16, QLatin1Char('0')),
                frameBytes);
            m_buffer.remove(0, frameSize);
            continue;
        }

        ParsedFrame frame;
        frame.raw = frameBytes;
        frame.cmd = static_cast<quint8>(frameBytes.at(3));
        frame.data = data;
        frame.valid = true;
        emit frameParsed(frame);
        m_buffer.remove(0, frameSize);
    }
}

quint8 ProtocolParser::calculateChecksum(const QByteArray &data) const
{
    quint32 sum = 0;
    for (unsigned char byte : data) {
        sum += byte;
    }
    return static_cast<quint8>(sum & 0xFF);
}
