#include <stdio.h>
#include <sched.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "timing.h"
#include "regulator.h"
#include "flow.h"
#include "pump.h"
#include "ranger.h"
#include "keyboard.h"
#include "inputs.h"
#include "system.h"
#include "display.h"
#include "adc.h"
#include "pressure.h"
#include "settings.h"
#include "pigpiomgr.h"

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

/// Automatic power off time in seconds. Zero disables the time out.
double g_autoPowerOff = 0.0;

class Hardware {
private:
    Timer       m_lastUsed;     ///< When was the last user interaction?
    ADC         m_adc;          ///< ADC used for buttons and pressure sensor
    Pump        m_pump;         ///< Pump controller
    Flow        m_flow;         ///< Flow sensor to measure volume dispensed
    Temperature m_temperature;  ///< Boiler temperature sensor
    Ranger      m_ranger;       ///< Range finder to measure water level
    Display     m_display;      ///< LCD display screen
    System      m_system;       ///< System information
    bool        m_pumpSense;    ///< Is the pump active?
    unsigned    m_pourCount;    ///< Pour count
    Timer       m_pourTime;     ///< Pour timer

    std::shared_ptr<Regulator> m_regulator;

    std::shared_ptr<Inputs> m_inputs;

    std::shared_ptr<Pressure> m_pressure;

public:
    Timer & lastUsed() { return m_lastUsed; }

    ADC & adc() { return m_adc; }

    Pump & pump() { return m_pump; }

    Flow & flow() { return m_flow; }

    Temperature & temperature() { return m_temperature; }

    Ranger & ranger() { return m_ranger; }

    Display & display() { return m_display; }

    System & system() { return m_system; }

    Regulator & regulator() { return *m_regulator; }

    Inputs & inputs() { return *m_inputs; }

    Pressure & pressure() { return *m_pressure; }

    /// Returns true if the pump is active (whether enabled in software, or by
    /// using the manual front panel switch)
    bool pumpSense() const { return m_pumpSense; }

    /// Returns the pour counter value
    unsigned pourCount() const { return m_pourCount; }

    /// Returns the pour time in seconds
    double pourTime() const;

    /// Constructor
    Hardware() {
        // reset and stop the timer
        m_pourTime.reset().stop();

        // used to sense when pump is running
        m_pumpSense = false;

        // used to count how many times the pump has run
        m_pourCount = 0;

        // initialise ADC
        if ( !m_adc.open( I2C_DEVICE_PATH, ADS1015_ADC_I2C_ADDRESS ) )
            cerr << "gaggia: failed to open ADC\n";

        m_regulator = std::make_shared<Regulator>( m_temperature );
        m_inputs = std::make_shared<Inputs>( m_adc, ADC_BUTTON_CHANNEL );
        m_pressure = std::make_shared<Pressure>( m_adc, ADC_PRESSURE_CHANNEL );

        using namespace std::placeholders;

        // register button handler
        inputs().notifyRegister(
            std::bind( &Hardware::buttonHandler, this, _1, _2, _3 )
        );

        // register flow notification handler
        flow().notifyRegister(
            std::bind( &Hardware::flowHandler, this, _1 )
        );
    }

    /// Destructor
    virtual ~Hardware() {
        m_inputs.reset();
        m_regulator.reset();
        m_pressure.reset();
    }

    /// Called when buttons are pressed or released
    void buttonHandler(
        int button,     // button number (1,2)
        bool state,     // pressed (true) or released (false)
        double time     // time period since last state change
    );

    /// Called when notifications are received from the flow sensor
    void flowHandler( Flow::NotifyType type );

    /// Run the control loop
    int runController(
	    bool interactive,
	    const std::string & fileName
    );

    /// Run test mode
    int runTests();
};

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

double Hardware::pourTime() const
{
    return m_pourTime.getElapsed();
}

//-----------------------------------------------------------------------------

