#include "inputs.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Inputs::Inputs() :
	m_button1( BUTTON1_PIN ),
	m_button2( BUTTON2_PIN )
{
	m_button1.setOutput( false );
	m_button2.setOutput( false );
}

//-----------------------------------------------------------------------------

Inputs::~Inputs()
{
}

//-----------------------------------------------------------------------------

bool Inputs::getButton( int button ) const
{
	switch ( button ) {
	case 1: return !m_button1.getState(); break;
	case 2: return !m_button2.getState(); break;
	}
	return false;
}

//-----------------------------------------------------------------------------
