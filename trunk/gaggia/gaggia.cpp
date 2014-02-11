#include <stdio.h>
#include <sched.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

#include "timing.h"
#include "pid.h"
#include "temperature.h"
#include "boiler.h"
#include "flow.h"
#include "pump.h"
#include "ranger.h"
#include "keyboard.h"
#include "inputs.h"
#include "system.h"

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

	// check that flow meter is available
	if ( !flow.ready() ) {
		out << "error: flow meter not ready\n";
		return 1;
	}

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

//		if ( inputs.getHaltButton() ) {
//			halt = true;
//			break;
//		}

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

		// number of millilitres of water drawn up by the pump
		double ml = 1000.0 * flow.getLitres();

		// dump values to log file
		sprintf(
			buffer,
			"%.3lf,%.2lf,%.2lf,%.1lf",
			elapsed, drive, temp, ml
		);
		out << buffer << endl;

		if (interactive) {
			printf( "%.2lf %.2lf %.1lf\n", elapsed, temp, ml );
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

int runTests()
{
    Temperature temperature;
    Flow flow;
    Ranger ranger;
	Pump pump;
    System system;

    cout << "flow: " <<
        (flow.ready() ? "ready" : "not ready")
        << endl;

    cout << "temp: " <<
        (temperature.initialise() ? "ready" : "not ready")
        << endl;

	cout << "range: " <<
		(ranger.initialise() ? "ready" : "not ready")
		<< endl;

    // set 10 bit resolution
    temperature.setResolution( 10 );

    nonblock(1);

    do {
        if ( kbhit() ) {
			// get key
			char key = getchar();

			bool stop = false;
			switch ( tolower(key) ) {
			case 'p':
				pump.setState( !pump.getState() );
				cout << "pump: " << (pump.getState() ? "on" : "off") << endl;
				break;

			default:
				stop = true;
			}

			if (stop) break;
		}

        // read temperature sensor
        double temp = 0.0;
        while ( !temperature.read( &temp ) ) {}

        // read core temperature
        double coreTemp = system.getCoreTemperature();

        // number of millilitres of water drawn up by the pump
        double ml = 1000.0 * flow.getLitres();

        // range measurement (convert to mm)
        double range = 1000.0 * ranger.getRange();

        // print sensor values
        printf(
			"%.2lfC %.2lfC %.1lfml %.0lfmm\n",
			temp, coreTemp, ml, range
		);
    } while (true);

    nonblock(0);

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
	} else if ( command == "test" ) {
		cout << "gaggia: test mode\n";
		return runTests();
	} else {
		cerr << "gaggia: unrecognised command (" << command << ")\n";
		return 1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
