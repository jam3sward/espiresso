#include <stdio.h>
#include <sched.h>
#include <string>
#include <iostream>
#include <fstream>

#include "timing.h"
#include "pid.h"
#include "temperature.h"
#include "boiler.h"
#include "keyboard.h"
#include "inputs.h"

using namespace std;

//-----------------------------------------------------------------------------

static const string filePath( "/var/log/gaggia/" );

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
	// run at high priority
//	struct sched_param sched = {};
//	sched.sched_priority = sched_get_priority_max( SCHED_RR );
//	sched_setscheduler( 0, SCHED_RR, &sched );

	Temperature temperature;
	Boiler boiler;
	Inputs inputs;

	// open log file
	ofstream out( fileName.c_str() );

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

	const double kP = 0.07;
	const double kI = 0.05;
	const double kD = 0.9;
	const double kMin = 0.0;
	const double kMax = 1.0;

	PIDControl pid;
	pid.setPIDGains( kP, kI, kD );
	pid.setIntegratorLimits( kMin, kMax );

	char buffer[256];
	sprintf(
		buffer,
		"P=%.4f,I=%.4f,D=%.4f,iMin=%.3f,iMax=%.3f",
		kP, kI, kD, kMin, kMax
	);
	out << buffer << endl;

	double targetPosition = 95.0;

	double timeStep = 1.0;;

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

		double position = 0.0;
		while ( !temperature.read( &position ) ) {}

		double drive = pid.update( targetPosition - position, position );

		if ( drive > 1.0 )
			drive = 1.0;
		else if ( drive < 0.0 )
			drive = 0.0;

		// turn on the boiler (pulse width modulation)
		// note: will leave boiler on if drive is 1
		boiler.setPower( drive );

		sprintf(
			buffer,
			"%.3lf,%.2lf,%.2lf",
			elapsed, drive, position
		);
		out << buffer << endl;

		if (interactive) printf( "%.2lf\n", position );

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
