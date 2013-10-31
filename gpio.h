#ifndef __gpio_h
#define __gpio_h

//-----------------------------------------------------------------------------

namespace BCM {

extern volatile unsigned *gpio;
extern volatile unsigned *pwm;
extern volatile unsigned *clk;

/// Open Broadcom peripheral interface
bool open();

/// Close Broadcom peripheral interface
void close();

} // namespace BCM

//-----------------------------------------------------------------------------

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or
// SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(BCM::gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(BCM::gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) \
  *(BCM::gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

// set bits
#define GPIO_SET *(BCM::gpio+7)

// clear bits
#define GPIO_CLR *(BCM::gpio+10)

// read GPIO input bits 0-31
#define GPIO_IN0   *(BCM::gpio+13)

// pull up/pull down
#define GPIO_PULL   *(BCM::gpio+37)

// pull up/pull down clock
#define GPIO_PULLCLK0 *(BCM::gpio+38)

//-----------------------------------------------------------------------------

#endif//__gpio_h

