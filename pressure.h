#ifndef __pressure_h
#define __pressure_h

//-----------------------------------------------------------------------------

#include "adc.h"

//-----------------------------------------------------------------------------

/// Represents the pressure sensor
class Pressure {
public:
    /// Constructor
    Pressure( ADC & adc, unsigned channel );

    /// Destructor
    virtual ~Pressure();

    /// Returns pressure measurement in bar
    double getBar() const;

private:
    ADC    & m_adc;     ///< Reference to the ADC
    unsigned m_channel; ///< ADC channel number
};

//-----------------------------------------------------------------------------

#endif//__pressure_h
