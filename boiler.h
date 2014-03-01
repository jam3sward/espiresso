#ifndef __boiler_h
#define __boiler_h

//-----------------------------------------------------------------------------

#include "pwm.h"

//-----------------------------------------------------------------------------

/// Represents the boiler, which can be turned on and off
class Boiler {
public:
	/// Default constructor
	Boiler();

	/// Destructor
	~Boiler();

	/// Set current power level (0..1)
	void setPower( double value );

	/// Returns power level (0..1)
	double getPower() const;

	/// Returns true if boiler is on
	bool isOn() const;

	/// Turn off the boiler (same as setting power to 0)
	void powerOff();

private:
	PWM	 m_pwm;		///< Hardware PWM
};

//-----------------------------------------------------------------------------

#endif//__boiler_h

