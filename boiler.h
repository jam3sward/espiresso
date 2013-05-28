#ifndef __boiler_h
#define __boiler_h

//-----------------------------------------------------------------------------

/// Represents the boiler, which can be turned on and off
class Boiler {
public:
	/// Default constructor
	Boiler();

	/// Turn on/off
	void setPower( bool on );

	/// Returns true if on, false otherwise
	bool getPower() const;

	/// Pulse the power for width*period (seconds)
	void pulsePower( double width, double period );

private:
	bool m_power;	/// Power on/off
};

//-----------------------------------------------------------------------------

#endif//__boiler_h

