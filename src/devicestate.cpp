#include "devicestate.h"

DeviceStateStore::DeviceStateStore(QObject *parent)
    : QObject(parent)
{
}

const DeviceState &DeviceStateStore::state() const
{
    return m_state;
}

void DeviceStateStore::reset()
{
    m_state = DeviceState{};
    emit stateChanged(m_state);
}

void DeviceStateStore::update(const DeviceState &state)
{
    m_state = state;
    emit stateChanged(m_state);
}
