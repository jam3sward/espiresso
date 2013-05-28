#ifndef __temperature_h
#define __temperature_h

#include <stdlib.h>

//-----------------------------------------------------------------------------

/// Temperature sensor class
class Temperature
{
public:
	/// Default constructor
	Temperature();

	/// Initialise the sensor (start of a communication transaction)
	bool initialise();

	/// Write a byte to the device
	void writeByte( unsigned char data );

	/// Read a byte from the device
	unsigned char readByte();

	/// Read a block of date from the device
	bool readData( char *data, size_t size );

	/// Perform temperature conversion
	void convert();

	/// Read the raw temperature value
	bool readRaw( short *value );

	/// Read the temperature in degrees C
	bool read( double *value );

	/// Set the temperature sensor resolution
	bool setResolution( int bits );

private:
	short  m_mask;		///< bit-mask for current sensor resolution
	double m_degrees;	///< last read temperature in degrees C
};

//-----------------------------------------------------------------------------

#endif//__temperature_h

