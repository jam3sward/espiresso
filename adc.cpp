#include "adc.h"
#include <array>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

//-----------------------------------------------------------------------------

ADC::ADC() :
    m_file( -1 )
{
}

//-----------------------------------------------------------------------------

ADC::~ADC()
{
}

//-----------------------------------------------------------------------------

bool ADC::open( const std::string & device, unsigned address )
{
    // close if already open
    close();

    // attempt to open
    m_file = ::open( device.c_str(), O_RDWR );
    if ( m_file < 0 ) return false;

    // select the I2C slave address
    if ( ioctl( m_file, I2C_SLAVE, address ) < 0 ) {
        close();
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

double ADC::getVoltage( unsigned channel )
{
    // check that the device is open
    if ( m_file < 0 ) return false;

    const uint16_t CFG_COMP_QUE_AFTER1  = 0x00;     // Fire after 1 conversion
    const uint16_t CFG_COMP_QUE_AFTER2  = 0x01;     // Fire after 2 conversions
    const uint16_t CFG_COMP_QUE_AFTER4  = 0x02;     // Fire after 4 conversions
    const uint16_t CFG_COMP_QUE_DISABLE = 0x03;     // Disable comparator
    const uint16_t CFG_COMP_LAT_ENABLE  = (1<<2);   // Latching comparator
    const uint16_t CFG_COMP_POL_HIGH    = (1<<3);   // Comparator active high
    const uint16_t CFG_COMP_MODE_WINDOW = (1<<4);   // Comparator window mode
    const uint16_t CFG_DATA_RATE_128    = (0<<5);   // 128 samples/s
    const uint16_t CFG_DATA_RATE_250    = (1<<5);   // 250 samples/s
    const uint16_t CFG_DATA_RATE_490    = (2<<5);   // 490 samples/s
    const uint16_t CFG_DATA_RATE_920    = (3<<5);   // 920 samples/s
    const uint16_t CFG_DATA_RATE_1600   = (4<<5);   // 1600 samples/s (default)
    const uint16_t CFG_DATA_RATE_2400   = (5<<5);   // 2400 samples/s
    const uint16_t CFG_DATA_RATE_3300   = (4<<5);   // 3300 samples/s
    const uint16_t CFG_DATA_RATE_MAX    = (5<<5);   // Also 3300 samples/s
    const uint16_t CFG_MODE_CONTINUOUS  = (0<<8);   // Continuous conversion
    const uint16_t CFG_MODE_SINGLE_SHOT = (1<<8);   // Single-shot (default)
    const uint16_t CFG_PGA_FS_6_144     = (0<<9);   // +/-6.144V
    const uint16_t CFG_PGA_FS_4_096     = (1<<9);   // +/-4.096V
    const uint16_t CFG_PGA_FS_2_048     = (2<<9);   // +/-2.048V (default)
    const uint16_t CFG_PGA_FS_1_024     = (3<<9);   // +/-1.024V
    const uint16_t CFG_PGA_FS_0_512     = (4<<9);   // +/-0.512V
    const uint16_t CFG_PGA_FS_0_256     = (5<<9);   // +/-0.256V
    const uint16_t CFG_OS_BEGIN_CONV    = (1<<15);  // Begin conversion
    const uint16_t CFG_MUX_A0           = (4<<12);  // A0 single input
    const uint16_t CFG_MUX_A1           = (5<<12);  // A1 single input
    const uint16_t CFG_MUX_A2           = (6<<12);  // A2 single input
    const uint16_t CFG_MUX_A3           = (7<<12);  // A3 single input

    // converts integer channel number to bit mask
    const std::array<uint16_t,4> channelMap{
        CFG_MUX_A0, CFG_MUX_A1, CFG_MUX_A2, CFG_MUX_A3
    };

    if ( channel > channelMap.size() ) return 0.0;

    // initialise config register and start conversion of A0
    if ( !writeRegister(
          REG_CONFIG,
          CFG_COMP_QUE_DISABLE  // Disable comparator
        | CFG_MODE_SINGLE_SHOT  // Single-shot conversion mode
        | CFG_DATA_RATE_MAX     // Maximum data rate
        | CFG_PGA_FS_4_096      // 4.096V full scale
        | channelMap[channel]   // Select single ended input A0..A3
        | CFG_OS_BEGIN_CONV     // Begin conversion
    ) ) {
        // write failed
        return 0.0;
    }

    // wait for conversion to complete
    uint16_t value = 0;
    do {
        if ( !readRegister( REG_CONFIG, value ) ) {
            // read failed
            return 0.0;
        }
    } while ( (value & CFG_OS_BEGIN_CONV) == 0 );

    // read the conversion register
    uint16_t result = 0;
    if ( !readRegister( REG_CONVERSION, result ) ) {
        // read failed
        return 0.0;
    }

    // convert to voltage
    return static_cast<double>(result) * 4.096 / 32752.0;
}

//-----------------------------------------------------------------------------

void ADC::close()
{
    if ( m_file > 0 ) {
        ::close( m_file );
        m_file = -1;
    }
}

//-----------------------------------------------------------------------------

bool ADC::writeRegister( Register address, uint16_t value )
{
    // build the command buffer
    uint8_t buffer[] = {
        static_cast<uint8_t>(address),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value & 0xFF)
    };

    // send the command
    return ( write( m_file, buffer, sizeof(buffer) ) == sizeof(buffer) );
}

//-----------------------------------------------------------------------------

bool ADC::readRegister( Register address, uint16_t & value )
{
    // initialise value to zero in case of early exit
    value = 0;

    // set the pointer register to specify the register address
    uint8_t pointer = static_cast<uint8_t>(address);
    if ( write( m_file, &pointer, 1 ) != 1 )
        return false;

    // read the register (2 bytes)
    uint8_t result[2] = {};
    if ( read( m_file, result, sizeof(result) ) != sizeof(result) )
        return false;

    // return the result
    value = ((static_cast<uint16_t>(result[0])) << 8) + result[1];
    return true;
}

//-----------------------------------------------------------------------------
