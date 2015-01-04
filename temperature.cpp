#include "temperature.h"
#include "settings.h"

//-----------------------------------------------------------------------------

Temperature::Temperature()
{
    m_tsic.open( TSIC_PIN );
}

//-----------------------------------------------------------------------------

Temperature::~Temperature()
{
    m_tsic.close();
}

//-----------------------------------------------------------------------------

bool Temperature::getDegrees( double & value ) const
{
    return m_tsic.getDegrees( value );
}

//-----------------------------------------------------------------------------
