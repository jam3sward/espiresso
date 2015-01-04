#ifndef __temperature_h
#define __temperature_h

//-----------------------------------------------------------------------------

#include <string>
#include "tsic.h"

//-----------------------------------------------------------------------------

/// Temperature sensor class
class Temperature
{
public:
	/// Default constructor
	Temperature();

    /// Destructor
    virtual ~Temperature();

	/// Read the temperature in degrees C
	bool getDegrees( double & value ) const;

private:
    TSIC m_tsic;    ///< Associated temperature sensor
};

//-----------------------------------------------------------------------------

#endif//__temperature_h

