#include "pump.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Pump::Pump() :
	m_pump( PUMP_PIN ),
    m_pumpPWM( PUMP_PWM_PIN ),
	m_state( false )
{
	// Make the pin an output and drive it low
	m_pump.setOutput( true ).setState( false );

    // Make the pin an output and set the PWM frequency and duty
    m_pumpPWM.setOutput( true ).setPWMFrequency( 2000 ).setPWMDuty( 0.0 );
}

//-----------------------------------------------------------------------------

Pump::~Pump()
{
	// Turn off the pump
	m_pump.setState( false );
}

//-----------------------------------------------------------------------------

bool Pump::setState( bool state )
{
	m_pump.setState( state );
	m_state = state;
	return getState();
}

//-----------------------------------------------------------------------------

bool Pump::getState() const
{
	return m_state;
}

//-----------------------------------------------------------------------------

bool Pump::setPWMDuty( double duty )
{
    m_pumpPWM.setPWMDuty( duty );
    return true;
}

//-----------------------------------------------------------------------------
