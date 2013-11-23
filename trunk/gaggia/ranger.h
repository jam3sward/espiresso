#ifndef __ranger_h
#define __ranger_h

//-----------------------------------------------------------------------------

#include "gpiopin.h"

//-----------------------------------------------------------------------------

class Ranger {
public:
	/// Default constructor
	Ranger();

	/// Destructor
	virtual ~Ranger();

	/// Measures and returns range in metres. Will block while the ranging
	/// operation is in progress. May return zero in case of failure.
	double getRange();

private:
	GPIOPin m_trigger;		///< Output pin used to trigger the ranger
	GPIOPin m_echo;			///< Input pin used to receive the echo signal
	double m_timeLastRun;	///< Time when getRange() was last called
};

//-----------------------------------------------------------------------------

#endif//__ranger_h

