#include "mainwindow.h"

#include "protocolcodec.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QListView>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextCharFormat>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr int kOfflineTimeoutMs = 3000;
constexpr int kLeftColumnModuleWidth = 320;

QString timestampPrefix()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
}

QTableWidgetItem *makeReadOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

struct ModuleParts
{
    QWidget *root = nullptr;
    QWidget *header = nullptr;
    QWidget *frame = nullptr;
    QHBoxLayout *headerLayout = nullptr;
    QLabel *titleLabel = nullptr;
};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_serialManager(new SerialPortManager(this))
    , m_stateStore(new DeviceStateStore(this))
    , m_periodicSendTimer(new QTimer(this))
    , m_settingsAckTimer(new QTimer(this))
    , m_offlineTimer(new QTimer(this))
{
    m_periodicSendTimer->setSingleShot(false);
    m_settingsAckTimer->setSingleShot(true);
    m_offlineTimer->setSingleShot(true);

    setupUi();
    createConnections();
    refreshPorts();
    loadSettings();
    updateConnectionState(false);
    applyDeviceState(m_stateStore->state());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("DeviceLink"));
    resize(960, 760);

    auto *central = new QWidget(this);
    auto *rootLayout = new QGridLayout(central);
    rootLayout->setContentsMargins(12, 10, 12, 12);
    rootLayout->setHorizontalSpacing(18);
    rootLayout->setVerticalSpacing(10);

    const QString buttonStyle = QStringLiteral(
        "QPushButton, QToolButton {"
        " background:#FFFFFF;"
        " border:1px solid #CFCFCF;"
        " border-radius:5px;"
        " min-height:30px;"
        " padding:0 12px;"
        " color:#303030;"
        "}"
        "QPushButton:hover, QToolButton:hover { background:#F7F7F7; }"
        "QPushButton:pressed, QToolButton:pressed { background:#EFEFEF; }"
        "QPushButton:disabled, QToolButton:disabled { color:#A0A0A0; background:#F5F5F5; }"
        "QToolButton:checked { background:#E6E6E6; border-color:#BEBEBE; font-weight:600; }");

    const QString logFilterButtonBaseStyle = QStringLiteral(
        "QToolButton {"
        " background:#E2E2E2;"
        " border:1px solid #C6C6C6;"
        " border-radius:5px;"
        " min-height:30px;"
        " padding:0 12px;"
        " color:#666666;"
        " font-weight:500;"
        "}"
        "QToolButton:hover { background:#EAEAEA; border-color:#BBBBBB; color:#4A4A4A; }"
        "QToolButton:pressed { background:#DADADA; }"
        "QToolButton:checked {"
        " background:#FFFFFF;"
        " border:1px solid #B7B7B7;"
        " color:#303030;"
        " font-weight:600;"
        "}"
        "QToolButton:checked:hover { background:#FFFFFF; border-color:#A9A9A9; }"
        "QToolButton:disabled { color:#A0A0A0; background:#F2F2F2; border-color:#DDDDDD; }");

    const QString lineEditStyle = QStringLiteral(
        "QLineEdit, QTextEdit {"
        " background:#FFFFFF;"
        " border:1px solid #C9CDD3;"
        " border-radius:4px;"
        " color:#303030;"
        " padding:4px 8px;"
        " selection-background-color:#DCE8FF;"
        "}"
        "QLineEdit:hover, QTextEdit:hover { border-color:#B8BEC7; }"
        "QLineEdit:focus, QTextEdit:focus { border-color:#8FA9D6; }"
        "QLineEdit:disabled, QTextEdit:disabled { background:#F5F5F5; color:#A0A0A0; }");

    const QString comboBoxStyle = QStringLiteral(
        "QComboBox {"
        " background:#FFFFFF;"
        " border:1px solid #C9CDD3;"
        " border-radius:4px;"
        " color:#303030;"
        " min-height:28px;"
        " padding:1px 14px 1px 8px;"
        "}"
        "QComboBox:hover { border-color:#B8BEC7; }"
        "QComboBox:focus { border-color:#8FA9D6; }"
        "QComboBox:disabled { background:#F5F5F5; color:#A0A0A0; }"
        "QComboBox::drop-down {"
        " subcontrol-origin: padding;"
        " subcontrol-position: top right;"
        " width:10px;"
        " border:none;"
        " background:transparent;"
        "}"
        "QComboBox::down-arrow {"
        " image:none;"
        " width:5px;"
        " height:5px;"
        " border-right:1px solid #666666;"
        " border-bottom:1px solid #666666;"
        " margin-right:3px;"
        " margin-top:-1px;"
        "}"
        "QComboBox QAbstractItemView {"
        " background:#FFFFFF;"
        " border:1px solid #C9CDD3;"
        " padding:4px 0px;"
        " outline:0px;"
        " selection-background-color:#E9EEF8;"
        " selection-color:#202020;"
        "}"
        "QComboBox QAbstractItemView::item {"
        " min-height:22px;"
        " max-height:22px;"
        " padding:0px 10px;"
        "}");

    const QString spinBoxStyle = QStringLiteral(
        "QSpinBox {"
        " background:#FFFFFF;"
        " border:1px solid #C9CDD3;"
        " border-radius:4px;"
        " color:#303030;"
        " min-height:28px;"
        " padding:1px 18px 1px 8px;"
        "}"
        "QSpinBox:hover { border-color:#B8BEC7; }"
        "QSpinBox:focus { border-color:#8FA9D6; }"
        "QSpinBox:disabled { background:#F5F5F5; color:#A0A0A0; }"
        "QSpinBox::up-button, QSpinBox::down-button {"
        " width:14px;"
        " border:none;"
        " background:transparent;"
        " subcontrol-origin:border;"
        "}"
        "QSpinBox::up-button { subcontrol-position: top right; }"
        "QSpinBox::down-button { subcontrol-position: bottom right; }"
        "QSpinBox::up-arrow, QSpinBox::down-arrow {"
        " width:7px;"
        " height:7px;"
        "}");

    const QString periodSpinBoxStyle = QStringLiteral(
        "QSpinBox {"
        " background:#FFFFFF;"
        " border:1px solid #C9CDD3;"
        " border-radius:4px;"
        " color:#303030;"
        " min-height:28px;"
        " padding:1px 8px 1px 8px;"
        "}"
        "QSpinBox:hover { border-color:#B8BEC7; }"
        "QSpinBox:focus { border-color:#8FA9D6; }"
        "QSpinBox:disabled { background:#F5F5F5; color:#A0A0A0; }"
        "QSpinBox::up-button, QSpinBox::down-button {"
        " width:0px;"
        " height:0px;"
        " border:none;"
        " background:transparent;"
        "}"
        "QSpinBox::up-arrow, QSpinBox::down-arrow {"
        " width:0px;"
        " height:0px;"
        " image:none;"
        "}");

    auto createModule = [central](const QString &title) -> ModuleParts {
        ModuleParts parts;
        parts.root = new QWidget(central);
        auto *outerLayout = new QVBoxLayout(parts.root);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->setSpacing(2);

        parts.header = new QWidget(parts.root);
        parts.headerLayout = new QHBoxLayout(parts.header);
        parts.headerLayout->setContentsMargins(0, 0, 0, 0);
        parts.headerLayout->setSpacing(8);
        parts.titleLabel = new QLabel(title, parts.header);
        parts.titleLabel->setStyleSheet(QStringLiteral(
            "font-size:12px;"
            "font-weight:600;"
            "color:#3A3A3A;"
            "background:transparent;"
            "border:none;"));
        parts.headerLayout->addWidget(parts.titleLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
        parts.headerLayout->addStretch();
        outerLayout->addWidget(parts.header);

        parts.frame = new QWidget(parts.root);
        parts.frame->setStyleSheet(QStringLiteral(
            "background:#ECECEC;"
            "border:1px solid #DDDDDD;"
            "border-radius:4px;"));
        outerLayout->addWidget(parts.frame);
        return parts;
    };

    const ModuleParts logModule = createModule(QStringLiteral("数据日志"));
    auto *logLayout = new QVBoxLayout(logModule.frame);
    logLayout->setContentsMargins(12, 18, 12, 12);
    logLayout->setSpacing(8);
    auto *logHeaderLayout = new QHBoxLayout();
    m_showAllLogsButton = new QToolButton(logModule.frame);
    m_showTxLogsButton = new QToolButton(logModule.frame);
    m_showRxLogsButton = new QToolButton(logModule.frame);
    m_showErrorLogsButton = new QToolButton(logModule.frame);
    m_showSettingsLogsButton = new QToolButton(logModule.frame);
    const QList<QToolButton *> logButtons = {
        m_showAllLogsButton, m_showTxLogsButton, m_showRxLogsButton, m_showErrorLogsButton, m_showSettingsLogsButton};
    const QStringList labels = {
        QStringLiteral("全部"), QStringLiteral("发送"), QStringLiteral("接收"), QStringLiteral("异常"), QStringLiteral("设置")};
    const QStringList activeColors = {
        QStringLiteral("#4B5563"), QStringLiteral("#2563EB"), QStringLiteral("#0F766E"), QStringLiteral("#B91C1C"),
        QStringLiteral("#7C3AED")};
    for (int i = 0; i < logButtons.size(); ++i) {
        logButtons[i]->setText(labels[i]);
        logButtons[i]->setCheckable(true);
        logButtons[i]->setStyleSheet(
            logFilterButtonBaseStyle
            + QStringLiteral(
                  "QToolButton:checked {"
                  " background:#FFFFFF;"
                  " border:1px solid %1;"
                  " color:%1;"
                  " font-weight:600;"
                  "}"
                  "QToolButton:checked:hover {"
                  " background:#FFFFFF;"
                  " border:1px solid %1;"
                  " color:%1;"
                  "}").arg(activeColors[i]));
    }
    m_showAllLogsButton->setChecked(true);
    m_showAllLogsButton->setAutoExclusive(true);
    m_showTxLogsButton->setAutoExclusive(true);
    m_showRxLogsButton->setAutoExclusive(true);
    m_showErrorLogsButton->setAutoExclusive(true);
    m_showSettingsLogsButton->setAutoExclusive(true);
    logHeaderLayout->addWidget(m_showAllLogsButton);
    logHeaderLayout->addWidget(m_showTxLogsButton);
    logHeaderLayout->addWidget(m_showRxLogsButton);
    logHeaderLayout->addWidget(m_showErrorLogsButton);
    logHeaderLayout->addWidget(m_showSettingsLogsButton);
    logHeaderLayout->addStretch();
    m_logEdit = new QTextEdit(logModule.frame);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMinimumHeight(150);
    m_logEdit->setStyleSheet(lineEditStyle);
    m_clearLogButton = new QPushButton(QStringLiteral("接收清空"), logModule.frame);
    m_clearLogButton->setStyleSheet(buttonStyle);
    logLayout->addLayout(logHeaderLayout);
    logLayout->addWidget(m_logEdit);
    logLayout->addWidget(m_clearLogButton, 0, Qt::AlignLeft);

    const ModuleParts serialModule = createModule(QStringLiteral("串口设置"));
    serialModule.root->setMaximumWidth(kLeftColumnModuleWidth);
    m_refreshPortsButton = new QPushButton(QStringLiteral("刷新"), serialModule.header);
    m_refreshPortsButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        " background:transparent;"
        " border:none;"
        " color:#7B7B7B;"
        " min-height:0px;"
        " padding:0px;"
        "}"
        "QPushButton:hover { color:#505050; }"
        "QPushButton:pressed { color:#303030; }"));
    m_connectionLabel = new QLabel(QStringLiteral("未连接"), serialModule.header);
    m_connectionLabel->setStyleSheet(QStringLiteral(
        "color:#8A8A8A;"
        "font-weight:600;"
        "background:transparent;"
        "border:none;"));
    serialModule.headerLayout->setSpacing(10);
    serialModule.headerLayout->addWidget(m_refreshPortsButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    serialModule.headerLayout->addWidget(m_connectionLabel, 0, Qt::AlignRight | Qt::AlignVCenter);

    auto *serialLayout = new QGridLayout(serialModule.frame);
    serialLayout->setContentsMargins(12, 18, 12, 12);
    serialLayout->setSpacing(8);
    m_portCombo = new QComboBox(serialModule.frame);
    m_baudCombo = new QComboBox(serialModule.frame);
    m_dataBitsCombo = new QComboBox(serialModule.frame);
    m_parityCombo = new QComboBox(serialModule.frame);
    m_stopBitsCombo = new QComboBox(serialModule.frame);
    m_flowControlCombo = new QComboBox(serialModule.frame);
    m_customBaudEdit = new QLineEdit(serialModule.frame);
    auto *portListView = new QListView(serialModule.frame);
    auto *baudListView = new QListView(serialModule.frame);
    auto *dataBitsListView = new QListView(serialModule.frame);
    auto *parityListView = new QListView(serialModule.frame);
    auto *stopBitsListView = new QListView(serialModule.frame);
    auto *flowControlListView = new QListView(serialModule.frame);
    m_openButton = new QPushButton(QStringLiteral("打开"), serialModule.frame);
    m_closeButton = new QPushButton(QStringLiteral("关闭"), serialModule.frame);
    m_openButton->setStyleSheet(buttonStyle);
    m_closeButton->setStyleSheet(buttonStyle);
    m_openButton->setFixedWidth(92);
    m_closeButton->setFixedWidth(92);
    m_portCombo->setStyleSheet(comboBoxStyle);
    m_baudCombo->setStyleSheet(comboBoxStyle);
    m_dataBitsCombo->setStyleSheet(comboBoxStyle);
    m_parityCombo->setStyleSheet(comboBoxStyle);
    m_stopBitsCombo->setStyleSheet(comboBoxStyle);
    m_flowControlCombo->setStyleSheet(comboBoxStyle);
    m_customBaudEdit->setStyleSheet(lineEditStyle);
    m_customBaudEdit->setPlaceholderText(QStringLiteral("输入波特率"));
    m_customBaudEdit->setValidator(new QIntValidator(1, 4000000, m_customBaudEdit));
    m_portCombo->setView(portListView);
    m_baudCombo->setView(baudListView);
    m_dataBitsCombo->setView(dataBitsListView);
    m_parityCombo->setView(parityListView);
    m_stopBitsCombo->setView(stopBitsListView);
    m_flowControlCombo->setView(flowControlListView);
    const QString serialLabelStyle = QStringLiteral(
        "color:#3A3A3A;"
        "background:transparent;"
        "border:none;");

    m_baudCombo->addItems({QStringLiteral("9600"), QStringLiteral("19200"), QStringLiteral("38400"),
                           QStringLiteral("57600"), QStringLiteral("115200"), QStringLiteral("其他")});
    m_dataBitsCombo->addItem(QStringLiteral("8"), QSerialPort::Data8);
    m_dataBitsCombo->addItem(QStringLiteral("7"), QSerialPort::Data7);
    m_parityCombo->addItem(QStringLiteral("none"), QSerialPort::NoParity);
    m_parityCombo->addItem(QStringLiteral("even"), QSerialPort::EvenParity);
    m_parityCombo->addItem(QStringLiteral("odd"), QSerialPort::OddParity);
    m_stopBitsCombo->addItem(QStringLiteral("1"), QSerialPort::OneStop);
    m_stopBitsCombo->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
    m_flowControlCombo->addItem(QStringLiteral("none"), QSerialPort::NoFlowControl);
    m_flowControlCombo->addItem(QStringLiteral("RTS/CTS"), QSerialPort::HardwareControl);
    m_flowControlCombo->addItem(QStringLiteral("XON/XOFF"), QSerialPort::SoftwareControl);
    configureComboPopup(m_portCombo, portListView);
    configureComboPopup(m_baudCombo, baudListView);
    configureComboPopup(m_dataBitsCombo, dataBitsListView);
    configureComboPopup(m_parityCombo, parityListView);
    configureComboPopup(m_stopBitsCombo, stopBitsListView);
    configureComboPopup(m_flowControlCombo, flowControlListView);

    auto *portLabel = new QLabel(QStringLiteral("串口号"), serialModule.frame);
    auto *baudLabel = new QLabel(QStringLiteral("波特率"), serialModule.frame);
    auto *parityLabel = new QLabel(QStringLiteral("校验位"), serialModule.frame);
    auto *dataBitsLabel = new QLabel(QStringLiteral("数据位"), serialModule.frame);
    auto *stopBitsLabel = new QLabel(QStringLiteral("停止位"), serialModule.frame);
    auto *flowControlLabel = new QLabel(QStringLiteral("流控"), serialModule.frame);
    m_customBaudLabel = new QLabel(QStringLiteral("自定义波特率"), serialModule.frame);
    auto *serialButtonRow = new QWidget(serialModule.frame);
    serialButtonRow->setStyleSheet(QStringLiteral(
        "background:transparent;"
        "border:none;"
        "border-radius:0px;"));
    auto *serialButtonLayout = new QHBoxLayout(serialButtonRow);
    serialButtonLayout->setContentsMargins(0, 0, 0, 0);
    serialButtonLayout->setSpacing(16);
    serialButtonLayout->addStretch();
    serialButtonLayout->addWidget(m_openButton);
    serialButtonLayout->addWidget(m_closeButton);
    serialButtonLayout->addStretch();
    portLabel->setStyleSheet(serialLabelStyle);
    baudLabel->setStyleSheet(serialLabelStyle);
    parityLabel->setStyleSheet(serialLabelStyle);
    dataBitsLabel->setStyleSheet(serialLabelStyle);
    stopBitsLabel->setStyleSheet(serialLabelStyle);
    flowControlLabel->setStyleSheet(serialLabelStyle);
    m_customBaudLabel->setStyleSheet(serialLabelStyle);

    serialLayout->addWidget(portLabel, 0, 0);
    serialLayout->addWidget(m_portCombo, 0, 1);
    serialLayout->addWidget(baudLabel, 1, 0);
    serialLayout->addWidget(m_baudCombo, 1, 1);
    serialLayout->addWidget(parityLabel, 2, 0);
    serialLayout->addWidget(m_parityCombo, 2, 1);
    serialLayout->addWidget(dataBitsLabel, 3, 0);
    serialLayout->addWidget(m_dataBitsCombo, 3, 1);
    serialLayout->addWidget(stopBitsLabel, 4, 0);
    serialLayout->addWidget(m_stopBitsCombo, 4, 1);
    serialLayout->addWidget(flowControlLabel, 5, 0);
    serialLayout->addWidget(m_flowControlCombo, 5, 1);
    serialLayout->addWidget(m_customBaudLabel, 6, 0);
    serialLayout->addWidget(m_customBaudEdit, 6, 1);
    serialLayout->addWidget(serialButtonRow, 7, 0, 1, 2);
    serialLayout->setColumnStretch(0, 0);
    serialLayout->setColumnStretch(1, 1);
    updateBaudRateMode();

    const ModuleParts sendModule = createModule(QStringLiteral("调试发送"));
    auto *sendTitleHint = new QLabel(
        QStringLiteral("仅用于协议调试；业务控制优先使用左下“设置”和右侧“读取状态”按钮"),
        sendModule.header);
    sendTitleHint->setStyleSheet(QStringLiteral(
        "font-size:12px;"
        "color:#8A8F98;"
        "background:transparent;"
        "border:none;"));
    sendModule.headerLayout->addWidget(sendTitleHint, 0, Qt::AlignRight | Qt::AlignVCenter);

    auto *sendLayout = new QGridLayout(sendModule.frame);
    sendLayout->setContentsMargins(18, 14, 18, 14);
    sendLayout->setHorizontalSpacing(6);
    sendLayout->setVerticalSpacing(8);

    m_sendEdit = new QLineEdit(sendModule.frame);
    m_sendEdit->setPlaceholderText(QString());
    m_sendEdit->setPlaceholderText(QStringLiteral("调试发送 HEX，支持带空格连续输入，如 24 24 04... 或 242404..."));
    m_sendEdit->setFixedHeight(32);
    m_sendEdit->setStyleSheet(lineEditStyle);
    m_periodicSendCheck = new QCheckBox(QStringLiteral("定时发送"), sendModule.frame);
    m_periodicSendCheck->setStyleSheet(QStringLiteral(
        "QCheckBox {"
        " background:transparent;"
        " border:none;"
        " color:#4A4A4A;"
        " spacing:6px;"
        " padding:0px;"
        "}"));
    m_periodicIntervalSpin = new QSpinBox(sendModule.frame);
    m_periodicIntervalSpin->setRange(10, 600000);
    m_periodicIntervalSpin->setValue(1000);
    m_periodicIntervalSpin->setFixedSize(84, 28);
    m_periodicIntervalSpin->setStyleSheet(periodSpinBoxStyle);
    auto *sendPeriodLabel = new QLabel(QStringLiteral("周期"), sendModule.frame);
    auto *sendMsLabel = new QLabel(QStringLiteral("ms"), sendModule.frame);
    const QString plainSendTextStyle = QStringLiteral(
        "background:transparent;"
        "border:none;"
        "color:#4A4A4A;");
    sendPeriodLabel->setStyleSheet(plainSendTextStyle);
    sendMsLabel->setStyleSheet(plainSendTextStyle);
    m_sendButton = new QPushButton(QStringLiteral("发送"), sendModule.frame);
    m_clearSendButton = new QPushButton(QStringLiteral("清空"), sendModule.frame);
    m_sendButton->setStyleSheet(buttonStyle);
    m_clearSendButton->setStyleSheet(buttonStyle);
    m_sendButton->setFixedSize(72, 28);
    m_clearSendButton->setFixedSize(72, 28);

    sendLayout->addWidget(m_sendEdit, 0, 0, 1, 7);
    sendLayout->addWidget(m_periodicSendCheck, 1, 0, 1, 2, Qt::AlignLeft);
    sendLayout->addWidget(sendPeriodLabel, 1, 2, 1, 1, Qt::AlignRight);
    sendLayout->addWidget(m_periodicIntervalSpin, 1, 3);
    sendLayout->addWidget(sendMsLabel, 1, 4, 1, 1, Qt::AlignLeft);
    sendLayout->addWidget(m_sendButton, 1, 5);
    sendLayout->addWidget(m_clearSendButton, 1, 6);
    sendLayout->setColumnStretch(0, 1);
    sendLayout->setColumnStretch(1, 0);
    sendLayout->setColumnMinimumWidth(2, 28);
    sendLayout->setColumnMinimumWidth(4, 34);
    sendLayout->setColumnMinimumWidth(5, 72);
    sendLayout->setColumnMinimumWidth(6, 72);
    updateSendButtonsState();

    const ModuleParts controlModule = createModule(QStringLiteral("控制与设置"));
    controlModule.root->setMaximumWidth(kLeftColumnModuleWidth);
    auto *controlLayout = new QGridLayout(controlModule.frame);
    controlLayout->setContentsMargins(12, 18, 12, 12);
    controlLayout->setSpacing(8);
    const QString controlTextStyle = QStringLiteral(
        "background:transparent;"
        "border:none;"
        "color:#3A3A3A;");
    m_heatingCheck = new QCheckBox(QStringLiteral("开启加热"), controlModule.frame);
    m_shutdownEnableCheck = new QCheckBox(QStringLiteral("开启延时关机"), controlModule.frame);
    m_heatingCheck->setStyleSheet(QStringLiteral(
        "QCheckBox {"
        " background:transparent;"
        " border:none;"
        " color:#3A3A3A;"
        " spacing:6px;"
        "}"));
    m_shutdownEnableCheck->setStyleSheet(QStringLiteral(
        "QCheckBox {"
        " background:transparent;"
        " border:none;"
        " color:#3A3A3A;"
        " spacing:6px;"
        "}"));
    m_shutdownDelaySpin = new QSpinBox(controlModule.frame);
    m_shutdownDelaySpin->setRange(0, 24 * 60 * 60);
    m_shutdownDelaySpin->setStyleSheet(spinBoxStyle);
    m_applySettingsButton = new QPushButton(QStringLiteral("设置"), controlModule.frame);
    m_applySettingsButton->setStyleSheet(buttonStyle);
    auto *shutdownDelayLabel = new QLabel(QStringLiteral("延时关机时间"), controlModule.frame);
    shutdownDelayLabel->setStyleSheet(controlTextStyle);

    controlLayout->addWidget(m_heatingCheck, 1, 0);
    controlLayout->addWidget(m_shutdownEnableCheck, 4, 0);
    controlLayout->addWidget(shutdownDelayLabel, 0, 1);
    controlLayout->addWidget(m_shutdownDelaySpin, 0, 2);

    for (int i = 0; i < 4; ++i) {
        auto *label = new QLabel(QStringLiteral("通道%1温度").arg(i + 1), controlModule.frame);
        label->setStyleSheet(controlTextStyle);
        auto *spin = new QSpinBox(controlModule.frame);
        spin->setRange(-1000, 5000);
        spin->setStyleSheet(spinBoxStyle);
        m_setTempSpins[i] = spin;
        controlLayout->addWidget(label, 1 + i, 1);
        controlLayout->addWidget(spin, 1 + i, 2);
    }
    controlLayout->addWidget(m_applySettingsButton, 5, 0, 1, 2, Qt::AlignLeft);
    controlLayout->setColumnStretch(2, 1);

    const ModuleParts measureModule = createModule(QStringLiteral("测量数据"));
    auto *measureOuterLayout = new QVBoxLayout(measureModule.frame);
    measureOuterLayout->setContentsMargins(12, 18, 12, 12);
    measureOuterLayout->setSpacing(8);
    auto *measureHeaderLayout = new QHBoxLayout();
    measureHeaderLayout->addStretch();
    m_readStatusButton = new QPushButton(QStringLiteral("读取状态"), measureModule.frame);
    m_readStatusButton->setStyleSheet(buttonStyle);
    measureHeaderLayout->addWidget(m_readStatusButton);

    auto *measureTablesLayout = new QHBoxLayout();
    m_channelTable = new QTableWidget(4, 3, measureModule.frame);
    m_channelTable->setHorizontalHeaderLabels(
        {QStringLiteral("加热通道"), QStringLiteral("设置值"), QStringLiteral("测量值")});
    m_channelTable->setShowGrid(true);
    m_channelTable->setGridStyle(Qt::SolidLine);
    m_channelTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_channelTable->verticalHeader()->setVisible(true);
    m_channelTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_channelTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_channelTable->setFocusPolicy(Qt::NoFocus);
    m_channelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_channelTable->setAlternatingRowColors(false);
    m_channelTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background:#FFFFFF; gridline-color: #C9CED6; border: 1px solid #C9CED6; }"
        "QHeaderView::section { "
        "background: #F7F7F7; "
        "border: 0px; }"));

    m_statusTable = new QTableWidget(4, 2, measureModule.frame);
    m_statusTable->setHorizontalHeaderLabels({QStringLiteral("状态名称"), QStringLiteral("当前值")});
    m_statusTable->setShowGrid(true);
    m_statusTable->setGridStyle(Qt::SolidLine);
    m_statusTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_statusTable->verticalHeader()->setVisible(true);
    m_statusTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_statusTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_statusTable->setFocusPolicy(Qt::NoFocus);
    m_statusTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statusTable->setAlternatingRowColors(false);
    m_statusTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background:#FFFFFF; gridline-color: #C9CED6; border: 1px solid #C9CED6; }"
        "QHeaderView::section { "
        "background: #F7F7F7; "
        "border: 0px; }"));

    for (int row = 0; row < 4; ++row) {
        m_channelTable->setRowHeight(row, 34);
        m_statusTable->setRowHeight(row, 34);
    }

    measureTablesLayout->addWidget(m_channelTable, 1);
    measureTablesLayout->addWidget(m_statusTable, 2);
    measureOuterLayout->addLayout(measureHeaderLayout);
    measureOuterLayout->addLayout(measureTablesLayout);

    rootLayout->addWidget(logModule.root, 0, 0, 1, 2);
    rootLayout->addWidget(serialModule.root, 1, 0);
    rootLayout->addWidget(sendModule.root, 1, 1, Qt::AlignVCenter);
    rootLayout->addWidget(controlModule.root, 2, 0);
    rootLayout->addWidget(measureModule.root, 2, 1);
    rootLayout->setColumnStretch(0, 0);
    rootLayout->setColumnStretch(1, 1);
    rootLayout->setRowStretch(0, 1);
    rootLayout->setRowStretch(2, 1);

    setCentralWidget(central);
}

