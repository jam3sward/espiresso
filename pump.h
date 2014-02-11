#ifndef __pump_h
#define __pump_h

//-----------------------------------------------------------------------------

#include "gpiopin.h"

//-----------------------------------------------------------------------------

/// Represents the pump, which can be turned on and off
class Pump {
public:
	/// Default constructor
	Pump();

	/// Destructor
	virtual ~Pump();

	/// Set current state (on/off)
	bool setState( bool state );

	/// Return current state (on/off)
	bool getState() const;

private:
	GPIOPin m_pump;		///< GPIO pin used to control the pump
	bool m_state;		///< Last state set
};

//-----------------------------------------------------------------------------

#endif//__pump_h

