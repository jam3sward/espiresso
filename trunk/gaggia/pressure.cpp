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
    static const double voltageRange = (maxVoltage - minVoltage);

    // supply voltage
    static const double supplyVoltage = 3.3;

    // measure the ADC voltage
    double voltage = 0.0;
    const int samples = 3;
    for (int i=0; i<samples; ++i)
        voltage += m_adc.getVoltage( m_channel );
    voltage /= static_cast<double>(samples);

    // approximate conversion to Bar
    double bar = maxPressure * (voltage - minVoltage) / voltageRange;

    // clamp to zero
    return ( bar > 0.0 ) ? bar : 0.0;
}

//-----------------------------------------------------------------------------
