#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include "timing.h"
#include "gpio.h"

//-----------------------------------------------------------------------------

const off_t BCM2708_PERI_BASE = 0x20000000;

const off_t BCM_BASE_CLOCK = (BCM2708_PERI_BASE + 0x101000);	///< Clocks
const off_t BCM_BASE_GPIO  = (BCM2708_PERI_BASE + 0x200000);	///< GPIO
const off_t BCM_BASE_PWM   = (BCM2708_PERI_BASE + 0x20C000);	///< PWM

const size_t BLOCK_SIZE = 4096;

//-----------------------------------------------------------------------------

volatile unsigned *BCM::gpio = 0;
volatile unsigned *BCM::clk  = 0;
volatile unsigned *BCM::pwm  = 0;

//-----------------------------------------------------------------------------

static volatile unsigned *mapRegion(
	int	   fd,		// file descriptor to /dev/mem
	size_t length,	// size of block
	off_t  offset	// offset of block
) {
	return reinterpret_cast<volatile unsigned *>( mmap(
		NULL,			// let the kernel choose the address for us
		length,			// size of the block to map
		PROT_READ  |	// read and write access
		PROT_WRITE,
		MAP_SHARED,		// share with other processes
		fd,				// file descriptor to /dev/mem
		offset			// base offset to map
	) );
}//mapRegion

//-----------------------------------------------------------------------------

bool BCM::open()
{
	// are we already initialised?
	if ( BCM::gpio != 0 ) return true;

	// open /dev/mem
	int fd = ::open( "/dev/mem", O_RDWR | O_SYNC );
	if ( fd < 0 ) return false;

	// map GPIO
	BCM::gpio = mapRegion( fd, BLOCK_SIZE, BCM_BASE_GPIO );
	if ( BCM::gpio == MAP_FAILED ) return false;

	// map Clocks
	BCM::clk = mapRegion( fd, BLOCK_SIZE, BCM_BASE_CLOCK );
	if ( BCM::clk == MAP_FAILED ) return false;

	// map PWM
	BCM::pwm = mapRegion( fd, BLOCK_SIZE, BCM_BASE_PWM );
	if ( BCM::pwm == MAP_FAILED ) return false;

	// close /dev/mem
  	::close( fd );

	// success
	return true;
}

//-----------------------------------------------------------------------------

void BCM::close()
{
	if ( BCM::gpio == 0 ) return;

	munmap( (void*)BCM::pwm,  BLOCK_SIZE );
	munmap( (void*)BCM::clk,  BLOCK_SIZE );
	munmap( (void*)BCM::gpio, BLOCK_SIZE );

	pwm  = 0;
	clk  = 0;
	gpio = 0;
}

//-----------------------------------------------------------------------------
