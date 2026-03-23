#pragma once

#include "protocoltypes.h"

#include <QObject>

class DeviceStateStore : public QObject
{
    Q_OBJECT

public:
    explicit DeviceStateStore(QObject *parent = nullptr);

    const DeviceState &state() const;
    void reset();
    void update(const DeviceState &state);

signals:
    void stateChanged(const DeviceState &state);

private:
    DeviceState m_state;
};
