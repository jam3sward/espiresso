#ifndef __inputs_h
#define __inputs_h

//-----------------------------------------------------------------------------

#include <thread>
#include <mutex>
#include "gpiopin.h"

//-----------------------------------------------------------------------------

/// Represents the inputs
class Inputs {
public:
	/// Default constructor
	Inputs();

	/// Destructor
	virtual ~Inputs();

	/// Get button state (given button index, starting from 1)
	bool getButton( int button ) const;

    /// Notification function type
    typedef std::function<void(int button, bool down, double time)> NotifyFunc;

    /// Register a function to receive notifications when a button is pressed
    /// or released. The notification function is called asynchronously from
    /// another thread.
    Inputs & notifyRegister( NotifyFunc func );

    /// Cancel notifications
    Inputs & notifyCancel();

private:
    /// Worker thread
    void worker();

private:
	GPIOPin	m_button1;	///< GPIO pin for button 1
	GPIOPin m_button2;	///< GPIO pin for button 2
    bool    m_run;      ///< Should thread continue to run?

    NotifyFunc m_notifyFunc;    ///< Notification function

    /// Thread used to monitor the inputs
    std::thread m_thread;

    /// Mutex to control access to shared members
    mutable std::mutex m_mutex;
};

//-----------------------------------------------------------------------------

#endif//__inputs_h
