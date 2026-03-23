#pragma once

#include "devicestate.h"
#include "serialportmanager.h"

#include <QCloseEvent>
#include <QColor>
#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListView;
class QPushButton;
class QToolButton;
class QSpinBox;
class QTableWidget;
class QTextEdit;
class QTimer;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class LogFilter
    {
        All,
        Tx,
        Rx,
        Error,
        Settings,
    };

    struct LogEntry
    {
        QString line;
        QColor color;
    };

    SerialPortManager *m_serialManager = nullptr;
    DeviceStateStore *m_stateStore = nullptr;

    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QComboBox *m_dataBitsCombo = nullptr;
    QComboBox *m_parityCombo = nullptr;
    QComboBox *m_stopBitsCombo = nullptr;
    QComboBox *m_flowControlCombo = nullptr;
    QLineEdit *m_customBaudEdit = nullptr;
    QLabel *m_customBaudLabel = nullptr;
    QPushButton *m_openButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QPushButton *m_refreshPortsButton = nullptr;

    QTextEdit *m_logEdit = nullptr;
    QPushButton *m_clearLogButton = nullptr;
    QToolButton *m_showAllLogsButton = nullptr;
    QToolButton *m_showTxLogsButton = nullptr;
    QToolButton *m_showRxLogsButton = nullptr;
    QToolButton *m_showErrorLogsButton = nullptr;
    QToolButton *m_showSettingsLogsButton = nullptr;

    QLineEdit *m_sendEdit = nullptr;
    QCheckBox *m_periodicSendCheck = nullptr;
    QSpinBox *m_periodicIntervalSpin = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_clearSendButton = nullptr;
    QTimer *m_periodicSendTimer = nullptr;

    QPushButton *m_readStatusButton = nullptr;
    QCheckBox *m_heatingCheck = nullptr;
    QCheckBox *m_shutdownEnableCheck = nullptr;
    QSpinBox *m_shutdownDelaySpin = nullptr;
    QSpinBox *m_setTempSpins[4] = {nullptr, nullptr, nullptr, nullptr};
    QPushButton *m_applySettingsButton = nullptr;
    QTimer *m_settingsAckTimer = nullptr;
    QTimer *m_offlineTimer = nullptr;
    bool m_waitingForSettingsAck = false;
    DeviceState m_settingsStateBeforeAck;
    LogFilter m_logFilter = LogFilter::All;
    bool m_isOffline = true;
    QVector<LogEntry> m_logEntries;

    QTableWidget *m_channelTable = nullptr;
    QTableWidget *m_statusTable = nullptr;
    QLabel *m_connectionLabel = nullptr;

    void setupUi();
    void createConnections();
    void loadSettings();
    void saveSettings() const;
    void refreshPorts();
    void configureComboPopup(QComboBox *combo, QListView *view) const;
    SerialConfig currentSerialConfig() const;
    void updateBaudRateMode();
    void setLogFilter(LogFilter filter);
    void syncLogFilterButtons();
    void updateSendButtonsState();
    void appendLogLine(const QString &line);
    void appendLogLine(const QString &line, const QColor &color);
    bool shouldDisplayLogLine(const QString &line) const;
    void renderLogEntries();
    void updateConnectionState(bool connected);
    void applyDeviceState(const DeviceState &state);
    void handleFrame(const ParsedFrame &frame);
    QByteArray parseHexInput(const QString &text, bool *ok, QString *error) const;
    void sendManualHex();
    void updatePeriodicSendState();
    void sendCurrentSettingsBatch();
    void setSerialControlsEnabled(bool enabled);
    void handleSettingsAckTimeout();
    void handleOfflineTimeout();
    QString summarizeStateDiff(const DeviceState &before, const DeviceState &after) const;
};
