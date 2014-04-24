#include <array>
#include <future>
#include "inputs.h"
#include "settings.h"
#include "timing.h"

//-----------------------------------------------------------------------------

Inputs::Inputs() :
	m_button1( BUTTON1_PIN ),
	m_button2( BUTTON2_PIN ),
    m_notifyFunc( nullptr ),
    m_run( true ),
    m_thread( &Inputs::worker, this )
{
}

//-----------------------------------------------------------------------------

Inputs::~Inputs()
{
    // gracefully terminate the thread
    m_run = false;

    // wait for the thread to terminate
    m_thread.join();
}

//-----------------------------------------------------------------------------

bool Inputs::getButton( int button ) const
{
	switch ( button ) {
	case 1: return !m_button1.getState(); break;
	case 2: return !m_button2.getState(); break;
	}
	return false;
}

//-----------------------------------------------------------------------------

Inputs & Inputs::notifyRegister( NotifyFunc func )
{
    // set up the notification
    std::lock_guard<std::mutex> lock( m_mutex );
    m_notifyFunc = func;

    return *this;
}

//-----------------------------------------------------------------------------

Inputs & Inputs::notifyCancel()
{
    // clear the notification
    std::lock_guard<std::mutex> lock( m_mutex );
    m_notifyFunc = nullptr;

    return *this;
}

//-----------------------------------------------------------------------------

void Inputs::worker()
{
    // set up the button inputs
    m_button1.setOutput( false );
    m_button2.setOutput( false );

    // time period for polling
    const unsigned period = 10;

    struct State {
        bool state;
        bool oldState;
        double timeStamp;
    };

    // button states
    std::array<State,2> button;
    for (size_t i=0; i<button.size(); ++i) {
        button[i].oldState = false;
        button[i].timeStamp = getClock();
    }

    // poll the buttons
    while (m_run) {
        // sample the button state (active low: 0=down, 1=up)
        button[0].state = !m_button1.getState();
        button[1].state = !m_button2.getState();

        // for each button
        for (size_t i=0; i<button.size(); ++i) {
            // if the button state has changed
            if ( button[i].state != button[i].oldState ) {
                // calculate elapsed time since last event
                double now = getClock();
                double elapsed = now - button[i].timeStamp;
                button[i].timeStamp = now;

                std::lock_guard<std::mutex> lock( m_mutex );

                // fire the notification function
                if ( m_notifyFunc ) try {
                    std::async(
                        std::launch::async,
                        m_notifyFunc,
                        i+1, button[i].state, elapsed
                    );
                } catch ( const std::system_error & e ) {
                }
            }

            // store the old button state and time stamp
            button[i].oldState = button[i].state;
        }

        // sleep for a while
        delayms( period );
    }
}//worker

//-----------------------------------------------------------------------------
