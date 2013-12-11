#include <stdio.h>
#include <sched.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

#include "timing.h"
#include "pid.h"
#include "temperature.h"
#include "boiler.h"
#include "flow.h"
#include "keyboard.h"
#include "inputs.h"

using namespace std;

//-----------------------------------------------------------------------------

static const string filePath( "/var/log/gaggia/" );
static const string configFile( "/etc/gaggia.conf" );

//-----------------------------------------------------------------------------

std::map<std::string, double> config;

bool disableBoiler = false;		/// disable boiler if true (for testing)

//-----------------------------------------------------------------------------

/// very simplistic configuration file loader
bool loadConfig( std::string fileName )
{
	// open the file
	ifstream f( fileName.c_str() );
	if ( !f ) return false;

	do {
		// read key
		string key;
		f >> ws >> key;
		if ( !f ) break;

		// read value
		double value;
		f >> ws >> value;
		if ( !f ) break;

		// store key/value
		config[key] = value;
		//cout << key << " = " << value << endl;
	} while (true);

	f.close();

	return true;
}

//-----------------------------------------------------------------------------

std::string makeLogFileName()
{
	// get the time
	time_t now;
	time( &now );
	struct tm *info = localtime( &now );

	// format date and time into the filename
	char buffer[256];
	sprintf(
		buffer,
		"%02d%02d%02d-%02d%02d.csv",
		info->tm_year % 100,
		info->tm_mon+1,
		info->tm_mday,
		info->tm_hour,
		info->tm_min
	);

	// return the string
	return buffer;
}

//-----------------------------------------------------------------------------

int runController(
	bool interactive,
	const std::string & fileName
) {
	Temperature temperature;
	Boiler boiler;
	Inputs inputs;
	Flow flow;

	// open log file
	ofstream out( fileName.c_str() );

	// read configuration file
	if ( !loadConfig( configFile ) ) {
		out << "error: failed to load configuration from "
			<< configFile << endl;
		return 1;
	}

	// initialise digital thermometer
	if ( !temperature.initialise() ) {
		out << "error: thermometer not found\n";
		return 1;
	}

	// set 10 bit resolution
	temperature.setResolution( 10 );

	// a couple of dummy temperature reads
	double value = 0.0;
	temperature.read( &value );
	temperature.read( &value );

	// read PID controller parameters from configuration
	// these are the proportional, integral, derivate coefficients and
	// the integrator limits
	const double kP = config["kP"];
	const double kI = config["kI"];
	const double kD = config["kD"];
	const double kMin = config["iMin"];
	const double kMax = config["iMax"];

	PIDControl pid;
	pid.setPIDGains( kP, kI, kD );
	pid.setIntegratorLimits( kMin, kMax );

	// target temperature in degrees centigrade
	double targetTemp = config["targetTemp"];

	// time step in seconds
	double timeStep = config["timeStep"];

	// output parameters to log
	char buffer[512];
	sprintf(
		buffer,
		"P=%.4lf,I=%.4lf,D=%.4lf,iMin=%.3lf,iMax=%.3lf,"
		"targetTemp=%.3lf,timeStep=%.3lf",
		kP, kI, kD, kMin, kMax,
		targetTemp, timeStep
	);
	out << buffer << endl;

	if ( interactive )
		nonblock(1);

	bool halt = false;

	short old = 0;
	double start = getClock();
	double next  = start;
	do {
		// next time step
		next += timeStep;

		if ( interactive && kbhit() ) break;

		if ( inputs.getHaltButton() ) {
			halt = true;
			break;
		}

		double elapsed = getClock() - start;

		double temp = 0.0;
		while ( !temperature.read( &temp ) ) {}

		double drive = pid.update( targetTemp - temp, temp );

		if ( drive > 1.0 )
			drive = 1.0;
		else if ( drive < 0.0 )
			drive = 0.0;

		// turn on the boiler (pulse width modulation)
		// note: will leave boiler on if drive is 1
		if ( !disableBoiler )
			boiler.setPower( drive );

		sprintf(
			buffer,
			"%.3lf,%.2lf,%.2lf",
			elapsed, drive, temp
		);
		out << buffer << endl;

		if (interactive) {
			printf( "%.2lf %.2lf %u\n", elapsed, temp, flow.getCount() );
		}

		// sleep for remainder of time step
		//double used = getClock() - elapsed - start;
		double remain = next - getClock();;
		if ( remain > 0.0 )
			delayms( (int)(1.0E3 * remain) );
	} while (true);

	if ( interactive )
		nonblock(0);

	// turn the boiler off before we exit!
	boiler.powerOff();

	// if the halt button was pushed, halt the system
	if ( halt ) {
		system( "halt" );
	}

  	return 0;
}

//-----------------------------------------------------------------------------

int main( int argc, char **argv )
{
	if ( argc < 2 ) {
		cerr << "gaggia: expected a command\n";
		return 1;
	}

	const string command( argv[1] );

	bool interactive = false;

	for (int i=2; i<argc; ++i) {
		string option( argv[i] );
		if ( option == "-i" )
			interactive = true;
		else if ( option == "-d" )
			disableBoiler = true;
		else
			cerr << "gaggia: unexpected option\n";
	}

	if ( command == "stop" ) {
		Boiler boiler;
		boiler.powerOff();
		cout << "gaggia: turned off boiler SSR\n";
		return 0;
	} else if ( command == "start" ) {
		string fileName( makeLogFileName() );
		cout << "gaggia: starting controller (log=" << fileName << ")\n";
		return runController( interactive, filePath + fileName );
	} else {
		cerr << "gaggia: unrecognised command (" << command << ")\n";
		return 1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
