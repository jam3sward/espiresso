#include <sstream>
#include <iomanip>
#include <math.h>
#include "display.h"
#include "timing.h"
#include "settings.h"

using namespace std;

//-----------------------------------------------------------------------------

Display::Display() :
	m_display( 0 ),
	m_font( 0 ),
	m_run( true ),
	m_dirty( false ),
	m_degrees( 0.0 ),
    m_pressure( 0.0 ),
	m_level( 0.0 ),
    m_powerOn( false ),
    m_powerIcon( 0 ),
    m_pumpOn( false ),
    m_pumpIcon( 0 ),
	m_width( 320 ),
	m_height( 240 )
{
	open();
	m_thread = std::thread( &Display::worker, this );
}

//-----------------------------------------------------------------------------

Display::~Display()
{
	m_run = false;
	m_thread.join();
	close();
}

//-----------------------------------------------------------------------------

Display & Display::updateTemperature( double degrees )
{
	std::lock_guard<std::mutex> lock( m_mutex );

	if ( fabs(degrees - m_degrees) > 0.05 )
		m_dirty = true;

	m_degrees = degrees;

	return *this;
}

//-----------------------------------------------------------------------------

Display & Display::updatePressure( double pressure )
{
	std::lock_guard<std::mutex> lock( m_mutex );

    if ( pressure < 0.0 ) pressure = 0.0;

    if ( fabs(pressure - m_pressure) >= 0.1 )
        m_dirty = true;

    m_pressure = pressure;

    return *this;
}

//-----------------------------------------------------------------------------

Display & Display::updateLevel( double level )
{
	std::lock_guard<std::mutex> lock( m_mutex );

	m_level = std::max( std::min(level, 1.0) , 0.0 );
	m_dirty = true;

	return *this;
}

//-----------------------------------------------------------------------------

Display & Display::updateTime( double time )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if ( time < 0.0 ) time = 0.0;

    m_time = time;
    m_dirty = true;

    return *this;
}

//-----------------------------------------------------------------------------

Display & Display::setPowerOn( bool powerOn )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if ( m_powerOn != powerOn ) {
        m_powerOn = powerOn;
        m_dirty = true;
    }

    return *this;
}

//-----------------------------------------------------------------------------

Display & Display::setPumpOn( bool pumpOn )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if ( m_pumpOn != pumpOn ) {
        m_pumpOn = pumpOn;
        m_dirty = true;
    }

    return *this;
}

//-----------------------------------------------------------------------------

bool Display::open()
{
	// todo: clean this up
	static const char *table[] = {
		"TSLIB_TSDEVICE=/dev/input/event0",
        "TSLIB_TSEVENTTYPE=INPUT",
        "TSLIB_CONFFILE=/etc/ts.conf",
        "TSLIB_CALIBFILE=/etc/pointercal",
        "SDL_FBDEV=/dev/fb1",
        "SDL_MOUSEDRV=TSLIB",
        "SDL_MOUSEDEV=/dev/input/event0",
        "SDL_NOMOUSE=1",
        "SDL_VIDEODRIVER=FBCON",
        0
    };

    for ( int i=0; table[i] != 0; ++i )
    	putenv( (char*)table[i] );

	// initialise SDL
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) return false;

	// get video information (resolution and bits per pixel)
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();

	// display resolution
	m_width  = videoInfo->current_w;
	m_height = videoInfo->current_h;

	// create display surface
	m_display = SDL_SetVideoMode(
		videoInfo->current_w,
		videoInfo->current_h,
		videoInfo->vfmt->BitsPerPixel,
		0
	);
	if ( m_display == 0 ) return false;

	// initialise TTF
	if ( TTF_Init() < 0 ) return false;

	// load the font
	m_font = TTF_OpenFont(
		"/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
		64
	);
	if ( m_font == 0 ) return false;

    // load the power icon
    SDL_Surface *temp = IMG_Load( ICON_BOILER_POWER );
    if ( temp == 0 ) return false;
    m_powerIcon = SDL_DisplayFormat( temp );
    SDL_FreeSurface( temp );
    if ( m_powerIcon == 0 ) return false;

    // load the pump icon
    temp = IMG_Load( ICON_PUMP_ACTIVE );
    if ( temp == 0 ) return false;
    m_pumpIcon = SDL_DisplayFormat( temp );
    SDL_FreeSurface( temp );
    if ( m_pumpIcon == 0 ) return false;

	// hide mouse pointer
	SDL_ShowCursor( 0 );

	// success
	return true;
}

