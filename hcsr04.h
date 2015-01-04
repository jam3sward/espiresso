#ifndef __hcsr04_h
#define __hcsr04_h

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

#include <mutex>

//-----------------------------------------------------------------------------

/// HSCR-04 Ultrasonic Range Finder Class
class HCSR04
{
public:
    /// Default constructor
    HCSR04();

    /// Destructor
    virtual ~HCSR04();

    /// Open the sensor, given the GPIO pins for trigger (output) and
    /// echo (input). Returns true for success, or false in case of failure
    bool open( unsigned gpioTrigOut, unsigned gpioEchoIn );

    /// Close the sensor
    void close();

    /// Get the range, in microseconds and mm
    bool getRange( long & us, long & mm );

    /// Set the range finding timeout
    void setTimeout( unsigned ms );

private:
    unsigned m_gpioTrig;    ///< the trigger output GPIO pin
    unsigned m_gpioEcho;    ///< the echo input GPIO pin
    bool     m_open;        ///< the device is open (true) or closed (false)
    int      m_callback;    ///< the callback identifier

    unsigned m_timeout;     ///< timeout for range finding in ms
    unsigned m_count;       ///< number of interrupts received
    uint32_t m_timeStamp[2];///< time-stamp of each interrupt

    std::mutex m_mutex;     ///< mutex for shared variables

    /// Alert function called when the GPIO pin changes state
    void alertFunction( unsigned gpio, unsigned level, uint32_t tick );
};

//-----------------------------------------------------------------------------

#endif//__hcsr04_h
