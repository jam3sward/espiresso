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
	struct sched_param sched = {};
	sched.sched_priority = sched_get_priority_max( SCHED_RR );
	sched_setscheduler( 0, SCHED_RR, &sched );

	Temperature temperature;
	Boiler boiler;

	// open log file
	ofstream out( fileName.c_str() );

	if ( !temperature.initialise() ) {
		out << "error: thermometer not found\n";
		return 1;
	}

	// set 10 bit resolution
	temperature.setResolution( 10 );

	const double kP = 0.05;
	const double kI = 0.04;
	const double kD = 0.3;
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

	short old = 0;
	double start = getClock();
	do {
		if ( interactive && kbhit() ) break;

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
		boiler.pulsePower( drive, 1.0 );

		sprintf(
			buffer,
			"%.3lf,%.1lf,%.4lf",
			elapsed, drive, position
		);
		out << buffer << endl;

		// sleep for remainder of time step
		double used = getClock() - elapsed - start;
		double remain = timeStep - used;
		if ( remain > 0.0 )
			delayus( (int)(1.0E6 * remain) );
	} while (true);

	if ( interactive )
		nonblock(0);

	// turn the boiler off before we exit!
	boiler.setPower( false );

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

	if ( command == "stop" ) {
		Boiler boiler;
		boiler.setPower( false );
		cout << "gaggia: turned off boiler SSR\n";
		return 0;
	} else if ( command == "start" ) {
		string fileName( makeLogFileName() );
		cout << "gaggia: starting controller (log=" << fileName << ")\n";
		return runController( false, filePath + fileName );
	} else {
		cerr << "gaggia: unrecognised command (" << command << ")\n";
		return 1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