//-----------------------------------------------------------------------------

void Display::close()
{
    // free power icon
    if ( m_powerIcon != 0 ) {
        SDL_FreeSurface( m_powerIcon );
        m_powerIcon = 0;
    }

    // free pump icon
    if ( m_pumpIcon != 0 ) {
        SDL_FreeSurface( m_pumpIcon );
        m_pumpIcon = 0;
    }

	// close font
	if ( m_font != 0 ) TTF_CloseFont( m_font );

	// close TTF
	TTF_Quit();

	// close SDL
	SDL_Quit();
}

//-----------------------------------------------------------------------------

void Display::worker()
{
	while (m_run) {
		if (m_dirty) {
			m_dirty = false;
			render();
		} else
			delayms(10);
	}
}

//-----------------------------------------------------------------------------

void Display::render()
{
    // clear the screen first
    SDL_FillRect( m_display, 0, 0 );

	double degrees  = 0.0;
    double pressure = 0.0;
	double level    = 0.0;
    double time     = 0.0;

	// size of screen border
	const int border = 10;

	{
		std::lock_guard<std::mutex> lock( m_mutex );
		degrees  = m_degrees;
        pressure = m_pressure;
		level    = m_level;
        time     = m_time;
	}

	static const SDL_Color
		black  = {   0,   0,   0, 255 },
        green  = {   0, 255,   0, 255 },
		yellow = { 255, 255,   0, 255 },
        cyan   = {   0, 255, 255, 255 };

	// format temperature value: 92.9
	stringstream text;
	text << std::fixed << std::setprecision(1) << degrees;

	// display temperature value
	drawText( m_font, border, border, text.str(), yellow, black );

    // format pressure value: 6.9
    text.str(string());
    text.clear();
    text << std::fixed << std::setprecision(1) << pressure;

    // display pressure value
    drawText( m_font, border, border + 80, text.str(), green, black );

    // format time value: 12.3
    text.str(string());
    text.clear();
    text << std::fixed << std::setprecision(1) << time;

    // display time
    drawText( m_font, border + 110, border + 80, text.str(), cyan, black );

	// RGB colours
	Uint32 rgbCyan = SDL_MapRGB( m_display->format, 0, 255, 255 );
	Uint32 rgbBlue = SDL_MapRGB( m_display->format, 0,   0, 255 );

	// draw water level bar
	short maxWidth = 300;
	Uint16 width = static_cast<Uint16>(
		level * static_cast<double>(maxWidth) + 0.5
	);
	Uint16 height = 10;
	short left	 = 10;
	short top	 = 240 - height - 10;
	SDL_Rect rect = { left, top, width, height };

	SDL_FillRect( m_display, &rect, rgbCyan );

	rect.x = left + width;
	rect.w = maxWidth - width;
	SDL_FillRect( m_display, &rect, rgbBlue );

    // draw power icon
    if ( m_powerOn && (m_powerIcon != 0) ) {
        SDL_Rect destRect = {
            static_cast<short>(m_width - 32 - border), border,
            0, 0
        };
        SDL_BlitSurface( m_powerIcon, 0, m_display, &destRect );
    }

    // draw pump icon
    if ( m_pumpOn && (m_pumpIcon != 0) ) {
        SDL_Rect destRect = {
            static_cast<short>(m_width - 32 - border), border + 32,
            0, 0
        };
        SDL_BlitSurface( m_pumpIcon, 0, m_display, &destRect );
    }

    // flip the display buffers
	SDL_Flip( m_display );
}

//-----------------------------------------------------------------------------

void Display::drawText(
	TTF_Font *font,
	short x, short y,
	const std::string & text,
	SDL_Color foregroundColour,
	SDL_Color backgroundColour
) {
	if ( m_display == 0 ) return;

	SDL_Surface *textSurface = TTF_RenderText_Shaded(
		font,
		text.c_str(),
		foregroundColour,
		backgroundColour
	);
	if ( textSurface == 0 ) return;

	SDL_Rect destRect = { x, y };

	SDL_BlitSurface( textSurface, 0, m_display, &destRect );

	SDL_FreeSurface( textSurface );
}

//-----------------------------------------------------------------------------