/// Called when buttons are pressed or released
void Hardware::buttonHandler(
    int button,     // button number (1,2)
    bool state,     // pressed (true) or released (false)
    double time     // time period since last state change
) {
    // reset the timer whenever the user interacts with the machine
    lastUsed().reset();

    switch ( button ) {
    case BREW_SWITCH:
        // brew switch pushed
        // When the front panel brew switch is pushed to enable the pump, this
        // will be triggered. Since we detect this from the 5V supply in the
        // pump modulation controller, there will be a delay in detecting when
        // the pump is switched off (because the reservoir capacitor in the 5V
        // supply takes time to discharge)
        cout << "gaggia: brew switch "
             << (state ? "enabled" : "disabled")
             << endl;

        // set or clear the flag (stores current state)
        m_pumpSense = state;

        // start or stop the pour timer
        if ( state )
            m_pourTime.start();
        else
            m_pourTime.stop();

        // if the power has been enabled, increment the pour counter
        if ( state ) ++m_pourCount;

        break;

    case BUTTON1:
        if ( state ) {
            // button 1 pushed

            // if the pump is idle
            if ( !pump().getState() ) {
                // ask the flow sensor to notify us after a shot
                // has been dispensed
                flow().notifyAfter( g_shotSize / 1000.0 );
                // turn on the pump
                pump().setState( true );
            } else {
                // pump is already running: turn it off
                pump().setState( false );
            }

            // display pump status for diagnostic purposes
            cout << "gaggia: pump "
                 << (pump().getState() ? "enabled" : "disabled")
                 << endl;
        } else {
            // button 1 released
        }
        break;

    case BUTTON2:
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
                regulator().setPower( !regulator().getPower() );
                cout << "gaggia: boiler "
                     << (regulator().getPower() ? "enabled" : "disabled")
                     << endl;
            }
        }
        break;
    }
}

//-----------------------------------------------------------------------------

