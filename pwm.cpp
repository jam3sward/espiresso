#include <unistd.h>
#include <assert.h>
#include "pwm.h"
#include "gpio.h"
#include "timing.h"
#include "settings.h"

#define PWMCLK_CNTL  *(BCM::clk+40)
#define PWMCLK_DIV   *(BCM::clk+41)

#define PWM_CONTROL *(BCM::pwm)
#define PWM_STATUS  *(BCM::pwm+1)
#define PWM1_RANGE  *(BCM::pwm+4)
#define PWM1_DATA   *(BCM::pwm+5)

// PWM Control register bits for Channel 1
const unsigned PWM1_MS_ENABLE   = 0x0080;	///< Mark/Space Enable (MSEN1)
const unsigned PWM1_CLEAR_FIFO  = 0x0040;	///< Clear FIFO (CLRF1)
const unsigned PWM1_USE_FIFO    = 0x0020;	///< Use FIFO (USEF1) 1=Enable
const unsigned PWM1_POLARITY    = 0x0010; 	///< Polarity (POLA1) 1=Reverse 
const unsigned PWM1_SILENCE_BIT = 0x0008; 	///< Silence bit (SBIT1)
const unsigned PWM1_REPEAT_LAST = 0x0004;	///< Repeat last data (RPTL1)
const unsigned PWM1_MODE        = 0x0002;	///< Serialiser mode (MODE1)
const unsigned PWM1_ENABLE      = 0x0001;	///< Channel enable (PWEN1)

//-----------------------------------------------------------------------------

PWM::PWM() :
	m_enabled( false ),
	m_mode( PWM1_ENABLE | PWM1_MS_ENABLE ),
	m_divisor( 375 ),
	m_range( 1024 ),
	m_value( 512 )
{
	BCM::open();
	initialise();
}

//-----------------------------------------------------------------------------

void PWM::enable()
{
	PWM_CONTROL = m_mode;
	delayus(1);
	m_enabled = true;
}

//-----------------------------------------------------------------------------

void PWM::disable()
{
	PWM_CONTROL = 0;
	delayus(1);
	m_enabled = false;
}

//-----------------------------------------------------------------------------

void PWM::setDivisor( int divisor )
{
	bool wasEnabled = m_enabled;

	// disable PWM
	disable();

	// stop PWM clock
	PWMCLK_CNTL = 0x5A000001;
	sleep(1);

	// set PWM divisor
	PWMCLK_DIV  = 0x5A000000 | (divisor<<12);

	// clock control
	// TODO: should ensure BUSY is low first to avoid glitches / lock-up
	// bits 3-0: SRC  = 1 (oscillator)
	PWMCLK_CNTL = 0x5A000001;
	delayus(1);
	// also assert ENAB (bit 4) to enable the clock generator
	PWMCLK_CNTL = 0x5A000011;
	delayus(1);

	// enable PWM
	if ( wasEnabled ) enable();

	// store divisor
	m_divisor = divisor;
}

//-----------------------------------------------------------------------------

int PWM::getDivisor() const
{
	return m_divisor;
}

//-----------------------------------------------------------------------------

void PWM::setRange( int range )
{
	bool wasEnabled = m_enabled;

	// disable PWM
	disable();

	// set the PWM range
 	PWM1_RANGE = range;
	delayus(1);

	// enable PWM
	if ( wasEnabled ) enable();

	// store new range
	m_range = range;
}

//-----------------------------------------------------------------------------

int PWM::getRange() const
{
	return m_range;
}

//-----------------------------------------------------------------------------

double PWM::getFrequency() const
{
	// we only support mark-space mode currently
	assert( (m_mode & PWM1_MS_ENABLE) != 0 );

	// master PWM clock frequency
	static const double masterClock = 19.2E6;

	// calculate PWM frequency
	return masterClock / (
		static_cast<double>( getRange() ) *
		static_cast<double>( getDivisor() )
	);
}

//-----------------------------------------------------------------------------

void PWM::setIntegerValue( int value )
{
	bool wasEnabled = m_enabled;

	// disable PWM
	disable();

	// clamp the PWM value
	if ( value < 0) value = 0;
	if ( value > m_range ) value = m_range;

	// set the PWM value
	PWM1_DATA = value;
	delayus(1);

	// enable PWM
	if ( wasEnabled ) enable();

	// store value
	m_value = value;
}

//-----------------------------------------------------------------------------

int PWM::getIntegerValue() const
{
	return m_value;
}

//-----------------------------------------------------------------------------

void PWM::setValue( double value )
{
	setIntegerValue( value * static_cast<double>( getRange() ) );
}

//-----------------------------------------------------------------------------

double PWM::getValue() const
{
	return
		static_cast<double>( getIntegerValue() ) /
		static_cast<double>( getRange() );
}

//-----------------------------------------------------------------------------

void PWM::initialise()
{
	// configure GPIO18 as PWM output
	assert( SSRPIN == 18 );
	INP_GPIO( SSRPIN );
	SET_GPIO_ALT( SSRPIN, 5 );

	// apply default settings
	setDivisor( m_divisor );
	setRange( m_range );
	setValue( m_value );
}

//-----------------------------------------------------------------------------
