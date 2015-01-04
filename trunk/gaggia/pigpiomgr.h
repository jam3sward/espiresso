#ifndef __pigpiomgr_h
#define __pigpiomgr_h

//-----------------------------------------------------------------------------

#include <iostream>
extern "C" {
#include <pigpiod_if.h>
}

//-----------------------------------------------------------------------------

/// Singleton class to manage initialisation of PIGPIO
class PIGPIOManager {
public:
    /// Returns the single instance
    static PIGPIOManager & get();

    /// Returns true if PIPGIO is available
    bool ready() const;

    /// Returns the PIGPIO version number
    int version() const;

private:
    /// Constructor
    PIGPIOManager();

    /// Destructor
    ~PIGPIOManager();

    int m_version;  ///< PIGPIO version number (or PI_INIT_FAILED)
};

//-----------------------------------------------------------------------------

#endif//__pigpiomgr_h

