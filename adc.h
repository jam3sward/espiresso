#ifndef __ADC_h
#define __ADC_h

//-----------------------------------------------------------------------------

#include <string>
#include <inttypes.h>

//-----------------------------------------------------------------------------

/**
 * Represents the ADC interface
 */
class ADC {
public:
    /// Default constructor
    ADC();

    /// Destructor
    virtual ~ADC();

    /// Open the ADC, given the device path name and I2C address.
    /// Returns true for success, false in case of failure.
    bool open( const std::string & device, unsigned address );

    /// Read the voltage on the specified channel
    double getVoltage( unsigned channel );

    /// Close the ADC
    void close();

private:
    /// Copy constructor (unsupported)
    ADC( const ADC & );

    /// Assignment operator (unsupported)
    ADC & operator = ( const ADC & );

    /// Register addresses
    enum Register {
        REG_CONVERSION = 0, ///< Conversion register
        REG_CONFIG     = 1, ///< Configuration register
        REG_LO_THRESH  = 2, ///< Low threshold register
        REG_HI_THRESH  = 3  ///< High threshold register
    };

    /// Write register (returns true for success)
    bool writeRegister( Register address, uint16_t value );

    /// Read register (returns true for success)
    bool readRegister( Register address, uint16_t & value );

private:
    int m_file;     ///< File for communication with I2C device
};

//-----------------------------------------------------------------------------

#endif//__ADC_h
