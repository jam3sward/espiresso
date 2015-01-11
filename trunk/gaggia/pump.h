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

    /// Set PWM duty cycle (0..1)
    bool setPWMDuty( double duty );

private:
	GPIOPin m_pump;		///< GPIO pin used to control the pump
    GPIOPin m_pumpPWM;  ///< GPIO pin used for pump PWM
	bool m_state;		///< Last state set
};

//-----------------------------------------------------------------------------

#endif//__pump_h

