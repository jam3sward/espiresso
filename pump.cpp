#include "pump.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Pump::Pump() :
	m_pump( PUMP_PIN ),
	m_state( false )
{
	// Make the pin an output and drive it low
	m_pump.setOutput( true ).setState( false );
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
