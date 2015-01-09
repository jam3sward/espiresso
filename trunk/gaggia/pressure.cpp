#include "pressure.h"

//-----------------------------------------------------------------------------

Pressure::Pressure( ADC & adc, unsigned channel ) :
    m_adc( adc ),
    m_channel( channel )
{
}

//-----------------------------------------------------------------------------

Pressure::~Pressure()
{
}

//-----------------------------------------------------------------------------

double Pressure::getBar() const
{
    // maximum reading of pressure sensor in Bar (0..300psi)
    static const double maxPressure = 20.6842719;

    // minimum, maximum voltage and voltage range of pressure sensor
    static const double minVoltage = 0.5;
    static const double maxVoltage = 4.5;

    // supply voltage
    static const double supplyVoltage = 3.3;

    // measure the ADC voltage
    double voltage = m_adc.getVoltage( m_channel );

    // approximate conversion to Bar
    return maxPressure * (voltage - minVoltage) / (maxVoltage - minVoltage);
}

//-----------------------------------------------------------------------------