/// Called when notifications are received from the flow sensor
void Hardware::flowHandler( Flow::NotifyType type ) {
    // reset the timer when the pump is used (e.g. via front panel switch)
    lastUsed().reset();

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
        pump().setState( false );
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

int Hardware::runController(
	bool interactive,
	const std::string & fileName
) {
    // if button 2 is pushed during initialisation, abort
    if ( inputs().getButton(2) ) {
        cerr << "gaggia: button 2 is down: aborting" << endl;
        return 1;
    }

	// open log file
	ofstream out( fileName.c_str() );
	if ( !out ) {
		cerr << "error: unable to open log file " << fileName << endl;
		return 1;
	}

	// check that flow meter is available
	if ( !flow().ready() ) {
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

    // read auto cut out time (the value in the file is in minutes,
    // and we convert to seconds here)
    g_autoPowerOff = config["autoPowerOff"] * 60.0;

	// read PID controller parameters from configuration
	// these are the proportional, integral, derivate coefficients and
	// the integrator limits
	const double kP = config["kP"];
	const double kI = config["kI"];
	const double kD = config["kD"];
	const double kMin = config["iMin"];
	const double kMax = config["iMax"];

	// set the PID controller parameters
	regulator().setPIDGains( kP, kI, kD );
	regulator().setIntegratorLimits( kMin, kMax );

	// target temperature in degrees centigrade
	double targetTemp = config["targetTemp"];
	regulator().setTargetTemperature( targetTemp );

	// time step in seconds
	double timeStep = config["timeStep"];
	regulator().setTimeStep( timeStep );

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
	regulator().setPower( g_enableBoiler ).start();

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
		double latestTemp = regulator().getTemperature();

		// get the latest boiler power level
		double powerLevel = regulator().getPowerLevel();

		// number of millilitres of water drawn up by the pump
		double ml = 1000.0 * flow().getLitres();

        // pressure in Bar
        double bar = pressure().getBar();

        // pump status
        int pump = pumpSense() ? 1 : 0;

        // pour number (zero if the pump isn't running)
        int pour = (pump > 0) ? pourCount() : 0;

		// dump values to log file
		sprintf(
			buffer,
			"%.3lf,%.2lf,%.2lf,%.1lf,%.2lf,%d,%d",
			elapsed, powerLevel, latestTemp, ml, bar, pump, pour
		);
		out << buffer << endl;

		if (interactive) {
			printf( "%.2lf %.2lf %.1lf %.2lf %d\n", elapsed, latestTemp, ml, bar, pour );
		}

		// update temperature display
		display().updateTemperature( latestTemp );

        // update pressure display
        display().updatePressure( bar );

        // range measurement (convert to mm)
        double range = 1000.0 * ranger().getRange();

		// simplistic conversion to water level
		double minDist = 20.0;
		double maxDist = 100.0;
		range = std::max( minDist, std::min( range, maxDist ) );
		double level = 1.0 - (range - minDist) / (maxDist - minDist);

		// update water level display
		display().updateLevel( level );

        // update boiler power indicator
        display().setPowerOn( regulator().getPower() );

        // update pump status indicator
        display().setPumpOn( pumpSense() );

        // update time display
        display().updateTime( pourTime() );

        // if auto cut out is enabled (greater than one second) and if too
        // much time has elapsed since the last user interaction, and the
        // timer is running, turn off the boiler as a precaution
        if (
            (g_autoPowerOff > 1.0) &&
            lastUsed().isRunning() &&
            (lastUsed().getElapsed() > g_autoPowerOff)
        ) {
            // switch off the boiler
            regulator().setPower( false );

            // stop the timer to prevent repeat triggering
            lastUsed().stop();

            // explanatory message
            cout << "gaggia: switched off power due to inactivity\n";
        }

		// sleep for remainder of time step
		double remain = next - getClock();;
		if ( remain > 0.0 )
			delayms( static_cast<int>(1.0E3 * remain) );
	} while (true);

	if ( interactive )
		nonblock(0);

	// turn the boiler off before we exit
	regulator().setPower( false );

	// if the halt button was pushed, halt the system
	if ( g_halt ) {
		::system( "halt" );
	}

  	return 0;
}

//-----------------------------------------------------------------------------

int Hardware::runTests()
{
    cout << "flow: " <<
        (flow().ready() ? "ready" : "not ready")
        << endl;

    double degrees = 0.0;
    cout << "temp: " <<
        ( temperature().getDegrees(degrees) ? "ready" : "not ready")
        << endl;

	cout << "range: " <<
		( ranger().initialise() ? "ready" : "not ready" )
		<< endl;

	flow().notifyAfter( g_shotSize / 1000.0 );

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

            if ( ( key >= '0' ) && ( key <= '9' ) ) {
                int value = key - '0';
                if ( value > 0 ) {
                    double duty = static_cast<double>(value) / 10.0;
                    pump().setPWMDuty( duty );
                    pump().setState( true );
                    cout << "pump duty = " << duty << endl;
                } else
                    pump().setState( false );
                continue;
            }

			bool stop = false;
			switch ( tolower(key) ) {
			case 'p':
                if ( !pump().getState() ) {
                    flow().notifyAfter( 60.0 / 1000.0 );
                }
                pump().setPWMDuty( 1.0 );
				pump().setState( !pump().getState() );
				cout << "pump: " << (pump().getState() ? "on" : "off") << endl;
				break;

            case 'r':
                {
                    double range = 0.0;
                    const int count = 50;
                    for (int i=0; i<count; ++i)
                        range += ranger().getRange() * 1000.0;
                    cout << "range average: "
                         << range / static_cast<double>(count) << endl;
                }
                break;

			default:
				stop = true;
			}

			if (stop) break;
		}

        // read temperature sensor
        double temp = 0.0;
        temperature().getDegrees( temp );

        // read core temperature
        double coreTemp = system().getCoreTemperature();

        // number of millilitres of water drawn up by the pump
        double ml = 1000.0 * flow().getLitres();

        // range measurement (convert to mm)
        double range = 1000.0 * ranger().getRange();

        // pressure measurement
        double bar = pressure().getBar();

        // print sensor values
        printf(
			"%.2lfC %.2lfC %.1lfml %.0lfmm %.1lf\n",
			temp, coreTemp, ml, range, bar
		);

		// update temperature on display
		display().updateTemperature( temp );

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
    // check that PIGPIO is initialised
    if ( !PIGPIOManager::get().ready() ) {
        cerr << "gaggia: failed to initialise PIGPIO\n";
        return 1;
    }

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
		return Hardware().runController( interactive, filePath + fileName );
	} else if ( command == "test" ) {
		cout << "gaggia: test mode\n";
		return Hardware().runTests();
	} else {
		cerr << "gaggia: unrecognised command (" << command << ")\n";
		return 1;
	}

	return 1;
}

//-----------------------------------------------------------------------------
