#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/widgets/TextureMapper.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // 버튼 클릭 핸들러 (TouchGFX Designer에서 생성된 함수들)
    virtual void incrementNumber();
    virtual void decrementNumber();

    // 타이머 콜백
    virtual void handleTickEvent() override;

protected:
    // ============ 애니메이션 관련 변수 ============
    uint8_t currentNumber;      // 현재 표시 중인 숫자 (0~99)
    uint8_t tensDigit;         // 십의 자리 (0~9)
    uint8_t onesDigit;         // 일의 자리 (0~9)

    bool isAnimating;          // 애니메이션 진행 중 여부
    uint8_t animationStep;     // 현재 애니메이션 단계
    uint8_t maxAnimationSteps; // 전체 애니메이션 단계 수

    bool animatingTens;        // 십의 자리 애니메이션 중
    bool animatingOnes;        // 일의 자리 애니메이션 중

    uint8_t nextTensDigit;     // 다음 십의 자리 숫자
    uint8_t nextOnesDigit;     // 다음 일의 자리 숫자

    // ============ 3D 효과 관련 변수 ============
    bool use3DEffect;          // 3D 효과 사용 여부
    bool imageSwapped;         // 90도 지점에서 이미지 교체 완료 여부

    // 3D TextureMapper 위젯들
    TextureMapper flipperTens;      // 십의 자리용 3D 위젯
    TextureMapper flipperOnes;      // 일의 자리용 3D 위젯

    // ============ Container 내부 위젯 참조 (getChild 대신 사용) ============
    Image* imgCurrentTens;
    Image* imgTopTens;
    Image* imgBottomTens;
    Image* imgNextTens;

    Image* imgCurrentOnes;
    Image* imgTopOnes;
    Image* imgBottomOnes;
    Image* imgNextOnes;

    // ============ 애니메이션 함수들 ============
    void startFlipAnimation(bool tens, bool ones);
    void updateFlipAnimation();
    void update3DFlipAnimation(float angle);
    void update2DFlipAnimation(float angle);
    void finishFlipAnimation();

    // ============ 숫자 업데이트 함수들 ============
    void updateDigitDisplay();
    void setDigitImages(bool isTens, uint8_t digit, bool isNext = false);
    void findContainerChildren();  // Container 내부 위젯들 찾기

    // ============ 3D 효과 함수들 ============
    void setup3DFlippers();
    void toggle3DMode();

    // ============ 유틸리티 함수들 ============
    float applyCubicEaseOut(float progress);
    void updateNumber(int8_t delta);
};

// ============ 이미지 ID 배열들 (extern 선언) ============
extern const uint16_t numberImages[10];
extern const uint16_t numberTopImages[10];
extern const uint16_t numberBottomImages[10];

#endif // SCREEN1VIEW_HPP
