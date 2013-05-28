#include "boiler.h"
#include "gpio.h"
#include "settings.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Boiler::Boiler() :
	m_power( false )
{
	gpio_initialise();

	// make SSR pin an output and set low initially
    INP_GPIO(SSRPIN);
    OUT_GPIO(SSRPIN);
    GPIO_CLR = 1<<SSRPIN;
}

//-----------------------------------------------------------------------------

void Boiler::setPower( bool on )
{
	// turn SSR on/off as required
	if (on)
		GPIO_SET = 1<<SSRPIN;
	else
    	GPIO_CLR = 1<<SSRPIN;

	// store power state
	m_power = on;
}

//-----------------------------------------------------------------------------

bool Boiler::getPower() const
{
	return m_power;
}

//-----------------------------------------------------------------------------

void Boiler::pulsePower( double width, double period )
{
	// for a very small width, do nothing
	if ( width < 0.01 ) return;

	// clamp width to 100% maximum
	if ( width > 1.0 ) width = 1.0;

	// turn the power on
	setPower( true );

	// keep it on for specified period
	delayus( width * 1.0E6 * period );

	// if less than 100% then leave it on
	if ( width < 0.99 )
		setPower( false );
}

//-----------------------------------------------------------------------------
