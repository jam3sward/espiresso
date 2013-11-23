#ifndef __settings_h
#define __settings_h

//-----------------------------------------------------------------------------

// GPIO pin used for DS18B20
#define DSPIN 17

// GPIO pin used for Solid State Relay
// NOTE: this is now using hardware PWM
#define SSRPIN 18

// GPIO pin used for shutdown button
#define HALTBUTTONPIN 24

// GPIO pin used by flow sensor
#define FLOWPIN 21

// GPIO pins used by ultrasonic ranger
#define RANGER_TRIGGER_OUT 25
#define RANGER_ECHO_IN 23

//-----------------------------------------------------------------------------

#endif//__settings_h

