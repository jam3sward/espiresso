#include "pigpiomgr.h"

//-----------------------------------------------------------------------------

PIGPIOManager & PIGPIOManager::get() {
    static PIGPIOManager instance;
    return instance;
}

//-----------------------------------------------------------------------------

bool PIGPIOManager::ready() const {
    return (m_version != PI_INIT_FAILED);
}

//-----------------------------------------------------------------------------

int PIGPIOManager::version() const {
    return ready() ? m_version : 0;
}

//-----------------------------------------------------------------------------

PIGPIOManager::PIGPIOManager()
{
    // guessing this is the same return value as gpioInitialise (undocumented)
    m_version = pigpio_start( NULL, NULL );
}

//-----------------------------------------------------------------------------

PIGPIOManager::~PIGPIOManager()
{
    pigpio_stop();
}

//-----------------------------------------------------------------------------
