#include "timing.h"
#include <sys/time.h>
#include <time.h>

//-----------------------------------------------------------------------------

/// defines which clock we use for timing:
/// CLOCK_MONOTONIC_RAW = raw hardware based time, not subject to NTP
///   adjustments or incremental adjustments performed by adjtime. Reports 2ns
///   resolution on Raspbian and two successive reads take about 2us.
static const clockid_t g_timingClockId = CLOCK_MONOTONIC_RAW;

//-----------------------------------------------------------------------------

void delayms( unsigned ms )
{
	struct timespec req, rem;

	req.tv_sec  = (time_t)(ms / 1000) ;
  	req.tv_nsec = (long)(ms % 1000) * 1000000L;

  	nanosleep( &req, &rem );
}

//-----------------------------------------------------------------------------

void delayus ( unsigned us )
{
  	struct timeval now;
  	gettimeofday( &now, 0 );

	struct timeval delay, end;
  	delay.tv_sec  = us / 1000000 ;
  	delay.tv_usec = us % 1000000 ;
  	timeradd( &now, &delay, &end );

  	while ( timercmp( &now, &end, <) )
		gettimeofday( &now, 0 );
}

//-----------------------------------------------------------------------------

double getClock()
{
	struct timespec now;
	clock_gettime( g_timingClockId, &now );

	return (double)now.tv_sec + (double)now.tv_nsec * 1.0E-9;
}

//-----------------------------------------------------------------------------

Timer::Timer()
{
    reset();
}

//-----------------------------------------------------------------------------

Timer & Timer::reset()
{
    m_startTime = getClock();
    m_stopTime  = m_startTime;
    m_running   = true;
    return *this;
}

//-----------------------------------------------------------------------------

Timer & Timer::start()
{
    m_startTime = getClock();
    m_running   = true;
}

//-----------------------------------------------------------------------------

double Timer::stop()
{
    m_stopTime  = getClock();
    m_running   = false;
    return m_stopTime - m_startTime;
}

//-----------------------------------------------------------------------------

double Timer::getElapsed() const
{
    if ( m_running )
        return getClock() - m_startTime;
    else
        return m_stopTime - m_startTime;
}

//-----------------------------------------------------------------------------

bool Timer::isRunning() const
{
    return m_running;
}

//-----------------------------------------------------------------------------

