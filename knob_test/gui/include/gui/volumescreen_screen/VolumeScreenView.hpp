#ifndef VOLUMESCREENVIEW_HPP
#define VOLUMESCREENVIEW_HPP

#include <gui_generated/volumescreen_screen/VolumeScreenViewBase.hpp>
#include <gui/volumescreen_screen/VolumeScreenPresenter.hpp>

class VolumeScreenView : public VolumeScreenViewBase
{
public:
    VolumeScreenView();
    virtual ~VolumeScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void increaseVolume();
    virtual void decreaseVolume();
protected:

    void changeVolume(int change);

    bool isAnimating = false;
};

#endif // VOLUMESCREENVIEW_HPP
