#include "regulator.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Regulator::Regulator() :
	m_run( true ),
	m_power( false ),
	m_timeStep( 1.0 ),
	m_targetTemp( 95.0 ),
	m_latestTemp( 20.0 ),
	m_latestPower( 0.0 ),
	m_thread( &Regulator::worker, this )
{
}

//-----------------------------------------------------------------------------

Regulator::~Regulator()
{
	// stop thread execution
	stop();
}

//-----------------------------------------------------------------------------

bool Regulator::start()
{
	// stop any existing thread
	stop();

	// start thread
	m_run = true;
	m_thread = std::thread( &Regulator::worker, this );

	// todo: possible indication of failed initialisation
	return true;
}

//-----------------------------------------------------------------------------

void Regulator::stop()
{
	// gracefully terminate the thread
	m_run = false;

	// wait for the thread to terminate
	m_thread.join();
}

//-----------------------------------------------------------------------------

Regulator & Regulator::setPIDGains( double pGain, double iGain, double dGain )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	baseClass::setPIDGains( pGain, iGain, dGain );
	return *this;
}

//-----------------------------------------------------------------------------

Regulator & Regulator::setIntegratorLimits( double iMin, double iMax )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	baseClass::setIntegratorLimits( iMin, iMax );
	return *this;
}

//-----------------------------------------------------------------------------

Regulator & Regulator::setTimeStep( double timeStep )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_timeStep = timeStep;
	return *this;
}

//-----------------------------------------------------------------------------

Regulator & Regulator::setTargetTemperature( double target )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_targetTemp = target;
	return *this;
}

//-----------------------------------------------------------------------------

double Regulator::getTargetTemperature() const
{
	// the worker never modifies this
	return m_targetTemp;
}

//-----------------------------------------------------------------------------

double Regulator::getTemperature() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_latestTemp;
}

//-----------------------------------------------------------------------------

double Regulator::getPowerLevel() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_latestPower;
}

//-----------------------------------------------------------------------------

Regulator & Regulator::setPower( bool power )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_power = power;
	return * this;
}

//-----------------------------------------------------------------------------

void Regulator::worker()
{
	// initialise digital thermometer
	m_temperature.getDegrees(0);	// todo: error handling

	// start time and next time step
	double start = getClock();
	double next  = start;

	// run the thread until requested to stop
	while (m_run) {
		// get elapsed time since start
		double elapsed = getClock() - start;

		// take temperature measurement
		double latestTemp = 0.0;
		m_temperature.getDegrees( &latestTemp );

		// boiler drive (duty cycle)
		double drive = 0.0;

		// if the temperature is near zero, we assume there's an error
		// reading the sensor and drive (duty cycle) will be zero
		if ( latestTemp > 0.5 ) {
			// lock shared data before use
			std::lock_guard<std::mutex> lock( m_mutex );

			// calculate next time step
			next += m_timeStep;

			// calculate PID update
			drive = update( m_targetTemp - latestTemp, latestTemp );

			// disable boiler if power is off
			if ( !m_power ) drive = 0.0;
		}

		// clamp the output power to sensible range
		if ( drive > 1.0 )
			drive = 1.0;
		else if ( drive < 0.0 )
			drive = 0.0;

		// set the boiler power (uses pulse width modulation)
		m_boiler.setPower( drive );

		// store the latest temperature reading
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			m_latestTemp  = latestTemp;
			m_latestPower = drive;
		}

		// sleep for the remainder of the time step
		double remain = next - getClock();
		if ( remain > 0.0 )
			delayms( static_cast<int>(1.0E3 * remain) );
	};

	// ensure the boiler is turned off before exit
	m_boiler.powerOff();
}

//-----------------------------------------------------------------------------
