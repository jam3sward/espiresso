#ifndef __regulator_h
#define __regulator_h

//-----------------------------------------------------------------------------

#include <thread>
#include <mutex>
#include <atomic>
#include "pid.h"
#include "temperature.h"
#include "boiler.h"

//-----------------------------------------------------------------------------

/// This temperature regulator class manages the temperature sensor, boiler
/// and PID temperature control loop on an independent thread
class Regulator :private PIDControl
{
typedef PIDControl baseClass;

public:
	/// Default constructor. The boiler power is off by default, and must be
	/// switched on with setPower()
	Regulator();

	/// Destructor
	virtual ~Regulator();

	/// Start controller
	bool start();

	/// Stop control loop
	void stop();

	/// Set the Proportional, Integral and Derivative gains
	Regulator & setPIDGains( double pGain, double iGain, double dGain );

	/// Set the lower and upper limits for the integrator
	Regulator & setIntegratorLimits( double iMin, double iMax );

	/// Set the time step
	Regulator & setTimeStep( double timeStep );

	/// Set the target temperature
	Regulator & setTargetTemperature( double target );

	/// Returns the target temperature in degrees C
	double getTargetTemperature() const;

	/// Read the temperature in degrees C
	double getTemperature() const;

	/// Read the boiler power level (0..1)
	double getPowerLevel() const;

	/// Switch boiler power on/off
	Regulator & setPower( bool power );

private:
	/// Worker thread which regulates boiler temperature
	void worker();

private:
	std::atomic<bool> m_run;	///< Should thread continue to run?

	/// Thread used to monitor the flow sensor
	std::thread m_thread;

	/// Mutex to control access to shared data
	mutable std::mutex m_mutex;

	bool		m_power;		///< Power on/off
	double		m_timeStep;		///< Time step in seconds
	double		m_targetTemp;	///< Target temperature in degrees
	double		m_latestTemp;	///< Latest temperature in degrees
	double		m_latestPower;	///< Latest power level (0..1)
	Temperature	m_temperature;	///< Temperature sensor
	Boiler		m_boiler;		///< Boiler control
};

//-----------------------------------------------------------------------------

#endif//__regulator_h
