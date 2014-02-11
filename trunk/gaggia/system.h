#ifndef __system_h
#define __system_h

//-----------------------------------------------------------------------------

class System {
public:
    System();

    virtual ~System();

    /// Returns core temperature in degrees C
    double getCoreTemperature() const;
};

//-----------------------------------------------------------------------------

#endif//__system_h
