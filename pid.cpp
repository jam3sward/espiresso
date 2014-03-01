#include "pid.h"

//-----------------------------------------------------------------------------

PIDControl::PIDControl() :
	m_dState( 0.0 ),
	m_iState( 0.0 ),
	m_iMin( -1.0 ),
	m_iMax(  1.0 ),
	m_iGain( 0.0 ),
	m_pGain( 1.0 ),
	m_dGain( 0.0 )
{
}

//-----------------------------------------------------------------------------

PIDControl::~PIDControl()
{
}

//-----------------------------------------------------------------------------

PIDControl & PIDControl::setPIDGains( double pGain, double iGain, double dGain )
{
	m_pGain = pGain;
	m_iGain = iGain;
	m_dGain = dGain;
	return *this;
}

//-----------------------------------------------------------------------------

PIDControl & PIDControl::setIntegratorLimits( double iMin, double iMax )
{
	m_iMin = iMin;
	m_iMax = iMax;
	return *this;
}

//-----------------------------------------------------------------------------

double PIDControl::update( double error, double position )
{
	// calculate proportional term
	double pTerm = m_pGain * error;

	// calculate integral state with appropriate limiting
	m_iState += error;
	if ( m_iState > m_iMax )
		m_iState = m_iMax;
	else if ( m_iState < m_iMin )
		m_iState = m_iMin;

	// calculate integral term
	double iTerm = m_iGain * m_iState;

	// calculate derivative term
	double dTerm = m_dGain * (m_dState - position);
	m_dState = position;

	return pTerm + dTerm + iTerm;
}

//-----------------------------------------------------------------------------
