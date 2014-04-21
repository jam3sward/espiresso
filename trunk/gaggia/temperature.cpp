#include "temperature.h"
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

//-----------------------------------------------------------------------------

Temperature::Temperature()
{
	// attempt to find the sensor path on the filesystem
	m_sensorPath = findSensorPath();
}

//-----------------------------------------------------------------------------

bool Temperature::getDegrees( double *value ) const
{
    // attempt to open the sensor
    ifstream sensor( m_sensorPath.c_str() );

    // read the first line
    string line;
    getline( sensor, line );

    // convert to degrees
    double degrees = static_cast<double>( atoi(line.c_str()) ) / 1000.0;

	if ( value != 0 ) *value = degrees;

	return true;
}

//-----------------------------------------------------------------------------

std::string Temperature::findSensorPath()
{
    /* Now we use the TSIC 306, the sensor path is fixed (currently) */
	return "/sys/kernel/tsic/temp";
}

//-----------------------------------------------------------------------------
