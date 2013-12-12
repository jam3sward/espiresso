#include <chrono>
#include <iostream>
#include "flow.h"
#include "settings.h"

// Using the Digmesa FHKSC 932-9521-B flow sensor with 1.2mm diameter bore
// With flow sensor in situ, pumping fresh water, measured:
//  count=1033, weight=248g = 4165 counts/l
//  count=1272, weight=316g = 4025 counts/l
//  average of these: 4095 counts/l
// Manufacturer data suggests 1925 pulses/l and we trigger on both rising and
// falling edge which equates to 3850 pulses/l, about 6% difference

//-----------------------------------------------------------------------------

Flow::Flow() :
	m_flowPin( FLOWPIN ),
	m_count( 0 ),
	m_run( true ),
	m_countsPerLitre( 4095 ),
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

Flow & Flow::resetCount()
{
	// reset the counter
	std::lock_guard<std::mutex> lock( m_mutex );
	m_count = 0;
	return *this;
}

//-----------------------------------------------------------------------------

unsigned Flow::getCount() const
{
	// return the counter value
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_count;
}

//-----------------------------------------------------------------------------

double Flow::getLitres() const
{
	return
		static_cast<double>( getCount() ) /
		static_cast<double>( m_countsPerLitre );
}

//-----------------------------------------------------------------------------

unsigned Flow::getCountsPerLitre() const
{
	return m_countsPerLitre;
}

//-----------------------------------------------------------------------------

Flow & Flow::setCountsPerLitre( unsigned counts )
{
	m_countsPerLitre = counts;
	return *this;
}

//-----------------------------------------------------------------------------

bool Flow::ready() const {
	return m_flowPin.ready();
}

//-----------------------------------------------------------------------------
