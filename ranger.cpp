#include "ranger.h"
#include "settings.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Ranger::Ranger() :
	m_trigger( RANGER_TRIGGER_OUT ),
	m_echo( RANGER_ECHO_IN )
{
	// make trigger an output and set it low
	m_trigger.setOutput( true ).setState( false );

	// make echo an input and enable edge triggered interrupts on both
	// rising and falling edges
	m_echo.setOutput( false ).setEdgeTrigger( GPIOPin::Both );

	// record current time as last run time, in case the initialisation
	// above has triggered the ranger
	m_timeLastRun = getClock();
}

//-----------------------------------------------------------------------------

Ranger::~Ranger()
{
}

//-----------------------------------------------------------------------------

double Ranger::getRange()
{
    // minimum time (in seconds) between successive calls
    // this is to prevent the ranger from being triggered too frequently
    const double minimumInterval = 0.060;

    // timeout in milliseconds when waiting for echo
    const unsigned timeout = 60;

    double elapsed = 0.0;   // elapsed time in seconds
    bool response = false;  // have we received a reply?

    // calculate interval since we were last run
    double interval = getClock() - m_timeLastRun;
    if ( interval < minimumInterval ) {
        // delay if needed to avoid sending a trigger when echoes from the
        // previous trigger are still incoming
        delayms( 1000.0 * (minimumInterval - interval) );
    }

    // remember time of last run
    m_timeLastRun = getClock();

    // transmit 10us high pulse to trigger the ranger
	m_trigger.usPulse( true, 10 );

    // wait for rising edge
    if ( m_echo.poll( timeout ) ) {
        // record time of rising edge
        double rise = getClock();

		// wait for falling edge
        if ( m_echo.poll( timeout ) ) {
            // record time of falling edge
            double fall = getClock();

            // calculate elapsed time
            elapsed = fall - rise;

            // set flag to indicate that a response was received
            response = true;
        }
    }

    // calculate distance in m
    double speedOfSound = 340.27;   // m/s
    double distance = elapsed * speedOfSound / 2.0;

    // return distance (or zero if echo not received)
    if ( response )
        return distance;
    else
        return 0.0;
}//getRange

//-----------------------------------------------------------------------------
