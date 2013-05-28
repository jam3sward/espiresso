#include "timing.h"
#include <sys/time.h>
#include <time.h>

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
	struct timeval now;
	gettimeofday( &now, 0 );

	return (double)now.tv_sec + (double)now.tv_usec * 1.0E-6;
}

//-----------------------------------------------------------------------------
