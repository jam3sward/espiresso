#ifndef __gpiopin_h
#define __gpiopin_h

//-----------------------------------------------------------------------------

#include <functional>
#include "pigpiomgr.h"

//-----------------------------------------------------------------------------

class GPIOPin {
public:
	GPIOPin( unsigned pin );

	virtual ~GPIOPin();

	/// Supported edge trigger modes
	enum Edge {
		Falling = FALLING_EDGE,
		Rising  = RISING_EDGE,
		Both    = EITHER_EDGE
	};

	/// Set the pin to be an output (true) or input (false)
	GPIOPin & setOutput( bool output );

	/// Set the pin state
	GPIOPin & setState( bool state );

    /// Set PWM duty cycle on a pin (0 to 1 range)
    GPIOPin & setPWMDuty( double duty );

    /// Set PWM frequency on a pin (nearest match is used)
    GPIOPin & setPWMFrequency( unsigned frequency );

	/// Get the pin state
	bool getState() const;

	/// Pulse high or low for specified number of microseconds
	GPIOPin & usPulse( bool state, unsigned us );

	/// Pulse high or low for given number of milliseconds
	GPIOPin & msPulse( bool state, unsigned ms );

	/// Set edge trigger
	GPIOPin & setEdgeTrigger( Edge edge );

    /// Notification function type
    typedef std::function<void(
        unsigned pin,
        bool     level,
        unsigned tick
    )> EdgeFunc;

    /// Set edge notification
    GPIOPin & edgeFuncRegister( EdgeFunc edgeFunc );

    /// Cancel edge notification
    GPIOPin & edgeFuncCancel();

	/// Poll the pin with timeout
	bool poll( unsigned timeout );

	/// Is the pin ready (is it configured successfully?) 
	bool ready() const;

protected:
	/// Open the pin for IO
	bool open();

	/// Close the pin
	void close();

private:
    /// Called when an edge event is received
    void callback( unsigned pin, bool level, unsigned tick );

private:
	unsigned m_pin;         ///< GPIO pin number
    bool     m_open;        ///< Successfully opened
    Edge     m_edge;        ///< Edge trigger mode

    EdgeFunc m_edgeFunc;    ///< Edge function
    int      m_callback;    ///< Callback function identifier
};

//-----------------------------------------------------------------------------

#endif//__gpiopin_h


