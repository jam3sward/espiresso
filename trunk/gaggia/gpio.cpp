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

// GPIO access
volatile unsigned *gpio = 0;

//-----------------------------------------------------------------------------

bool gpio_initialise()
{
	// are we already initialised?
	if ( gpio != 0 ) return true;

	// open /dev/mem
	int fd = 0;
  	if ( (fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 )
		return false;

  	// mmap GPIO
	void *map = mmap(
		NULL,             		// Kernel chooses the address
		BLOCK_SIZE,       		// Size of the block
		PROT_READ | PROT_WRITE,
		MAP_SHARED,       		// Share with other processes
		fd,
		GPIO_BASE         		// Offset of GPIO
	);

	// close /dev/mem
  	close( fd );

  	if ( map == MAP_FAILED )
		return false;

	// pointer to mapped region
  	gpio = (volatile unsigned *)map;

	// success
	return true;
} // setup_io

//-----------------------------------------------------------------------------
