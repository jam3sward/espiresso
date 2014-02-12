#ifndef __temperature_h
#define __temperature_h

//-----------------------------------------------------------------------------

#include <string>

//-----------------------------------------------------------------------------

/// Temperature sensor class
class Temperature
{
public:
	/// Default constructor
	Temperature();

	/// Read the temperature in degrees C
	bool getDegrees( double *value ) const;

private:
	/// Find the sensor path
	static std::string findSensorPath();

private:
	std::string m_sensorPath;	///< Path to sensor on file system
};

//-----------------------------------------------------------------------------

#endif//__temperature_h

