#include "keyboard.h"
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <sched.h>

//-----------------------------------------------------------------------------

int kbhit()
{
    struct timeval tv;
    fd_set set;
    tv.tv_sec  = 0;
    tv.tv_usec = 0;
    FD_ZERO( &set );
    FD_SET( STDIN_FILENO, &set );
    select( STDIN_FILENO+1, &set, 0, 0, &tv );
    return FD_ISSET( STDIN_FILENO, &set );
}

//-----------------------------------------------------------------------------

void nonblock( int nblock )
{
	struct termios state;
	tcgetattr( STDIN_FILENO, &state );

	if (nblock) {
		state.c_lflag &= ~ICANON;
		state.c_cc[VMIN] = 1;
	} else {
		state.c_lflag |= ICANON;
	}

	tcsetattr( STDIN_FILENO, TCSANOW, &state );
}

//-----------------------------------------------------------------------------
