#ifndef NEOCHROMTOGGLEBUTTON_COMPASS_HPP
#define NEOCHROMTOGGLEBUTTON_COMPASS_HPP

#include <gui_generated/containers/NeoChromToggleButton_CompassBase.hpp>

class NeoChromToggleButton_Compass : public NeoChromToggleButton_CompassBase
{
public:
    NeoChromToggleButton_Compass();
    virtual ~NeoChromToggleButton_Compass() {}

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

#endif // NEOCHROMTOGGLEBUTTON_COMPASS_HPP
