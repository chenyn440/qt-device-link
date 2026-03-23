#include "protocolcodec.h"
#include "protocolparser.h"

#include <QSignalSpy>
#include <QtTest>

class ProtocolTests : public QObject
{
    Q_OBJECT

private slots:
    void buildFrames_data();
    void buildFrames();
    void decodeDeviceStateFromKnownSample();
    void parserHandlesHalfPacket();
    void parserHandlesStickyPackets();
    void parserRejectsBadChecksum();
};

void ProtocolTests::buildFrames_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<QString>("hex");

    QTest::newRow("set heating on")
        << 0
        << QStringLiteral("24 24 04 00 01 00 00 00 01 26");
    QTest::newRow("read actual temperatures")
        << 1
        << QStringLiteral("24 24 04 04 00 00 00 00 00 26");
    QTest::newRow("set shutdown 60s")
        << 2
        << QStringLiteral("24 24 04 05 3C 00 00 00 3C 26");
    QTest::newRow("enable shutdown")
        << 3
        << QStringLiteral("24 24 04 07 01 00 00 00 01 26");
    QTest::newRow("set temperatures")
        << 4
        << QStringLiteral("24 24 10 02 46 00 00 00 41 00 00 00 3F 00 00 00 3D 00 00 00 03 26");
}

void ProtocolTests::buildFrames()
{
    QFETCH(int, kind);
    QFETCH(QString, hex);

    QByteArray frame;
    switch (kind) {
    case 0:
        frame = ProtocolCodec::buildSetHeating(true);
        break;
    case 1:
        frame = ProtocolCodec::buildReadActualTemperatures();
        break;
    case 2:
        frame = ProtocolCodec::buildSetShutdownDelay(60);
        break;
    case 3:
        frame = ProtocolCodec::buildSetShutdownEnabled(true);
        break;
    case 4:
        frame = ProtocolCodec::buildSetTemperatures(70, 65, 63, 61);
        break;
    default:
        QFAIL("unknown test kind");
    }

    QCOMPARE(ProtocolCodec::bytesToHex(frame), hex);
}

void ProtocolTests::decodeDeviceStateFromKnownSample()
{
    const QByteArray data = QByteArray::fromHex(
        "01000000"
        "3C000000"
        "01000000"
        "FF00"
        "46000000"
        "41000000"
        "3F000000"
        "3D000000"
        "28000000"
        "31000000"
        "3D000000"
        "2A000000");

    bool ok = false;
    QString error;
    const DeviceState state = ProtocolCodec::decodeDeviceState(data, &ok, &error);

    QVERIFY2(ok, qPrintable(error));
    QVERIFY(state.heatingEnabled);
    QCOMPARE(state.shutdownDelaySec, 60);
    QVERIFY(state.shutdownEnabled);
    QCOMPARE(state.shutdownRemainingRaw, quint16(0x00FF));
    QCOMPARE(state.setTemps[0], 70);
    QCOMPARE(state.setTemps[1], 65);
    QCOMPARE(state.setTemps[2], 63);
    QCOMPARE(state.setTemps[3], 61);
    QCOMPARE(state.actualTemps[0], 4.0);
    QCOMPARE(state.actualTemps[1], 4.9);
    QCOMPARE(state.actualTemps[2], 6.1);
    QCOMPARE(state.actualTemps[3], 4.2);
}

void ProtocolTests::parserHandlesHalfPacket()
{
    ProtocolParser parser;
    QSignalSpy frameSpy(&parser, &ProtocolParser::frameParsed);
    QSignalSpy errorSpy(&parser, &ProtocolParser::parseError);

    const QByteArray frame = ProtocolCodec::buildSetHeating(true);
    parser.appendData(frame.left(4));
    QCOMPARE(frameSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    parser.appendData(frame.mid(4));
    QCOMPARE(frameSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
}

void ProtocolTests::parserHandlesStickyPackets()
{
    ProtocolParser parser;
    QSignalSpy frameSpy(&parser, &ProtocolParser::frameParsed);

    const QByteArray packet = ProtocolCodec::buildSetHeating(true)
                              + ProtocolCodec::buildSetShutdownDelay(60);
    parser.appendData(packet);
    QCOMPARE(frameSpy.count(), 2);
}

void ProtocolTests::parserRejectsBadChecksum()
{
    ProtocolParser parser;
    QSignalSpy frameSpy(&parser, &ProtocolParser::frameParsed);
    QSignalSpy errorSpy(&parser, &ProtocolParser::parseError);

    QByteArray frame = ProtocolCodec::buildSetHeating(true);
    frame[frame.size() - 2] = static_cast<char>(0xAA);
    parser.appendData(frame);

    QCOMPARE(frameSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(ProtocolTests)

#include "test_protocol.moc"
