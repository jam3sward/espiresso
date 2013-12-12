#include "gpiopin.h"
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include "timing.h"

using namespace std;

//-----------------------------------------------------------------------------

GPIOPin::GPIOPin( int pin ) :
	m_pin( pin ),
	m_file(-1)
{
	if ( exportPin() ) open();
}

//-----------------------------------------------------------------------------

GPIOPin::~GPIOPin()
{
	close();
	unexportPin();
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setOutput( bool output )
{
	ofstream fp( (m_path + "direction").c_str() );
	if ( !fp ) return *this;

	if ( output )
		fp << "out";
	else
		fp << "in";

	return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setState( bool state )
{
	if ( m_file < 0 ) return *this;

	char buffer[] = "0\n";
	if ( state ) buffer[0] = '1';
	int result = write( m_file, &buffer, sizeof(buffer)-1 );

	return *this;
}

//-----------------------------------------------------------------------------

bool GPIOPin::getState() const
{
	if ( m_file < 0 ) return false;

	char buffer[8] = {};
	int result = read( m_file, &buffer, sizeof(buffer)-1 );

	return ( result > 0 ) && ( buffer[0] == '1' );
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::usPulse( bool state, unsigned us )
{
	setState( state );
	delayus( us );
	setState( !state );
	return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::msPulse( bool state, unsigned ms )
{
	setState( state );
	delayms( ms );
	setState( !state );
	return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setEdgeTrigger( GPIOPin::Edge edge )
{
	ofstream fp( (m_path + "edge").c_str() );
	if ( !fp ) return *this;

	switch ( edge ) {
	case Falling:
		fp << "falling";
		break;

	case Rising:
		fp << "rising";
		break;

	case Both:
		fp << "both";
		break;

	case None:
	default:
		fp << "none";
	}

	return *this;
}

//-----------------------------------------------------------------------------

bool GPIOPin::poll( unsigned timeout )
{
	getState();

	struct pollfd pfd = {};
	pfd.fd 		= m_file;
	pfd.events 	= POLLPRI;

	int result = ::poll( &pfd, 1, timeout );

	return (result > 0);
}

//-----------------------------------------------------------------------------

bool GPIOPin::exportPin()
{
	ofstream fp( "/sys/class/gpio/export" );
	if ( !fp ) return false;
	fp << m_pin;
	return !fp.fail();
}

//-----------------------------------------------------------------------------

bool GPIOPin::unexportPin()
{
	ofstream fp( "/sys/class/gpio/unexport" );
	if ( !fp ) return false;
	fp << m_pin;
	return !fp.fail();
}

//-----------------------------------------------------------------------------

bool GPIOPin::open()
{
	// initialise GPIO path string
	{
		stringstream ss;
		ss << "/sys/class/gpio/gpio" << m_pin << '/';
		m_path = ss.str();
	}

	// open GPIO X value "/sys/class/gpio/gpioX/value"
	string filePath( m_path + "value" );
	m_file = ::open( filePath.c_str(), O_RDWR );
	if ( m_file < 0 ) {
		// failed to open
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------

void GPIOPin::close()
{
	if ( m_file >= 0 ) {
		::close( m_file );
		m_file = -1;
	}
}

//-----------------------------------------------------------------------------

bool GPIOPin::ready() const
{
	return (m_file >= 0);
}

//-----------------------------------------------------------------------------
