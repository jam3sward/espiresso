#ifndef __timing_h
#define __timing_h

//-----------------------------------------------------------------------------

void delayms( unsigned ms );

void delayus ( unsigned us );

double getClock();

//-----------------------------------------------------------------------------

/// Simple timer class
class Timer {
public:
    /// Default constructor
    Timer();

    /// Reset the timer: also starts the timer
    Timer & reset();

    /// Start the timer
    Timer & start();

    /// Stop the timer: returns elapsed time
    double stop();

    /// Returns the elapsed time in seconds
    double getElapsed() const;

    /// Returns true if timer is running
    bool isRunning() const;

private:
    double  m_startTime;    ///< Start time stamp in seconds
    double  m_stopTime;     ///< Stop time in seconds
    bool    m_running;      ///< Timer is running
};

//-----------------------------------------------------------------------------

#endif//__timing_h
