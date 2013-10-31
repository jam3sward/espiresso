#include "inputs.h"
#include "gpio.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Inputs::Inputs()
{
	BCM::open();
	INP_GPIO(HALTBUTTONPIN);
}

//-----------------------------------------------------------------------------

bool Inputs::getHaltButton() const
{
	return ( GPIO_IN0 & (1<<HALTBUTTONPIN) ) == 0;
}

//-----------------------------------------------------------------------------
