#ifndef __gpiopin_h
#define __gpiopin_h

//-----------------------------------------------------------------------------

#include <string>

//-----------------------------------------------------------------------------

class GPIOPin {
public:
	GPIOPin( int pin );

	virtual ~GPIOPin();

	/// Supported edge trigger modes
	enum Edge {
		None,
		Falling,
		Rising,
		Both
	};

	/// Set the pin to be an output (true) or input (false)
	GPIOPin & setOutput( bool output );

	/// Set the pin state
	GPIOPin & setState( bool state );

	/// Get the pin state
	bool getState() const;

	/// Pulse high or low for specified number of microseconds
	GPIOPin & usPulse( bool state, unsigned us );

	/// Pulse high or low for given number of milliseconds
	GPIOPin & msPulse( bool state, unsigned ms );

	/// Set edge trigger
	GPIOPin & setEdgeTrigger( Edge edge );

	/// Poll the pin with timeout
	bool poll( unsigned timeout );

	/// Is the pin ready (is it configured successfully?) 
	bool ready() const;

protected:
	/// Export the pin
	bool exportPin();

	/// Unexport the pin
	bool unexportPin();

	/// Open the pin for IO
	bool open();

	/// Close the pin
	void close();

private:
	int	m_pin;			///< GPIO pin number
	int m_file;			///< Associated file descriptor
	std::string m_path;
};

//-----------------------------------------------------------------------------

#endif//__gpiopin_h