void MainWindow::createConnections()
{
    connect(m_clearLogButton, &QPushButton::clicked, m_logEdit, &QTextEdit::clear);
    connect(m_clearSendButton, &QPushButton::clicked, m_sendEdit, &QLineEdit::clear);
    connect(m_refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::sendManualHex);
    connect(m_sendEdit, &QLineEdit::textChanged, this, [this](const QString &) { updateSendButtonsState(); });
    connect(m_baudCombo, &QComboBox::currentTextChanged, this, [this](const QString &) { updateBaudRateMode(); });
    connect(m_periodicSendCheck, &QCheckBox::toggled, this, &MainWindow::updatePeriodicSendState);
    connect(m_periodicIntervalSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        updatePeriodicSendState();
    });
    connect(m_periodicSendTimer, &QTimer::timeout, this, &MainWindow::sendManualHex);
    connect(m_settingsAckTimer, &QTimer::timeout, this, &MainWindow::handleSettingsAckTimeout);
    connect(m_offlineTimer, &QTimer::timeout, this, &MainWindow::handleOfflineTimeout);
    connect(m_showAllLogsButton, &QToolButton::clicked, this, [this]() { setLogFilter(LogFilter::All); });
    connect(m_showTxLogsButton, &QToolButton::clicked, this, [this]() { setLogFilter(LogFilter::Tx); });
    connect(m_showRxLogsButton, &QToolButton::clicked, this, [this]() { setLogFilter(LogFilter::Rx); });
    connect(m_showErrorLogsButton, &QToolButton::clicked, this, [this]() { setLogFilter(LogFilter::Error); });
    connect(m_showSettingsLogsButton, &QToolButton::clicked, this, [this]() { setLogFilter(LogFilter::Settings); });

    connect(m_openButton, &QPushButton::clicked, this, [this]() {
        refreshPorts();
        const SerialConfig config = currentSerialConfig();
        if (config.portName.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("串口"), QStringLiteral("未找到可用串口"));
            return;
        }
        if (config.baudRate <= 0) {
            QMessageBox::warning(this, QStringLiteral("串口"), QStringLiteral("请输入有效的自定义波特率"));
            return;
        }
        const bool opened = m_serialManager->openPort(config);
        updateConnectionState(opened);
        setSerialControlsEnabled(!opened);
    });

    connect(m_closeButton, &QPushButton::clicked, this, [this]() {
        m_serialManager->closePort();
        updateConnectionState(false);
        setSerialControlsEnabled(true);
    });

    connect(m_serialManager, &SerialPortManager::logMessage, this, [this](const QString &message) {
        appendLogLine(message);
    });

    connect(m_serialManager, &SerialPortManager::portError, this, [this](const QString &message) {
        appendLogLine(QStringLiteral("ERROR: %1").arg(message));
        updateConnectionState(false);
        setSerialControlsEnabled(true);
        QMessageBox::warning(this, QStringLiteral("串口错误"), message);
    });

    connect(m_serialManager, &SerialPortManager::frameReceived, this, &MainWindow::handleFrame);
    connect(m_stateStore, &DeviceStateStore::stateChanged, this, &MainWindow::applyDeviceState);

    connect(m_applySettingsButton, &QPushButton::clicked, this, &MainWindow::sendCurrentSettingsBatch);
    connect(m_readStatusButton, &QPushButton::clicked, this, [this]() {
        m_serialManager->sendBytes(ProtocolCodec::buildReadActualTemperatures());
    });
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("DeviceLink"), QStringLiteral("DeviceLink"));
    m_portCombo->setCurrentText(settings.value(QStringLiteral("serial/portName")).toString());
    const QString baudMode = settings.value(QStringLiteral("serial/baudMode"), QStringLiteral("preset")).toString();
    const QString baudText = settings.value(QStringLiteral("serial/baudRate"), QStringLiteral("9600")).toString();
    if (baudMode == QStringLiteral("custom")) {
        m_baudCombo->setCurrentText(QStringLiteral("其他"));
    } else {
        m_baudCombo->setCurrentText(baudText);
    }
    m_customBaudEdit->setText(settings.value(QStringLiteral("serial/customBaudRate")).toString());
    m_dataBitsCombo->setCurrentIndex(settings.value(QStringLiteral("serial/dataBitsIndex"), 0).toInt());
    m_parityCombo->setCurrentIndex(settings.value(QStringLiteral("serial/parityIndex"), 0).toInt());
    m_stopBitsCombo->setCurrentIndex(settings.value(QStringLiteral("serial/stopBitsIndex"), 0).toInt());
    m_flowControlCombo->setCurrentIndex(settings.value(QStringLiteral("serial/flowControlIndex"), 0).toInt());
    updateBaudRateMode();

    m_sendEdit->setText(settings.value(QStringLiteral("send/hex")).toString());
    m_periodicSendCheck->setChecked(settings.value(QStringLiteral("send/periodic"), false).toBool());
    m_periodicIntervalSpin->setValue(settings.value(QStringLiteral("send/intervalMs"), 1000).toInt());
    setLogFilter(static_cast<LogFilter>(settings.value(QStringLiteral("log/filter"), static_cast<int>(LogFilter::All)).toInt()));
    updateSendButtonsState();

    m_heatingCheck->setChecked(settings.value(QStringLiteral("control/heating"), false).toBool());
    m_shutdownEnableCheck->setChecked(settings.value(QStringLiteral("control/shutdownEnabled"), false).toBool());
    m_shutdownDelaySpin->setValue(settings.value(QStringLiteral("control/shutdownDelay"), 0).toInt());
    for (int i = 0; i < 4; ++i) {
        m_setTempSpins[i]->setValue(settings.value(QStringLiteral("control/setTemp%1").arg(i), 0).toInt());
    }
}

