#ifndef __settings_h
#define __settings_h

//-----------------------------------------------------------------------------

// GPIO pin used for Solid State Relay
// NOTE: this is now using hardware PWM
#define SSRPIN 18

// GPIO pin used for the Pump
#define PUMP_PIN 15
#define PUMP_PWM_PIN 14

// GPIO pin used by flow sensor
#define FLOWPIN 27

// GPIO pins used by ultrasonic ranger
#define RANGER_TRIGGER_OUT 23
#define RANGER_ECHO_IN 22

// GPIO pin used by the TSIC 306 temperature sensor
#define TSIC_PIN 24

// I2C directory path
#define I2C_DEVICE_PATH "/dev/i2c-1"

// ADS1015 ADC I2C address
#define ADS1015_ADC_I2C_ADDRESS 0x48

// ADC channel used by the button inputs
#define ADC_BUTTON_CHANNEL 0

// ADC channel used by the pressure sensor
#define ADC_PRESSURE_CHANNEL 1

// Maps logical button number to physical button number
// Where BUTTON1 is the top button on the panel, and the defined values
// correspond to the button numbers from the ADC
#define BUTTON1 3
#define BUTTON2 2

//-----------------------------------------------------------------------------

#endif//__settings_h
