#ifndef __inputs_h
#define __inputs_h

//-----------------------------------------------------------------------------

#include "gpiopin.h"

//-----------------------------------------------------------------------------

/// Represents the inputs
class Inputs {
public:
	/// Default constructor
	Inputs();

	/// Destructor
	virtual ~Inputs();

	/// Get button state (given button index, starting from 1)
	bool getButton( int button ) const;

private:
	GPIOPin	m_button1;	///< GPIO pin for button 1
	GPIOPin m_button2;	///< GPIO pin for button 2
};

//-----------------------------------------------------------------------------

#endif//__inputs_h
