#include "temperature.h"
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
using namespace std;

//-----------------------------------------------------------------------------

Temperature::Temperature()
{
	// attempt to find the sensor on the W1 bus
	m_sensorPath = findSensorPath();
}

//-----------------------------------------------------------------------------

bool Temperature::getDegrees( double *value ) const
{
	// attempt to open the sensor
	ifstream sensor( m_sensorPath.c_str() );

	string line;
	getline( sensor, line );	// discard first line
	getline( sensor, line );	// contains temperature at end, e.g. t=12345

	// if we don't find "t=" then something is wrong
	size_t pos = line.find("t=");
	if ( pos == string::npos ) return false;

	// extract just the number, example: 12345
	line = line.substr( pos+2 );

	// convert to degrees
	double degrees = static_cast<double>( atoi(line.c_str()) ) / 1000.0;

	if ( value != 0 ) *value = degrees;

	return true;
}

//-----------------------------------------------------------------------------

std::string Temperature::findSensorPath()
{
	// open file containing list of W1 slaves
	ifstream slaves(
		"/sys/bus/w1/devices/w1_bus_master1/w1_master_slaves"
	);

	// attempt to read first line of file: the name of the first slave
	string firstSlave;
	if ( !getline( slaves, firstSlave ) ) return string();

	// construct full path
	string fullPath(
		string("/sys/bus/w1/devices/") + firstSlave + "/w1_slave"
	);

	// return to caller
	return fullPath;
}

//-----------------------------------------------------------------------------
