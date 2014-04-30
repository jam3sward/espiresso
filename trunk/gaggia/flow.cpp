#include <chrono>
#include <future>
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
	m_notifyFunc( nullptr ),
	m_notifyCount( 0 ),
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

	// timeout period for detecting pulses
	const unsigned timeout = 1000;
	const unsigned idleTimeout = 1000;

	// is liquid flowing?
	bool flowing  = false;

	// how long has the flow sensor been idle?
	unsigned idleTime = 0;

	// poll for interrupts and count pulses
	while (m_run) {
		bool wasFlowing = flowing;

		if ( m_flowPin.poll( timeout ) ) {
			// received an interrupt

			// fluid is flowing
			flowing = true;

			// notification when flow starts
			if ( flowing != wasFlowing ) {
				if ( m_notifyFunc ) try {
					std::async(
						std::launch::async,
						m_notifyFunc, Flow::Start
					);
				} catch ( const std::system_error & e ) {
				}
			}

			bool notify = false;

			{
				// lock the mutex
				std::lock_guard<std::mutex> lock( m_mutex );

				// increment our counter
				++m_count;

				// should we send a notification?
				notify = (m_notifyCount > 0) && (m_count >= m_notifyCount);

				// prevent multiple notifications
				if ( notify ) m_notifyCount = 0;
			}

			// send the notification
			if ( notify ) {
				try {
					// call it asynchronously
					std::async(
						std::launch::async,
						m_notifyFunc, Flow::Target
					);
				} catch ( const std::system_error & e ) {
					// couldn't start thread: call from this one
					if ( e.code() == std::errc::resource_unavailable_try_again )
						m_notifyFunc( Flow::Target );
				}
			}
		} else {
			// timed out (opportunity to exit)

			// count how long the flow sensor has been idle
			idleTime += timeout;

			if ( idleTime >= idleTimeout ) {
				flowing = false;
				idleTime = 0;
			}

			if ( flowing != wasFlowing ) {
				if ( m_notifyFunc ) try {
					std::async(
						std::launch::async,
						m_notifyFunc, Flow::Stop
					);
				} catch ( const std::system_error & e ) {
				}
			}
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

Flow & Flow::notifyRegister( NotifyFunc func )
{
	// set up the notification
	std::lock_guard<std::mutex> lock( m_mutex );
	m_notifyFunc = func;

	return *this;
}

//-----------------------------------------------------------------------------

Flow & Flow::notifyAfter( double litres )
{
	// convert litres to an integer number of counts
	unsigned amount = static_cast<unsigned>(
		litres * static_cast<double>(m_countsPerLitre) + 0.5
	);

	// set up the notification
	std::lock_guard<std::mutex> lock( m_mutex );
	m_notifyCount = m_count + amount;

	return *this;
}

//-----------------------------------------------------------------------------

Flow & Flow::notifyCancel()
{
	// clear the notification
	std::lock_guard<std::mutex> lock( m_mutex );
	m_notifyFunc  = nullptr;
	m_notifyCount = 0;

	return *this;
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