void MainWindow::saveSettings() const
{
    QSettings settings(QStringLiteral("DeviceLink"), QStringLiteral("DeviceLink"));
    settings.setValue(QStringLiteral("serial/portName"), m_portCombo->currentText());
    settings.setValue(QStringLiteral("serial/baudMode"),
                      m_baudCombo->currentText() == QStringLiteral("其他") ? QStringLiteral("custom")
                                                                           : QStringLiteral("preset"));
    settings.setValue(QStringLiteral("serial/baudRate"),
                      m_baudCombo->currentText() == QStringLiteral("其他") ? m_customBaudEdit->text()
                                                                           : m_baudCombo->currentText());
    settings.setValue(QStringLiteral("serial/customBaudRate"), m_customBaudEdit->text());
    settings.setValue(QStringLiteral("serial/dataBitsIndex"), m_dataBitsCombo->currentIndex());
    settings.setValue(QStringLiteral("serial/parityIndex"), m_parityCombo->currentIndex());
    settings.setValue(QStringLiteral("serial/stopBitsIndex"), m_stopBitsCombo->currentIndex());
    settings.setValue(QStringLiteral("serial/flowControlIndex"), m_flowControlCombo->currentIndex());

    settings.setValue(QStringLiteral("send/hex"), m_sendEdit->text());
    settings.setValue(QStringLiteral("send/periodic"), m_periodicSendCheck->isChecked());
    settings.setValue(QStringLiteral("send/intervalMs"), m_periodicIntervalSpin->value());
    settings.setValue(QStringLiteral("log/filter"), static_cast<int>(m_logFilter));

    settings.setValue(QStringLiteral("control/heating"), m_heatingCheck->isChecked());
    settings.setValue(QStringLiteral("control/shutdownEnabled"), m_shutdownEnableCheck->isChecked());
    settings.setValue(QStringLiteral("control/shutdownDelay"), m_shutdownDelaySpin->value());
    for (int i = 0; i < 4; ++i) {
        settings.setValue(QStringLiteral("control/setTemp%1").arg(i), m_setTempSpins[i]->value());
    }
}

