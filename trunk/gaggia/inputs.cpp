#include <array>
#include <future>
#include <assert.h>
#include <math.h>
#include "inputs.h"
#include "settings.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Inputs::Inputs( ADC & adc, unsigned channel ) :
    m_adc( adc ),
    m_channel( channel ),
    m_buttonState( 0 ),
    m_notifyFunc( nullptr ),
    m_run( true ),
    m_thread( &Inputs::worker, this )
{
}

//-----------------------------------------------------------------------------

Inputs::~Inputs()
{
    // gracefully terminate the thread
    m_run = false;

    // wait for the thread to terminate
    m_thread.join();
}

//-----------------------------------------------------------------------------

bool Inputs::getButton( int button ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return ( (m_buttonState & (1 << button)) != 0 );
}

//-----------------------------------------------------------------------------

Inputs & Inputs::notifyRegister( NotifyFunc func )
{
    // set up the notification
    std::lock_guard<std::mutex> lock( m_mutex );
    m_notifyFunc = func;

    return *this;
}

//-----------------------------------------------------------------------------

Inputs & Inputs::notifyCancel()
{
    // clear the notification
    std::lock_guard<std::mutex> lock( m_mutex );
    m_notifyFunc = nullptr;

    return *this;
}

//-----------------------------------------------------------------------------

void Inputs::worker()
{
    // minimum time period for polling (milliseconds)
    const unsigned period = 50;

    struct State {
        bool state;
        bool oldState;
        double timeStamp;
    };

    // button states
    std::array<State,3> button;
    for (size_t i=0; i<button.size(); ++i) {
        button[i].oldState = false;
        button[i].timeStamp = getClock();
    }

    // poll the buttons
    while (m_run) {
        // sample the ADC voltage
        double voltage = m_adc.getVoltage( m_channel );

        // find the nearest matching button state for the ADC voltage
        // this is effectively a bit-mask, with one bit per button
        // (zero indicates that no buttons are pushed)
        unsigned buttonState = getNearestButtonState( voltage );

        // store the latest button state bit-mask
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_buttonState = buttonState;
        }

        // check the individual button states by testing bits within
        // the state bit-mask returned by getNearestButtonState
        for (size_t i=0; i<button.size(); ++i)
            button[i].state = ((buttonState & (1<<i)) != 0);

        // for each button
        for (size_t i=0; i<button.size(); ++i) {
            // if the button state has changed
            if ( button[i].state != button[i].oldState ) {
                // calculate elapsed time since last event
                double now = getClock();
                double elapsed = now - button[i].timeStamp;
                button[i].timeStamp = now;

                std::lock_guard<std::mutex> lock( m_mutex );

                // fire the notification function
                if ( m_notifyFunc ) try {
                    std::async(
                        std::launch::async,
                        m_notifyFunc,
                        i+1, button[i].state, elapsed
                    );
                } catch ( const std::system_error & e ) {
                }
            }

            // store the old button state and time stamp
            button[i].oldState = button[i].state;
        }

        // sleep for a while
        delayms( period );
    }
}//worker

//-----------------------------------------------------------------------------

/// Returns the index of the closest matching button state for a given
/// ADC voltage. Individual bits of this integer index can then be
/// tested to determine the state of each button.
unsigned Inputs::getNearestButtonState( double voltage ) const
{
    // lookup table to convert voltages into key states
    static const std::array<double,8> lookup{
        3.30754,    // measured
        2.65300,    // predicted. todo: update this value
        2.19561,    // measured
        1.87800,    // predicted. todo: update this value
        1.66669,    // measured
        1.47100,    // predicted. todo: update this value
        1.33261,    // measured
        1.19700     // predicted. todo: update this value
    };

    // minimum error so far (use large initial value)
    double minError = lookup[0];

    // minimum difference between two successive values in the table
    // (this assumes decreasing voltage, and uses the last two values)
    assert( lookup.size() >= 2 );
    static const double minGap = fabs(
        lookup[ lookup.size()-2 ] - lookup[ lookup.size()-1 ]
    );

    // the accepted tolerance when matching values
    static const double tolerance = minGap / 2.0;

    // closest index so far
    unsigned closest = 0;

    // calculate distance from each entry in lookup table
    for (unsigned i=0; i<lookup.size(); ++i) {
        double error = fabs( lookup[i] - voltage );
        if ( error < minError ) {
            closest  = i;
            minError = error;
        }
    }

    // is it within tolerance?
    if ( minError <= tolerance ) {
        // yes: return the closest matching value
        return closest;
    } else {
        // no: return zero
        return 0;
    }
}//getNearestButtonState

//-----------------------------------------------------------------------------
