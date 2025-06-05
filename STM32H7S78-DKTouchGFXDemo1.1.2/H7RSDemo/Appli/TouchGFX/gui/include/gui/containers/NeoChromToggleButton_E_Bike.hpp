#ifndef NEOCHROMTOGGLEBUTTON_E_BIKE_HPP
#define NEOCHROMTOGGLEBUTTON_E_BIKE_HPP

#include <gui_generated/containers/NeoChromToggleButton_E_BikeBase.hpp>

class NeoChromToggleButton_E_Bike : public NeoChromToggleButton_E_BikeBase
{
public:
    NeoChromToggleButton_E_Bike();
    virtual ~NeoChromToggleButton_E_Bike() {}

    virtual void initialize();
    virtual void toggleNeoChrom();
protected:

    void setButtonStateToOn();
    void setButtonStateToOff();

    void moveIndicatorsToOnState();
    void moveIndicatorsToOffState();

    void changeTextLabelColorsToOnState();
    void changeTextLabelColorsToOffState();
    void invalidateTextLabels();

    bool isNeoChromActive;
};

#endif // NEOCHROMTOGGLEBUTTON_E_BIKE_HPP
