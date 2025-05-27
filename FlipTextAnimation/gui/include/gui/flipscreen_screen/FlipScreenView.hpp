#ifndef FLIPSCREENVIEW_HPP
#define FLIPSCREENVIEW_HPP

#include <gui_generated/flipscreen_screen/FlipScreenViewBase.hpp>
#include <gui/flipscreen_screen/FlipScreenPresenter.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/Box.hpp>  // 클리핑용 박스 추가
#include <BitmapDatabase.hpp>

class FlipScreenView : public FlipScreenViewBase
{
public:
    FlipScreenView();
    virtual ~FlipScreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

protected:
    // 자연스러운 플립을 위한 카드 구조체
    struct NaturalFlipCard {
        // Designer 위젯들
        touchgfx::Image* mainImage;
        touchgfx::Image* upperImage;  
        touchgfx::Image* lowerImage;
        
        // 클리핑을 위한 마스크 박스들 (동적 생성)
        touchgfx::Box upperMask;
        touchgfx::Box lowerMask;
        
        // 카드 정보
        int16_t cardX, cardY;
        int currentValue;
        int nextValue;
        int maxValue;
        bool isAnimating;
        int animationPhase;
        
        NaturalFlipCard() : mainImage(nullptr), upperImage(nullptr), lowerImage(nullptr),
                           cardX(0), cardY(0), currentValue(0), nextValue(0), maxValue(9), 
                           isAnimating(false), animationPhase(0) {}
    };
    
    NaturalFlipCard minuteTens;
    NaturalFlipCard minuteUnits;
    NaturalFlipCard secondTens;
    NaturalFlipCard secondUnits;
    
    uint32_t tickCounter;
    uint32_t totalSeconds;
    int animationTimer;
    
    void initializeFlipCard(NaturalFlipCard& card, 
                           touchgfx::Image* main,
                           touchgfx::Image* upper,
                           touchgfx::Image* lower,
                           int16_t x, int16_t y,
                           int maxVal);
    void updateTime();
    void startFlipAnimation(NaturalFlipCard& card, int newValue);
    void updateCardImage(NaturalFlipCard& card);
    void onAnimationComplete(NaturalFlipCard& card);
    
    touchgfx::BitmapId getFullImageId(int number);
    touchgfx::BitmapId getUpperImageId(int number);
    touchgfx::BitmapId getLowerImageId(int number);
};

#endif // FLIPSCREENVIEW_HPP