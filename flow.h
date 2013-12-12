#ifndef __flow_h
#define __flow_h

//-----------------------------------------------------------------------------

#include <thread>
#include <mutex>
#include "gpiopin.h"

//-----------------------------------------------------------------------------

class Flow {
public:
	/// Default constructor
	Flow();

	/// Destructor
	virtual ~Flow();

	/// Reset the counter
	Flow & resetCount();

	/// Returns the current raw counter value, which is an integer
	/// measure of the volume of fluid that has passed through the sensor
	unsigned getCount() const;

	/// Returns the fluid volume that has passed through the flow
	/// sensor in litres
	double getLitres() const;

	/// Returns the number of counts per litre
	unsigned getCountsPerLitre() const;

	/// Set the number of counts per litre
	Flow & setCountsPerLitre( unsigned counts );

	/// Is the flow meter ready for use?
	bool ready() const;

private:
	/// Worker thread
	void worker();

private:
	GPIOPin  m_flowPin;	///< Pin used to read the flow sensor
	unsigned m_count;	///< Current counter value
	bool	 m_run;		///< Should thread continue to run?

	/// The number of counts per litre
	unsigned m_countsPerLitre;

	/// Thread used to monitor the flow sensor
	std::thread m_thread;

	/// Mutex to control access to the counter
	mutable std::mutex m_mutex;
};

//-----------------------------------------------------------------------------

#endif//__flow_h
