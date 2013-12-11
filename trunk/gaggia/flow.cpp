#include <chrono>
#include <iostream>
#include "flow.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Flow::Flow() :
	m_flowPin( FLOWPIN ),
	m_count( 0 ),
	m_run( true ),
	m_thread( &Flow::worker, this )
{
}

//-----------------------------------------------------------------------------

Flow::~Flow()
{
	// gracefully terminate the thread
	m_run = false;

	// wait for the thread to terminate
	m_thread.join();
}

//-----------------------------------------------------------------------------

void Flow::worker()
{
	// initialise GPIO pin as an input and enable interrupts on both
	// rising and falling edges
	m_flowPin.setOutput( false ).setEdgeTrigger( GPIOPin::Both );

	// poll for interrupts and count pulses
	while (m_run) {
		if ( m_flowPin.poll( 250 ) ) {
			// received an interrupt

			// increment our counter
			std::lock_guard<std::mutex> lock( m_mutex );
			++m_count;
		} else {
			// timed out (opportunity to exit)
		}
	}
}//worker

//-----------------------------------------------------------------------------

unsigned Flow::getCount() const
{
	// return the counter value
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_count;
}

//-----------------------------------------------------------------------------
