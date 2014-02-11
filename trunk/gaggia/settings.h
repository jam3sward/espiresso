#ifndef __settings_h
#define __settings_h

//-----------------------------------------------------------------------------

// GPIO pin used for DS18B20
#define DSPIN 4

// GPIO pin used for Solid State Relay
// NOTE: this is now using hardware PWM
#define SSRPIN 18

// GPIO pin used by flow sensor
#define FLOWPIN 27

// GPIO pins used by ultrasonic ranger
#define RANGER_TRIGGER_OUT 23
#define RANGER_ECHO_IN 22

// GPIO pins used for buttons
#define BUTTON1_PIN 2
#define BUTTON2_PIN 3

//-----------------------------------------------------------------------------

#endif//__settings_h

