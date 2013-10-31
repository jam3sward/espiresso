#ifndef __pwm_h
#define __pwm_h

//-----------------------------------------------------------------------------

/**
 * Simplified interface to Raspberry PI hardware PWM. There is only one
 * instance. Common sense is assumed to reduce code complexity.
 */
class PWM {
public:
	/// Default constructor
	PWM();

	/// Enable PWM
	void enable();

	/// Disable PWM
	void disable();

	/// Set integer divisor
	void setDivisor( int divisor );

	/// Get integer divisor
	int getDivisor() const;

	/// Set integer range 
	void setRange( int range );

	/// Returns current integer range
	int getRange() const;

	/// Returns the PWM frequency in Hz
	double getFrequency() const;

	/// Set PWM value as an integer (0..range)
	void setIntegerValue( int value );

	/// Get PWM value as an integer (0..range)
	int getIntegerValue() const;

	/// Set PWM value (0..1)
	void setValue( double value );

	/// Get PWM value (0..1)
	double getValue() const;

private:
	/// Initialise PWM with defaults
	void initialise();

private:
	bool m_enabled;		///< is PWM enabled?
	int	 m_mode;		///< current PWM mode
	int  m_divisor;		///< current PWM divisor
	int  m_range;		///< current PWM range
	int	 m_value;		///< current value
};

//-----------------------------------------------------------------------------

#endif//__pwm_h

