#include "boiler.h"
#include "gpio.h"
#include "settings.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Boiler::Boiler()
{
	BCM::open();

	// configure PWM for 2Hz frequency with 4000 range
	m_pwm.setDivisor( 2400 );
	m_pwm.setRange( 4000 );

	// set up with 0% duty cycle and enable
	m_pwm.setValue( 0.0 );
	m_pwm.enable();
}

//-----------------------------------------------------------------------------

Boiler::~Boiler()
{
	// shutdown PWM
	m_pwm.disable();
}

//-----------------------------------------------------------------------------

void Boiler::setPower( double value )
{
	// clamp width to 0% minimum
	if ( value < 0.0 ) value = 0.0;

	// clamp width to 100% maximum
	if ( value > 1.0 ) value = 1.0;

	// set pulse width
	m_pwm.setValue( value );
}

//-----------------------------------------------------------------------------

double Boiler::getPower() const
{
	return m_pwm.getValue();
}

//-----------------------------------------------------------------------------

bool Boiler::isOn() const
{
	return (m_pwm.getIntegerValue() != 0);
}

//-----------------------------------------------------------------------------

void Boiler::powerOff()
{
	m_pwm.setIntegerValue( 0 );
}

//-----------------------------------------------------------------------------