void MainWindow::refreshPorts()
{
    const QString current = m_portCombo->currentText();
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        m_portCombo->addItem(port.portName());
    }
    const int index = m_portCombo->findText(current);
    if (index >= 0) {
        m_portCombo->setCurrentIndex(index);
    }
}

void MainWindow::configureComboPopup(QComboBox *combo, QListView *view) const
{
    if (!combo || !view) {
        return;
    }

    constexpr int kComboItemHeight = 22;
    constexpr int kComboPopupPadding = 8;
    constexpr int kComboPopupMaxHeight = 176;

    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setSpacing(0);
    view->setUniformItemSizes(true);

    const int popupHeight = qMin(kComboPopupMaxHeight, combo->count() * kComboItemHeight + kComboPopupPadding);
    view->setMinimumHeight(popupHeight);
    view->setMaximumHeight(kComboPopupMaxHeight);
    if (combo == m_portCombo) {
        view->setMinimumWidth(320);
    }
}

void MainWindow::updateBaudRateMode()
{
    const bool customMode = m_baudCombo->currentText() == QStringLiteral("其他");
    if (m_customBaudLabel) {
        m_customBaudLabel->setVisible(customMode);
    }
    if (m_customBaudEdit) {
        m_customBaudEdit->setVisible(customMode);
        m_customBaudEdit->setEnabled(customMode && m_baudCombo->isEnabled());
    }
}

