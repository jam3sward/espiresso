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

    /// Set correction factors (scale and offset)
    void setCorrection( double scale, double offset );

private:
    ADC    & m_adc;     ///< Reference to the ADC
    unsigned m_channel; ///< ADC channel number
    double   m_scale;   ///< Scale factor
    double   m_offset;  ///< Constant offset
};

//-----------------------------------------------------------------------------

#endif//__pressure_h
