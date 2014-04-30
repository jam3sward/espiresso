#include <stdio.h>
#include <sched.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

#include "timing.h"
#include "regulator.h"
#include "flow.h"
#include "pump.h"
#include "ranger.h"
#include "keyboard.h"
#include "inputs.h"
#include "system.h"
#include "display.h"

using namespace std;

//-----------------------------------------------------------------------------

static const string filePath( "/var/log/gaggia/" );
static const string configFile( "/etc/gaggia.conf" );

//-----------------------------------------------------------------------------

std::map<std::string, double> config;

bool g_enableBoiler = true;	///< Enable boiler if true
bool g_quit = false;		///< Should we quit?
bool g_halt = false;        ///< Should we halt? (shutdown the system)

double g_shotSize = 60.0;   ///< Shot size in ml

Pump      g_pump;           ///< Pump controller
Flow      g_flow;           ///< Flow sensor
Inputs    g_inputs;         ///< Inputs (buttons)
Regulator g_regulator;      ///< Temperature regulator

//-----------------------------------------------------------------------------

void signalHandler( int signal )
{
	switch ( signal ) {
	case SIGINT:
		printf( "\ngaggia: received SIGINT\n" );
		g_quit = true;
		break;

	case SIGTERM:
		printf( "\ngaggia: received SIGTERM\n" );
		g_quit = true;
		break;
	}
}

//-----------------------------------------------------------------------------

/// Called when buttons are pressed or released
void buttonHandler(
    int button,     // button number (1,2)
    bool state,     // pressed (true) or released (false)
    double time     // time period since last state change
) {
    switch ( button ) {
    case 1:
        if ( state ) {
            // button 1 pushed

            // if the pump is idle
            if ( !g_pump.getState() ) {
                // ask the flow sensor to notify us after a shot
                // has been dispensed
                g_flow.notifyAfter( g_shotSize / 1000.0 );
                // turn on the pump
                g_pump.setState( true );
            } else {
                // pump is already running: turn it off
                g_pump.setState( false );
            }
        } else {
            // button 1 released
        }
        break;

    case 2:
        if ( state ) {
            // button 2 pushed
        } else {
            // button 2 released
            if ( time >= 1.0 ) {
                // button was held for 1 second, shut down the system
                cout << "gaggia: shutting down\n";
                g_halt = true;
                g_quit = true;
            } else {
                // button was pushed briefly, toggle boiler power
                g_regulator.setPower( !g_regulator.getPower() );
                cout << "gaggia: boiler "
                     << (g_regulator.getPower() ? "enabled" : "disabled")
                     << endl;
            }
        }
        break;
    }
}

//-----------------------------------------------------------------------------

/// Called when notifications are received from the flow sensor
static void flowHandler( Flow::NotifyType type ) {
	switch ( type ) {
	case Flow::Start :
		cout << "flow: started\n";
		break;

	case Flow::Stop  :
		cout << "flow: stopped\n";
		break;

	case Flow::Target:
		cout << "flow: target reached\n";
        // stop the pump
        g_pump.setState( false );
		break;
	}
}

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
    // if button 2 is pushed during initialisation, abort
    if ( g_inputs.getButton(2) ) {
        cerr << "gaggia: button 2 is down: aborting" << endl;
        return 1;
    }

	Display display;
	Ranger ranger;

	// open log file
	ofstream out( fileName.c_str() );
	if ( !out ) {
		cerr << "error: unable to open log file " << fileName << endl;
		return 1;
	}

	// check that flow meter is available
	if ( !g_flow.ready() ) {
		out << "error: flow meter not ready\n";
		return 1;
	}

	// read configuration file
	if ( !loadConfig( configFile ) ) {
		out << "error: failed to load configuration from "
			<< configFile << endl;
		return 1;
	}

    // shot size in millilitres
    g_shotSize = config["shotSize"];

	// read PID controller parameters from configuration
	// these are the proportional, integral, derivate coefficients and
	// the integrator limits
	const double kP = config["kP"];
	const double kI = config["kI"];
	const double kD = config["kD"];
	const double kMin = config["iMin"];
	const double kMax = config["iMax"];

	// set the PID controller parameters
	g_regulator.setPIDGains( kP, kI, kD );
	g_regulator.setIntegratorLimits( kMin, kMax );

	// target temperature in degrees centigrade
	double targetTemp = config["targetTemp"];
	g_regulator.setTargetTemperature( targetTemp );

	// time step in seconds
	double timeStep = config["timeStep"];
	g_regulator.setTimeStep( timeStep );

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

	// time step for user interface / display
	const double timeStepGUI = 0.25;

	// start time and next time step
	double start = getClock();
	double next  = start;

	// turn on the power and start the regulator (boiler will begin to heat)
	g_regulator.setPower( g_enableBoiler ).start();

	do {
		// next time step
		next += timeStepGUI;

		// in interactive mode, exit if any key is pressed
		if ( interactive && kbhit() ) break;

		// if asked to stop (e.g. via SIGINT)
		if ( g_quit ) break;

		// calculate elapsed time
		double elapsed = getClock() - start;

		// get the latest temperature reading
		double latestTemp = g_regulator.getTemperature();

		// get the latest boiler power level
		double powerLevel = g_regulator.getPowerLevel();

		// number of millilitres of water drawn up by the pump
		double ml = 1000.0 * g_flow.getLitres();

		// dump values to log file
		sprintf(
			buffer,
			"%.3lf,%.2lf,%.2lf,%.1lf",
			elapsed, powerLevel, latestTemp, ml
		);
		out << buffer << endl;

		if (interactive) {
			printf( "%.2lf %.2lf %.1lf\n", elapsed, latestTemp, ml );
		}

		// update temperature display
		display.updateTemperature( latestTemp );

        // range measurement (convert to mm)
        double range = 1000.0 * ranger.getRange();

		// simplistic conversion to water level
		double minDist = 20.0;
		double maxDist = 100.0;
		range = std::max( minDist, std::min( range, maxDist ) );
		double level = 1.0 - (range - minDist) / (maxDist - minDist);

		// update water level display
		display.updateLevel( level );

		// sleep for remainder of time step
		double remain = next - getClock();;
		if ( remain > 0.0 )
			delayms( static_cast<int>(1.0E3 * remain) );
	} while (true);

	if ( interactive )
		nonblock(0);

	// turn the boiler off before we exit
	g_regulator.setPower( false );

	// if the halt button was pushed, halt the system
	if ( g_halt ) {
		system( "halt" );
	}

  	return 0;
}

