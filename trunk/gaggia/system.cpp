#include "system.h"
#include <fstream>

using namespace std;

//-----------------------------------------------------------------------------

System::System()
{
}

//-----------------------------------------------------------------------------

System::~System()
{
}

//-----------------------------------------------------------------------------

double System::getCoreTemperature() const
{
    ifstream f( "/sys/class/thermal/thermal_zone0/temp" );
    if ( !f ) return 0.0;

    long temp = 0;
    f >> temp;
    f.close();

    return static_cast<double>(temp) / 1000.0;
}

//-----------------------------------------------------------------------------

