#include "hcsr04.h"
#include "pigpiomgr.h"

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

HCSR04::HCSR04() :
    m_gpioTrig(0),
    m_gpioEcho(0),
    m_open(false),
    m_callback(-1),
    m_timeout(60),
    m_count(0)
{
    m_timeStamp[0] = 0;
    m_timeStamp[1] = 0;
}

//-----------------------------------------------------------------------------

HCSR04::~HCSR04()
{
    // close at destruction
    close();
}

//-----------------------------------------------------------------------------

bool HCSR04::open( unsigned gpioTrigOut, unsigned gpioEchoIn )
{
    // close if already open
    close();

    // ensure PIGPIO is initialised
    if ( !PIGPIOManager::get().ready() )
        return false;

    // set up the input pin
    if ( set_mode( gpioEchoIn, PI_INPUT ) != 0 )
        return false;

    // set up the output pin
    if ( set_mode( gpioTrigOut, PI_OUTPUT ) != 0 ) {
        // note: echo is left as an input
        return false;
    }

    // local static function used to forward GPIO alerts to an
    // associated instance of the HCSR class
    struct local {
        static void alertFunction(
            unsigned gpio, unsigned level, uint32_t tick, void *userData
        ) {
            HCSR04 *self = reinterpret_cast<HCSR04*>( userData );
            if ( self != 0 ) self->alertFunction( gpio, level, tick );
        }
    };

    // set a function to receive events when the input changes state
    m_callback = callback_ex(
        gpioEchoIn, EITHER_EDGE, local::alertFunction, this
    );
    if ( m_callback < 0 ) {
        // note: in case of failure, set output back to an input
        set_mode( gpioTrigOut, PI_INPUT );
        return false;
    }

    // store GPIO pin numbers
    m_gpioTrig = gpioTrigOut;
    m_gpioEcho = gpioEchoIn;

    // set a flag to indicate we are open
    m_open = true;

    // success
    return true;
}//open

//-----------------------------------------------------------------------------

void HCSR04::close()
{
    if ( !m_open ) return;

    // remove the alert function
    callback_cancel( m_callback );
    m_callback = -1;

    // change the output to an input
    set_mode( m_gpioTrig, PI_INPUT );

    m_gpioTrig = 0;
    m_gpioEcho = 0;
    m_open = false;
}//close

//-----------------------------------------------------------------------------

bool HCSR04::getRange( long & us, long & mm )
{
    // check that the device is open
    if ( !m_open ) return false;

    /// the speed of sound in mm/s
    const long speedSound_mms = 340270;

    {
        // lock the mutex
        std::lock_guard<std::mutex> lock( m_mutex );
        // initialise interrupt counter
        m_count = 0;
    }

    //-- send 10us pulse

    // ensure GPIO is low initially
    gpio_write( m_gpioTrig, 0 );

    // take GPIO high
    if ( gpio_write( m_gpioTrig, 1 ) != 0 )
        return false;

    // delay for 10us
    usleep(10);

    // take GPIO low
    if ( gpio_write( m_gpioTrig, 0 ) != 0 )
        return false;

    // polling interval (ms)
    static const unsigned pollInterval = 1;

    //-- wait for the result to arrive
    bool complete = false;
    unsigned slept = 0;
    do {
        // polling interval
        usleep( pollInterval * 1000 );
        slept += pollInterval;

        std::lock_guard<std::mutex> lock( m_mutex );
        us = m_timeStamp[1] - m_timeStamp[0];
        complete = ( m_count == 2 );
    } while ( !complete && (slept < m_timeout) );

    //-- return the results
    if ( complete ) {
        mm = us * speedSound_mms / 2000000;
        return true;
    } else {
        us = 0;
        mm = 0;
        return false;
    }
}//getRange

//-----------------------------------------------------------------------------

void HCSR04::setTimeout( unsigned ms )
{
    // lock the mutex
    std::lock_guard<std::mutex> lock( m_mutex );
    m_timeout = ms;
}//setTimeout

//-----------------------------------------------------------------------------

void HCSR04::alertFunction(
    unsigned /*gpio*/,  // GPIO number (which should match the member variable)
    unsigned level,     // GPIO level
    uint32_t tick       // time stamp in microseconds
) {
    // lock the mutex
    std::lock_guard<std::mutex> lock( m_mutex );

    // for the first two interrupts received, store the time-stamp
    if ( m_count < 2 ) {
        m_timeStamp[m_count] = tick;
        ++m_count;
    }
}//alertFunction

//-----------------------------------------------------------------------------
