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

	/// Returns the current raw counter value
	unsigned getCount() const;

private:
	/// Worker thread
	void worker();

private:
	GPIOPin  m_flowPin;	///< Pin used to read the flow sensor
	unsigned m_count;	///< Current counter value
	bool	 m_run;		///< Should thread continue to run?

	/// Thread used to monitor the flow sensor
	std::thread m_thread;

	/// Mutex to control access to the counter
	mutable std::mutex m_mutex;
};

//-----------------------------------------------------------------------------

#endif//__flow_h
