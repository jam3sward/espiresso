#include "gpiopin.h"
#include "timing.h"
#include "pigpiomgr.h"

using namespace std;

//-----------------------------------------------------------------------------

GPIOPin::GPIOPin( unsigned pin ) :
	m_pin( pin ),
	m_open( false ),
    m_edge( Rising ),
    m_edgeFunc( nullptr ),
    m_callback( -1 )
{
    open();
}

//-----------------------------------------------------------------------------

GPIOPin::~GPIOPin()
{
	close();
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setOutput( bool output )
{
    if ( m_open ) set_mode( m_pin, output ? PI_OUTPUT : PI_INPUT );
	return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setState( bool state )
{
    if ( m_open ) gpio_write( m_pin, state ? 1 : 0 );
	return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setPWMDuty( double duty )
{
    if ( m_open )
        set_PWM_dutycycle( m_pin, static_cast<unsigned>(duty * 255.0 + 0.5) );
    return *this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::setPWMFrequency( unsigned frequency )
{
    if ( m_open )
        set_PWM_frequency( m_pin, frequency );
    return *this;
}

//-----------------------------------------------------------------------------

bool GPIOPin::getState() const
{
    if ( m_open )
        return (gpio_read( m_pin ) != 0);
    else
        return false;
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
    if ( !m_open || m_edgeFunc ) return *this;
    m_edge = edge;
    return * this;
}

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::edgeFuncRegister( EdgeFunc edgeFunc )
{
    // check that the device is open
    if ( !m_open ) return *this;

    // remove any existing function
    edgeFuncCancel();

    // local static function used to forward events to the parent class
    struct local {
        static void callback(
            unsigned gpio,
            unsigned level,
            uint32_t tick,
            void    *userData
        ) {
            GPIOPin *self = reinterpret_cast<GPIOPin*>(userData);
            if ( self != 0 ) self->callback(
                gpio,
                (level != 0),
                static_cast<unsigned>(tick)
            );
        }
    };

    // install the function
    m_edgeFunc = edgeFunc;

    // register a callback
    unsigned edge = static_cast<unsigned>(m_edge);
    m_callback = callback_ex( m_pin, edge, local::callback, this );

    return *this;
}//edgeFuncRegister

//-----------------------------------------------------------------------------

GPIOPin & GPIOPin::edgeFuncCancel()
{
    // cancel the pigpiod callback
    if ( m_callback >= 0 ) {
        callback_cancel( m_callback );
        m_callback = -1;
    }

    // remove the user function
    m_edgeFunc = nullptr;

    return *this;
}//edgeFuncCancel

//-----------------------------------------------------------------------------

bool GPIOPin::poll( unsigned timeout )
{
    // check that the device is open
    if ( !m_open ) return false;

    // timeout in seconds
    double seconds = static_cast<double>(timeout) / 1000.0;

    // wait for specified edge
    unsigned edge = static_cast<unsigned>(m_edge);
    return (wait_for_edge( m_pin, edge, seconds ) == 1);
}//poll

//-----------------------------------------------------------------------------

bool GPIOPin::open()
{
    m_open = PIGPIOManager::get().ready();
    m_edge = Rising;
    return m_open;
}

//-----------------------------------------------------------------------------

void GPIOPin::close()
{
    // close any callback function
    edgeFuncCancel();

    // always set back to an input when closed
    setOutput( false );

    m_open = false;
}

//-----------------------------------------------------------------------------

void GPIOPin::callback( unsigned pin, bool level, unsigned tick)
{
    if ( m_edgeFunc ) m_edgeFunc( pin, level, tick );
}

//-----------------------------------------------------------------------------

bool GPIOPin::ready() const
{
    return m_open;
}

//-----------------------------------------------------------------------------
