#ifndef GAUGESCREENVIEW_HPP
#define GAUGESCREENVIEW_HPP

#include <gui_generated/gaugescreen_screen/GaugeScreenViewBase.hpp>
#include <gui/gaugescreen_screen/GaugeScreenPresenter.hpp>
#include <images/BitmapDatabase.hpp>

class GaugeScreenView : public GaugeScreenViewBase
{
public:
    GaugeScreenView();
    virtual ~GaugeScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void increasePressure();
    virtual void decreasePressure();

protected:
    void changeValue(const int16_t change);
    void moveNeedleToPostion(uint16_t postion);

    struct NeedlePosition
    {
        int16_t x;
        int16_t y;
        uint16_t bitmapId;
    };

    bool isAnimating = false;
    uint32_t tickCount = 0;

    static const uint16_t NUMBER_OF_POSITIONS = 13;
    const NeedlePosition needlePositions[NUMBER_OF_POSITIONS] =
    {
        { 28,    150,    BITMAP_GAUGE_NEEDLE_00_ID},
        {  6,    136,    BITMAP_GAUGE_NEEDLE_01_ID},
        {  0,    117,    BITMAP_GAUGE_NEEDLE_02_ID},
        {  6,     68,    BITMAP_GAUGE_NEEDLE_03_ID},
        { 30,     30,    BITMAP_GAUGE_NEEDLE_04_ID},
        { 68,      6,    BITMAP_GAUGE_NEEDLE_05_ID},
        {113,      0,    BITMAP_GAUGE_NEEDLE_06_ID},
        {136,      6,    BITMAP_GAUGE_NEEDLE_07_ID},
        {152,     30,    BITMAP_GAUGE_NEEDLE_08_ID},
        {163,     68,    BITMAP_GAUGE_NEEDLE_09_ID},
        {168,    113,    BITMAP_GAUGE_NEEDLE_10_ID},
        {163,    136,    BITMAP_GAUGE_NEEDLE_11_ID},
        {152,    150,    BITMAP_GAUGE_NEEDLE_12_ID},
    };
};

#endif // GAUGESCREENVIEW_HPP