//-----------------------------------------------------------------------------

int runTests()
{
    Temperature temperature;
    Ranger ranger;
    System system;
	Display display;

    cout << "flow: " <<
        (g_flow.ready() ? "ready" : "not ready")
        << endl;

    cout << "temp: " <<
        (temperature.getDegrees(0) ? "ready" : "not ready")
        << endl;

	cout << "range: " <<
		(ranger.initialise() ? "ready" : "not ready")
		<< endl;

	g_flow.notifyAfter( g_shotSize / 1000.0 );

    nonblock(1);

	// time step
	const double timeStep = 0.5;

	// start time and next time step
	double start = getClock();
	double next  = start;

    do {
        // calculate next time step
        next += timeStep;

		// if quit has been requested (e.g. via SIGINT)
		if ( g_quit ) break;

        if ( kbhit() ) {
			// get key
			char key = getchar();

			bool stop = false;
			switch ( tolower(key) ) {
			case 'p':
                if ( !g_pump.getState() ) {
                    g_flow.notifyAfter( 60.0 / 1000.0 );
                }
				g_pump.setState( !g_pump.getState() );
				cout << "pump: " << (g_pump.getState() ? "on" : "off") << endl;
				break;

            case 'r':
                {
                    double range = 0.0;
                    const int count = 50;
                    for (int i=0; i<count; ++i)
                        range += ranger.getRange() * 1000.0;
                    cout << "range average: " << range / static_cast<double>(count) << endl;
                }
                break;

			default:
				stop = true;
			}

			if (stop) break;
		}

		// received request to stop pump
		g_pump.setState( false );

        // read temperature sensor
        double temp = 0.0;
        temperature.getDegrees( &temp );

        // read core temperature
        double coreTemp = system.getCoreTemperature();

        // number of millilitres of water drawn up by the pump
        double ml = 1000.0 * g_flow.getLitres();

        // range measurement (convert to mm)
        double range = 1000.0 * ranger.getRange();

        // print sensor values
        printf(
			"%.2lfC %.2lfC %.1lfml %.0lfmm\n",
			temp, coreTemp, ml, range
		);

		// update temperature on display
		display.updateTemperature( temp );

		// sleep for remainder of time step
		double remain = next - getClock();;
		if ( remain > 0.0 )
			delayms( static_cast<int>(1.0E3 * remain) );
    } while (true);

    nonblock(0);

	return 0;
}

//-----------------------------------------------------------------------------

int main( int argc, char **argv )
{
	// hook SIGINT so we can exit gracefully
	if ( signal(SIGINT, signalHandler) == SIG_ERR ) {
		cerr << "gaggia: failed to hook SIGINT\n";
		return 1;
	}

	// hook SIGTERM so we can exit gracefully
	if ( signal(SIGTERM, signalHandler) == SIG_ERR ) {
		cerr << "gaggia: failed to hook SIGTERM\n";
		return 1;
	}

    // register notification handlers
    g_inputs.notifyRegister( &buttonHandler );
    g_flow.notifyRegister( &flowHandler );

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
		else if ( option == "-d" ) {
            // disable the boiler
			g_enableBoiler = false;
		} else
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
