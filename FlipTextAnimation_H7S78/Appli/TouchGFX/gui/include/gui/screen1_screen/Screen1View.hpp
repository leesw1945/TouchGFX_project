#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // Designer에서 생성한 가상 함수
    virtual void incrementCounter();

    // 애니메이션 틱 처리
    virtual void handleTickEvent();

protected:
    // 현재 표시 중인 숫자
    uint8_t currentNumber;
    uint8_t targetNumber;

    // 애니메이션 상태
    bool isAnimatingTens;
    bool isAnimatingOnes;
    uint16_t animationStep;
    static const uint16_t ANIMATION_DURATION = 80;

    // 타이머용 컨테이너 (handleTickEvent를 받기 위해)
    Container tickHandler;

    // 이미지 ID를 저장할 배열
    uint16_t upperImageIds[10];
    uint16_t lowerImageIds[10];

    // 경계선을 위한 Box 위젯
    touchgfx::Box tensMiddleLine;
    touchgfx::Box onesMiddleLine;

    // 헬퍼 함수
    void startFlipAnimation(uint8_t newNumber);
    void updateFlipAnimation();
    void setDigitImages(uint8_t number);
};

#endif // SCREEN1VIEW_HPP
