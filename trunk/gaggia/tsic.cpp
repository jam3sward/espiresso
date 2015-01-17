#include "tsic.h"
#include <stdio.h>
#include <unistd.h>
#include "pigpiomgr.h"
using namespace std;

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

// Also requires the PIGPIO library for compilation, which can be
// downloaded here:
//     http://abyz.co.uk/rpi/pigpio/
//
// This version uses PIGPIOD via the pigpiod_if functions
//
// Alternatively, there's a Kernel Module also available: tsic-kernel
// which can be downloaded here:
//     https://code.google.com/p/tsic-kernel

//-----------------------------------------------------------------------------

/// the total number of bits to read from the TSIC sensor
static const unsigned TSIC_BITS = 20;

/// the length of the bit frame used by the TSIC sensor in microseconds
static const unsigned TSIC_FRAME_US = 125;

/// scale factor used to convert sensor values to fixed point integer
static const int SCALE_FACTOR = 1000;

/// minimum temperature for sensor (must match device data)
static const int MIN_TEMP = -50;

/// maximum temperature for sensor (must match device data)
static const int MAX_TEMP = 150;

/// special value used to denote invalid sensor data
static const int INVALID_TEMP = -100000;

//-----------------------------------------------------------------------------

/// calculate parity for an eight bit value
static int parity8( int value )
{
    value = (value ^ (value >> 4)) & 0x0F;
    return (0x6996 >> value) & 1;
}

//-----------------------------------------------------------------------------

// Decode two 9-bit packets from the sensor, and return the temperature.
// Returns either a fixed point integer temperature multiplied by SCALE_FACTOR,
// or INVALID_TEMP in case of error
static int tsicDecode( int packet0, int packet1 )
{
    // strip off the parity bits (LSB)
    int parity0 = packet0 & 1;
    packet0 >>= 1;
    int parity1 = packet1 & 1;
    packet1 >>= 1;

    // check the parity on both bytes
    bool valid =
        ( parity0 == parity8(packet0) ) &&
        ( parity1 == parity8(packet1) );

    // if the parity is wrong, return INVALID_TEMP
    if ( !valid ) {
        //cerr << "tsic: parity error\n";
        return INVALID_TEMP;
    }

    // if any of the top 5 bits of packet 0 are high, that's an error
    if ( (packet0 & 0xF8) != 0 ) {
        //cerr << "tsic: prefix error\n";
        return INVALID_TEMP;
    }

    // this is our raw 11 bit word */
    int raw = (packet0 << 8) | packet1;

    // convert raw integer to temperature in degrees C
    int temp = (MAX_TEMP - MIN_TEMP) * SCALE_FACTOR * raw / 2047 + MIN_TEMP * SCALE_FACTOR;

    // check that the temperature lies in the measurable range
    if ( (temp >= MIN_TEMP * SCALE_FACTOR) && (temp <= MAX_TEMP * SCALE_FACTOR) ) {
        // all looks good
        return temp;
    } else {
        // parity looked good, but the value is out of the valid range
        //cerr << "tsic: range error\n";
        return INVALID_TEMP;
    }
}//tsicDecode

//-----------------------------------------------------------------------------

TSIC::TSIC() :
    m_gpio(0),
    m_open(false),
    m_valid(false),
    m_temperature(0.0),
    m_callback(-1),
    m_count(0),
    m_lastLow(0),
    m_lastHigh(0),
    m_word(0)
{
}

//-----------------------------------------------------------------------------

TSIC::~TSIC()
{
    close();
}

//-----------------------------------------------------------------------------

