#include "inputs.h"
#include "gpio.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Inputs::Inputs()
{
	BCM::open();
	//INP_GPIO(HALTBUTTONPIN);
}

//-----------------------------------------------------------------------------