void MainWindow::setLogFilter(LogFilter filter)
{
    m_logFilter = filter;
    syncLogFilterButtons();
    renderLogEntries();
}

void MainWindow::syncLogFilterButtons()
{
    if (!m_showAllLogsButton) {
        return;
    }

    m_showAllLogsButton->setChecked(m_logFilter == LogFilter::All);
    m_showTxLogsButton->setChecked(m_logFilter == LogFilter::Tx);
    m_showRxLogsButton->setChecked(m_logFilter == LogFilter::Rx);
    m_showErrorLogsButton->setChecked(m_logFilter == LogFilter::Error);
    m_showSettingsLogsButton->setChecked(m_logFilter == LogFilter::Settings);
}

void MainWindow::updateSendButtonsState()
{
    if (!m_clearSendButton || !m_sendEdit) {
        return;
    }

    m_clearSendButton->setEnabled(!m_sendEdit->text().trimmed().isEmpty());
}

SerialConfig MainWindow::currentSerialConfig() const
{
    SerialConfig config;
    config.portName = m_portCombo->currentText();
    config.baudRate = (m_baudCombo->currentText() == QStringLiteral("其他"))
                          ? m_customBaudEdit->text().toInt()
                          : m_baudCombo->currentText().toInt();
    config.dataBits = m_dataBitsCombo->currentData().toInt();
    config.parity = m_parityCombo->currentData().toInt();
    config.stopBits = m_stopBitsCombo->currentData().toInt();
    config.flowControl = m_flowControlCombo->currentData().toInt();
    return config;
}

