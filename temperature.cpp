//#include <stdio.h>
//#include <stdlib.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <unistd.h>
//#include <termios.h>
//#include <assert.h>
#include "temperature.h"
#include "gpio.h"
#include "timing.h"
#include "settings.h"

//-----------------------------------------------------------------------------

// ROM commands
#define DS_SEARCH_ROM 	0xF0
#define DS_READ_ROM		0x33
#define DS_MATCH_ROM	0x55
#define DS_SKIP_ROM		0xCC
#define DS_ALARM_SEARCH	0xEC

// DS18B20 function commands
#define DS_CONVERT_T		 0x44
#define DS_WRITE_SCRATCHPAD  0x4E
#define DS_READ_SCRATCHPAD	 0xBE
#define DS_COPY_SCRATCHPAD	 0x48
#define DS_RECALL_E2		 0xB8
#define DS_READ_POWER_SUPPLY 0xB4

//-----------------------------------------------------------------------------

Temperature::Temperature() :
	m_mask( 0xFFFF )
{
	gpio_initialise();
}

//-----------------------------------------------------------------------------

bool Temperature::initialise()
{
	// float high
	INP_GPIO(DSPIN);
	delayms(1);

	// ensure that bus is high initially
	// (if low, looks like a hardware problem)
	if ( (GPIO_IN0 & (1<<DSPIN)) == 0 )	return 0;

	// pull the bus low for 480us (the minimum)
	// this is the "MASTER Tx RESET PULSE"
	OUT_GPIO(DSPIN);
	GPIO_CLR = 1<<DSPIN;
	delayus(480);

	// float high
	INP_GPIO(DSPIN);

	// DS18B20 waits 15-60us before pulling low
	delayus(65);

	// check for DS18B20 presence pulse
	bool found = ( (GPIO_IN0 & (1<<DSPIN)) == 0 );

	// wait for remainder of time slice
	delayus( 240-60 );

	// did we find the device?
	return found;
}

//-----------------------------------------------------------------------------

void Temperature::writeByte( unsigned char data )
{
	int i;

	INP_GPIO(DSPIN);
	for (i=0; i<8; ++i) {
		OUT_GPIO(DSPIN);
		GPIO_CLR = 1<<DSPIN;
		if ( data & (1<<i) ) {
			delayus(1);
			INP_GPIO(DSPIN);
			delayus(60);
		} else {
			delayus(60);
			INP_GPIO(DSPIN);
			delayus(1);
		}
	}
}

//-----------------------------------------------------------------------------

unsigned char Temperature::readByte()
{
	unsigned char data = 0;

	INP_GPIO(DSPIN);
	int i;
	for (i=0; i<8; ++i) {
		OUT_GPIO(DSPIN);
		GPIO_CLR = 1<<DSPIN;
		//delayus(1);
		INP_GPIO(DSPIN);
		delayus(10);
		if ( GPIO_IN0 & (1<<DSPIN) )
			data |= 1<<i;
		delayus(50);
	}

	return data;
}

//-----------------------------------------------------------------------------

char temp_crc( char *crc, char data )
{
	static unsigned char table[] = {
	0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
	};

	*crc = table[*crc ^ data];

	return *crc;
}

//-----------------------------------------------------------------------------

bool Temperature::readData( char *data, size_t size )
{
	assert( size == 9 );

	// read the data
	size_t i;
	for (i=0; i<size; ++i) 
		data[i] = readByte();

	// calculate the CRC for the data
	char crc = 0;
	for (i=0; i<size-1; ++i)
		temp_crc( &crc, data[i] );

	// compare calculated CRC to the received CRC in the last byte
	return ( crc == data[size-1] );
}

//-----------------------------------------------------------------------------

void Temperature::convert()
{
	writeByte( DS_CONVERT_T );

	//unsigned char busy = 0;
	//while (busy == 0)
	//	busy = readByte();

	do {
		// pull low
		OUT_GPIO(DSPIN);
		GPIO_CLR = 1<<DSPIN;
		delayus(1);

		// float
		INP_GPIO(DSPIN);

		// wait 10us and sample bus
		// if high, conversion has finished
		delayus(10);
		if ( GPIO_IN0 & (1<<DSPIN) )
			return;

		// wait until end of read slot
		delayus(50);
	} while (1);
}

//-----------------------------------------------------------------------------

bool Temperature::readRaw( short *value )
{
	// initialise value
	if ( value != 0 ) *value = 0;

	// reset
	if ( !initialise() ) return false;

	// skip ROM (we only support a single sensor)
	writeByte( DS_SKIP_ROM );

	// convert
	convert();

	// reset
	if ( !initialise() ) return false;

	// skip ROM
	writeByte( DS_SKIP_ROM );

	// read scratchpad
	writeByte( DS_READ_SCRATCHPAD );
	unsigned char data[9];
	if ( !readData( (char*)data, sizeof(data) ) )
		return false;

	// return the value
	if ( value != 0 )
		*value = (short)(data[0] | ((short)data[1] << 8)) & m_mask;

	// success
	return true;
}

//-----------------------------------------------------------------------------

bool Temperature::read( double *value )
{
	// read raw temperature
	short raw = 0;
	if ( readRaw( &raw ) ) {
		// success

		// convert the value
		m_degrees = static_cast<double>(raw) / 16.0;

		// return the value
		if ( value != 0 ) *value = m_degrees;

		return true;
	} else {
		// return the last (cached) value
		if ( value != 0 ) *value = m_degrees;

		// failure
		return false;
	}
}

//-----------------------------------------------------------------------------

bool Temperature::setResolution( int bits )
{
	// clamp to sensible range
	if ( bits < 9 )
		bits = 9;
	else if ( bits > 12 )
		bits = 12;

	// reset
	if ( !initialise() ) return false;

	// read the scratchpad
	writeByte( DS_SKIP_ROM );
	writeByte( DS_READ_SCRATCHPAD );
	char data[9];
	if ( !readData( data, sizeof(data) ) )
		return 0;

	// configuration register value
	// bit 7 is 0
	// bits 6:5 are the number of bits - 9
	// bits 4:0 are 1
	char config = ((bits-9) << 5) | 0x1F;

	// reset
	if ( !initialise() ) return false;

	// write the scratchpad
	writeByte( DS_SKIP_ROM );
	writeByte( DS_WRITE_SCRATCHPAD );
	writeByte( data[2] );	// TH (existing value)
	writeByte( data[3] );	// TL (existing value)
	writeByte( config );	// Configuration register

	// compute the bit mask
	m_mask = (0xFFF8 >> (bits-9)) | 0xFFF0;

	// currently, we assume it worked
	return true;
}

//-----------------------------------------------------------------------------
