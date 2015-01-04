#ifndef __tsic_h
#define __tsic_h

//-----------------------------------------------------------------------------
//
// Copyright (C) 2014-2015 James Ward
//
// This software is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
//-----------------------------------------------------------------------------

#include <inttypes.h>
#include <mutex>

//-----------------------------------------------------------------------------

/// TSIC Temperature Sensor Class, designed for use with the TSIC 306 sensor,
/// but may be adaptable for use with other devices of the same class.
class TSIC
{
public:
    /// Default constructor
    TSIC();

    /// Destructor
    ~TSIC();

    /// Open the sensor, given the GPIO pin. Returns true for success,
    /// or false in case of failure
    bool open( unsigned gpio );

    /// Close the sensor
    void close();

    /// Returns the current temperature in degrees C. Returns true for
    /// success, or false in case of failure
    bool getDegrees( double & value ) const;

    /// Alert function called when the GPIO pin changes state
    void alertFunction( int gpio, int level, uint32_t tick );

private:
    unsigned m_gpio;        ///< the GPIO pin used for the sensor
    bool     m_open;        ///< true if the sensor is open
    bool     m_valid;       ///< temperature data is valid
    double   m_temperature; ///< current temperature

    int      m_callback;    ///< callback identifier
    uint32_t m_count;       ///< number of bits received in current packet
    uint32_t m_lastLow;     ///< time when GPIO pin last went low (us)
    int      m_word;        ///< used to consolidate incoming packet bits

    /// Mutex to control access to the sensor data
    mutable std::mutex m_mutex;
};

//-----------------------------------------------------------------------------

#endif//__tsic_h