void MainWindow::appendLogLine(const QString &line)
{
    if (!shouldDisplayLogLine(line)) {
        return;
    }
    QColor color(QStringLiteral("#1F2937"));
    if (line.startsWith(QStringLiteral("TX:"))) {
        color = QColor(QStringLiteral("#2563EB"));
    } else if (line.startsWith(QStringLiteral("RX:"))) {
        color = QColor(QStringLiteral("#0F766E"));
    } else if (line.startsWith(QStringLiteral("ERROR:")) || line.startsWith(QStringLiteral("PARSE ERROR:"))) {
        color = QColor(QStringLiteral("#B91C1C"));
    } else if (line.startsWith(QStringLiteral("SETTINGS:"))) {
        color = QColor(QStringLiteral("#7C3AED"));
    } else if (line.startsWith(QStringLiteral("INFO:"))) {
        color = QColor(QStringLiteral("#6B7280"));
    }
    appendLogLine(line, color);
}

void MainWindow::appendLogLine(const QString &line, const QColor &color)
{
    m_logEntries.push_back({line, color});
    if (!shouldDisplayLogLine(line)) {
        return;
    }
    QTextCharFormat format;
    format.setForeground(color);
    QTextCursor cursor(m_logEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(QStringLiteral("[%1] %2\n").arg(timestampPrefix(), line), format);
    m_logEdit->setTextCursor(cursor);
    m_logEdit->ensureCursorVisible();
}

bool MainWindow::shouldDisplayLogLine(const QString &line) const
{
    switch (m_logFilter) {
    case LogFilter::All:
        return true;
    case LogFilter::Tx:
        return line.startsWith(QStringLiteral("TX:"));
    case LogFilter::Rx:
        return line.startsWith(QStringLiteral("RX:"));
    case LogFilter::Error:
        return line.startsWith(QStringLiteral("ERROR:")) || line.startsWith(QStringLiteral("PARSE ERROR:"));
    case LogFilter::Settings:
        return line.startsWith(QStringLiteral("SETTINGS:"));
    }
    return false;
}

void MainWindow::renderLogEntries()
{
    m_logEdit->clear();
    for (const LogEntry &entry : m_logEntries) {
        if (!shouldDisplayLogLine(entry.line)) {
            continue;
        }
        QTextCharFormat format;
        format.setForeground(entry.color);
        QTextCursor cursor(m_logEdit->document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QStringLiteral("[%1] %2\n").arg(timestampPrefix(), entry.line), format);
    }
    m_logEdit->moveCursor(QTextCursor::End);
}

void MainWindow::updateConnectionState(bool connected)
{
    m_connectionLabel->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
    const QString color = connected ? QStringLiteral("#0B8F3A") : QStringLiteral("#8A8A8A");
    m_connectionLabel->setStyleSheet(QStringLiteral("color:%1;font-weight:600;").arg(color));
    m_openButton->setEnabled(!connected);
    m_closeButton->setEnabled(connected);
}

void MainWindow::applyDeviceState(const DeviceState &state)
{
    if (state.updatedAt.isValid()) {
        m_isOffline = false;
        m_offlineTimer->start(kOfflineTimeoutMs);
    }
    m_heatingCheck->setChecked(state.heatingEnabled);
    m_shutdownEnableCheck->setChecked(state.shutdownEnabled);
    m_shutdownDelaySpin->setValue(state.shutdownDelaySec);

    for (int i = 0; i < 4; ++i) {
        m_setTempSpins[i]->setValue(state.setTemps[i]);
        m_channelTable->setVerticalHeaderItem(i, makeReadOnlyItem(QString::number(i + 1)));
        auto *channelItem = makeReadOnlyItem(QStringLiteral("通道%1").arg(i));
        channelItem->setTextAlignment(Qt::AlignCenter);
        m_channelTable->setItem(i, 0, channelItem);
        auto *setItem = makeReadOnlyItem(QString::number(state.setTemps[i]));
        setItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_channelTable->setItem(i, 1, setItem);
        auto *actualItem = makeReadOnlyItem(QString::number(state.actualTemps[i], 'f', 1));
        actualItem->setForeground(m_isOffline ? QColor(QStringLiteral("#9CA3AF")) : QColor(QStringLiteral("#1D4ED8")));
        actualItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_channelTable->setItem(i, 2, actualItem);
    }

    const QString labels[4] = {
        QStringLiteral("加热状态（1：开启/0：关闭）"),
        QStringLiteral("延时关机时间（S）"),
        QStringLiteral("延时关机功能（1：开启/0：关闭）"),
        QStringLiteral("延时关机剩余时间（S）"),
    };
    const QString values[4] = {
        m_isOffline ? QStringLiteral("离线") : (state.heatingEnabled ? QStringLiteral("1（开启）") : QStringLiteral("0（关闭）")),
        QString::number(state.shutdownDelaySec),
        m_isOffline ? QStringLiteral("离线") : (state.shutdownEnabled ? QStringLiteral("1（开启）") : QStringLiteral("0（关闭）")),
        state.shutdownRemainingRaw == 0x00FF ? QStringLiteral("--") : QString::number(state.shutdownRemainingRaw),
    };

    for (int row = 0; row < 4; ++row) {
        m_statusTable->setVerticalHeaderItem(row, makeReadOnlyItem(QString::number(row + 1)));
        m_statusTable->setItem(row, 0, makeReadOnlyItem(labels[row]));
        auto *valueItem = makeReadOnlyItem(values[row]);
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (m_isOffline) {
            valueItem->setForeground(QColor(QStringLiteral("#B91C1C")));
        } else if (row == 0 || row == 2) {
            valueItem->setForeground(state.heatingEnabled || row == 2
                                         ? QColor((row == 0 ? state.heatingEnabled : state.shutdownEnabled)
                                                      ? QStringLiteral("#0B8F3A")
                                                      : QStringLiteral("#8A8A8A"))
                                         : QColor(QStringLiteral("#8A8A8A")));
        } else {
            valueItem->setForeground(QColor(QStringLiteral("#1D4ED8")));
        }
        m_statusTable->setItem(row, 1, valueItem);
    }
}

void MainWindow::handleFrame(const ParsedFrame &frame)
{
    if (frame.cmd != static_cast<quint8>(Command::ReportAll)) {
        appendLogLine(QStringLiteral("INFO: unsupported report cmd 0x%1")
                          .arg(frame.cmd, 2, 16, QLatin1Char('0')));
        return;
    }

    bool ok = false;
    QString error;
    const DeviceState state = ProtocolCodec::decodeDeviceState(frame.data, &ok, &error);
    if (!ok) {
        appendLogLine(QStringLiteral("ERROR: %1").arg(error));
        return;
    }
    if (m_waitingForSettingsAck) {
        m_waitingForSettingsAck = false;
        m_settingsAckTimer->stop();
        appendLogLine(QStringLiteral("SETTINGS: device state report received"), QColor(QStringLiteral("#0B8F3A")));
        const QString diff = summarizeStateDiff(m_settingsStateBeforeAck, state);
        appendLogLine(QStringLiteral("SETTINGS: %1").arg(diff), QColor(QStringLiteral("#0B8F3A")));
    }
    m_stateStore->update(state);
}

QByteArray MainWindow::parseHexInput(const QString &text, bool *ok, QString *error) const
{
    QByteArray result;
    bool localOk = true;
    QString localError;

    QString normalized = text;
    normalized.remove(QRegularExpression(QStringLiteral("\\s+")));
    if (normalized.isEmpty()) {
        localOk = false;
        localError = QStringLiteral("发送内容为空");
    } else if ((normalized.size() % 2) != 0) {
        localOk = false;
        localError = QStringLiteral("HEX 长度必须为偶数");
    } else {
        QRegularExpression re(QStringLiteral("^[0-9A-Fa-f]+$"));
        if (!re.match(normalized).hasMatch()) {
            localOk = false;
            localError = QStringLiteral("HEX 内容包含非法字符");
        } else {
            for (int i = 0; i < normalized.size(); i += 2) {
                const QString part = normalized.mid(i, 2);
                bool partOk = false;
                const int value = part.toInt(&partOk, 16);
                if (!partOk) {
                    localOk = false;
                    localError = QStringLiteral("HEX 转换失败：%1").arg(part);
                    break;
                }
                result.append(static_cast<char>(value));
            }
        }
    }

    if (ok) {
        *ok = localOk;
    }
    if (error) {
        *error = localError;
    }
    return result;
}

void MainWindow::sendManualHex()
{
    if (!m_serialManager->isOpen()) {
        appendLogLine(QStringLiteral("ERROR: 串口未打开，发送被取消"), QColor(QStringLiteral("#B91C1C")));
        QMessageBox::warning(this, QStringLiteral("发送"), QStringLiteral("请先打开串口，再发送数据。"));
        if (m_periodicSendCheck->isChecked()) {
            m_periodicSendCheck->setChecked(false);
        }
        return;
    }

    bool ok = false;
    QString error;
    const QByteArray bytes = parseHexInput(m_sendEdit->text(), &ok, &error);
    if (!ok) {
        if (m_periodicSendCheck->isChecked()) {
            m_periodicSendCheck->setChecked(false);
        }
        QMessageBox::warning(this, QStringLiteral("发送"), error);
        return;
    }
    const bool sent = m_serialManager->sendBytes(bytes);
    if (sent) {
        appendLogLine(QStringLiteral("INFO: 已发送 %1 字节").arg(bytes.size()), QColor(QStringLiteral("#0B8F3A")));
    } else {
        appendLogLine(QStringLiteral("ERROR: 发送失败"), QColor(QStringLiteral("#B91C1C")));
    }
}

void MainWindow::updatePeriodicSendState()
{
    if (!m_periodicSendCheck->isChecked()) {
        m_periodicSendTimer->stop();
        return;
    }

    bool ok = false;
    QString error;
    parseHexInput(m_sendEdit->text(), &ok, &error);
    if (!ok) {
        m_periodicSendCheck->setChecked(false);
        QMessageBox::warning(this, QStringLiteral("定时发送"), error);
        return;
    }

    m_periodicSendTimer->start(m_periodicIntervalSpin->value());
}

void MainWindow::sendCurrentSettingsBatch()
{
    appendLogLine(QStringLiteral("SETTINGS: begin batch apply"));

    const bool heatingOk = m_serialManager->sendBytes(ProtocolCodec::buildSetHeating(m_heatingCheck->isChecked()));
    appendLogLine(QStringLiteral("SETTINGS: heating %1").arg(heatingOk ? QStringLiteral("sent") : QStringLiteral("failed")));

    const bool shutdownEnableOk = m_serialManager->sendBytes(ProtocolCodec::buildSetShutdownEnabled(m_shutdownEnableCheck->isChecked()));
    appendLogLine(QStringLiteral("SETTINGS: shutdown-enable %1").arg(shutdownEnableOk ? QStringLiteral("sent") : QStringLiteral("failed")));

    const bool shutdownDelayOk = m_serialManager->sendBytes(ProtocolCodec::buildSetShutdownDelay(m_shutdownDelaySpin->value()));
    appendLogLine(QStringLiteral("SETTINGS: shutdown-delay %1").arg(shutdownDelayOk ? QStringLiteral("sent") : QStringLiteral("failed")));

    const bool tempOk = m_serialManager->sendBytes(ProtocolCodec::buildSetTemperatures(
        m_setTempSpins[0]->value(),
        m_setTempSpins[1]->value(),
        m_setTempSpins[2]->value(),
        m_setTempSpins[3]->value()));
    appendLogLine(QStringLiteral("SETTINGS: temperatures %1").arg(tempOk ? QStringLiteral("sent") : QStringLiteral("failed")));
    const bool allSent = heatingOk && shutdownEnableOk && shutdownDelayOk && tempOk;
    if (allSent) {
        m_settingsStateBeforeAck = m_stateStore->state();
        m_waitingForSettingsAck = true;
        m_settingsAckTimer->start(1500);
        appendLogLine(QStringLiteral("SETTINGS: waiting for device state report"));
        m_serialManager->sendBytes(ProtocolCodec::buildReadActualTemperatures());
        appendLogLine(QStringLiteral("SETTINGS: requested fresh device state"));
    } else {
        m_waitingForSettingsAck = false;
    }
    appendLogLine(QStringLiteral("SETTINGS: batch apply done"));
}

void MainWindow::setSerialControlsEnabled(bool enabled)
{
    m_portCombo->setEnabled(enabled);
    m_baudCombo->setEnabled(enabled);
    if (m_customBaudEdit) {
        m_customBaudEdit->setEnabled(enabled && m_baudCombo->currentText() == QStringLiteral("其他"));
    }
    m_dataBitsCombo->setEnabled(enabled);
    m_parityCombo->setEnabled(enabled);
    m_stopBitsCombo->setEnabled(enabled);
    m_flowControlCombo->setEnabled(enabled);
    m_refreshPortsButton->setEnabled(enabled);
}

void MainWindow::handleSettingsAckTimeout()
{
    if (!m_waitingForSettingsAck) {
        return;
    }
    m_waitingForSettingsAck = false;
    appendLogLine(QStringLiteral("SETTINGS: timed out waiting for device state report"), QColor(QStringLiteral("#D97706")));
}

void MainWindow::handleOfflineTimeout()
{
    if (m_isOffline) {
        return;
    }
    m_isOffline = true;
    appendLogLine(QStringLiteral("INFO: device state timed out, marked offline"), QColor(QStringLiteral("#D97706")));
    const DeviceState state = m_stateStore->state();
    m_heatingCheck->setChecked(state.heatingEnabled);
    m_shutdownEnableCheck->setChecked(state.shutdownEnabled);
    m_shutdownDelaySpin->setValue(state.shutdownDelaySec);
    const QString offlineText = QStringLiteral("离线");
    for (int row = 0; row < 4; ++row) {
        auto *valueItem = makeReadOnlyItem(row < 2 ? offlineText : (row == 2 ? offlineText : QStringLiteral("--")));
        valueItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valueItem->setForeground(QColor(QStringLiteral("#B91C1C")));
        m_statusTable->setItem(row, 1, valueItem);
    }
}

QString MainWindow::summarizeStateDiff(const DeviceState &before, const DeviceState &after) const
{
    QStringList changes;

    if (before.heatingEnabled != after.heatingEnabled) {
        changes << QStringLiteral("加热状态 %1->%2")
                       .arg(before.heatingEnabled ? QStringLiteral("开") : QStringLiteral("关"),
                            after.heatingEnabled ? QStringLiteral("开") : QStringLiteral("关"));
    }
    if (before.shutdownEnabled != after.shutdownEnabled) {
        changes << QStringLiteral("延时关机 %1->%2")
                       .arg(before.shutdownEnabled ? QStringLiteral("开") : QStringLiteral("关"),
                            after.shutdownEnabled ? QStringLiteral("开") : QStringLiteral("关"));
    }
    if (before.shutdownDelaySec != after.shutdownDelaySec) {
        changes << QStringLiteral("延时时间 %1->%2")
                       .arg(before.shutdownDelaySec)
                       .arg(after.shutdownDelaySec);
    }
    for (int i = 0; i < 4; ++i) {
        if (before.setTemps[i] != after.setTemps[i]) {
            changes << QStringLiteral("通道%1设定 %2->%3")
                           .arg(i + 1)
                           .arg(before.setTemps[i])
                           .arg(after.setTemps[i]);
        }
    }

    if (changes.isEmpty()) {
        return QStringLiteral("状态回包已收到，未检测到配置字段变化");
    }
    return QStringLiteral("状态变化：%1").arg(changes.join(QStringLiteral("，")));
}
