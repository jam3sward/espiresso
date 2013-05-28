#ifndef __gpio_h
#define __gpio_h

//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>

//-----------------------------------------------------------------------------

bool gpio_initialise();

//-----------------------------------------------------------------------------

#define BCM2708_PERI_BASE 0x20000000
#define GPIO_BASE	  (BCM2708_PERI_BASE + 0x200000)

#define PAGE_SIZE  (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO access
extern volatile unsigned *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or
// SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) \
  *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

// set bits
#define GPIO_SET *(gpio+7)

// clear bits
#define GPIO_CLR *(gpio+10)

// read GPIO input bits 0-31
#define GPIO_IN0   *(gpio+13)

// pull up/pull down
#define GPIO_PULL   *(gpio+37)

// pull up/pull down clock
#define GPIO_PULLCLK0 *(gpio+38)

//-----------------------------------------------------------------------------

#endif//__gpio_h