bool TSIC::open( unsigned gpio )
{
    // close the device if already open
    close();

    // ensure PIGPIO is initialised
    if ( !PIGPIOManager::get().ready() )
        return false;

    // set the GPIO pin to be an input
    if ( set_mode( gpio, PI_INPUT ) != 0 )
        return false;

    // set the GPIO pin pull up
    set_pull_up_down( gpio, PI_PUD_UP );

    // local static function used to forward GPIO alerts to an
    // associated instance of the TSIC class
    struct local {
        static void alertFunction(
            unsigned gpio, unsigned level, uint32_t tick, void *userData
        ) {
            TSIC *self = reinterpret_cast<TSIC*>( userData );
            if ( self != 0 ) self->alertFunction( gpio, level, tick );
        }
    };

    // set a function to receive events when the input changes state
    m_callback = callback_ex( gpio, EITHER_EDGE, local::alertFunction, this );
    if ( m_callback < 0 ) {
        // note: in case of failure leaves GPIO pin set as input
        return false;
    }

    // wait for a packet to arrive
    bool success = false;
    for (int c=0; !success & (c<3); ++c) {
        // sample rate is 10Hz, so we need to wait at least 1/10th second
        usleep( 100000 );
        // attempt to read the value
        double value = 0.0;
        success = getDegrees(value);
    }

    // did we receive some data?
    if ( !success) {
        // no: remove alert function and return false
        callback_cancel( m_callback );
        m_callback = -1;
        return false;
    }

    // store the GPIO pin
    m_gpio = gpio;

    // set flag to indicate we are open
    m_open = true;

    // success
    return true;
}//open

//-----------------------------------------------------------------------------

void TSIC::close()
{
    if ( !m_open ) return;

    // remove the alert function
    callback_cancel( m_callback );
    m_callback = -1;

    // note: leaves the GPIO pin set as input

    // reset members
    m_gpio = 0;
    m_open = false;
    m_valid = false;
    m_temperature = 0.0;
    m_count = 0;
    m_word = 0;
    m_lastLow = 0;
    m_lastHigh = 0;
}//close

//-----------------------------------------------------------------------------

bool TSIC::getDegrees( double & value ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    value = m_temperature;
    return m_valid;
}//getDegrees

//-----------------------------------------------------------------------------

void TSIC::alertFunction(
    int /*gpio*/,   // GPIO number (which should match the member variable)
    int level,      // GPIO level
    uint32_t tick   // time stamp in microseconds
) {
    if ( level == 1 ) {
        // bus went high
        uint32_t elapsed = tick - m_lastLow;

        // assuming 125us frame, 25% duty (low) and 75% duty (high)
        // we treat anything more than 50% duty as a high bit
        if ( elapsed < TSIC_FRAME_US/2 ) {
            // high bit
            m_word = (m_word << 1) | 1;
        } else if ( elapsed < TSIC_FRAME_US ) {
            // low bit
            m_word <<= 1;
        } else {
            // low for more than one frame, which should never happen and
            // must therefore be an invalid bit: start again
            m_count = 0;
            m_word  = 0;
        }

        if ( ++m_count == TSIC_BITS ) {
            // decode the packet
            int result = tsicDecode(
                (m_word >> 10) & 0x1FF, // packet 0
                m_word & 0x1FF          // packet 1
            );

            // update the temperature value
            {
                // lock the mutex
                std::lock_guard<std::mutex> lock( m_mutex );

                // update the temperature value and validity flag
                if ( result != INVALID_TEMP ) {
                    m_temperature =
                        static_cast<double>( result ) /
                        static_cast<double>( SCALE_FACTOR );
                    m_valid = true;
                } else
                    m_valid = false;
            }

            // prepare to receive a new packet
            m_count = 0;
            m_word = 0;
        }

        // bus went high
        m_lastHigh = tick;
    } else {
        // bus went low
        m_lastLow = tick;

        // calculate time spent high
        uint32_t elapsed = tick - m_lastLow;

        // if the bus has been high for more than one frame, reset the
        // counters to start a new packet
        if ( elapsed > TSIC_FRAME_US ) {
            m_count = 0;
            m_word  = 0;
        }
    }
}//alertFunction

//-----------------------------------------------------------------------------
