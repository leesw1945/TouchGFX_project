#include <gui/screen1_screen/Screen1View.hpp>
#include "BitmapDatabase.hpp"
#include <touchgfx/Color.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Screen1View::Screen1View() :
    currentNumber(0),
    targetNumber(0),
    isAnimatingTens(false),
    isAnimatingOnes(false),
    animationStep(0)
{
    // tickHandler를 화면 밖에 배치 (보이지 않게)
    tickHandler.setPosition(-1, -1, 1, 1);
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    // 이미지 ID 배열 초기화
    upperImageIds[0] = BITMAP_NUMBER_0_UPPER_ID;
    upperImageIds[1] = BITMAP_NUMBER_1_UPPER_ID;
    upperImageIds[2] = BITMAP_NUMBER_2_UPPER_ID;
    upperImageIds[3] = BITMAP_NUMBER_3_UPPER_ID;
    upperImageIds[4] = BITMAP_NUMBER_4_UPPER_ID;
    upperImageIds[5] = BITMAP_NUMBER_5_UPPER_ID;
    upperImageIds[6] = BITMAP_NUMBER_6_UPPER_ID;
    upperImageIds[7] = BITMAP_NUMBER_7_UPPER_ID;
    upperImageIds[8] = BITMAP_NUMBER_8_UPPER_ID;
    upperImageIds[9] = BITMAP_NUMBER_9_UPPER_ID;

    lowerImageIds[0] = BITMAP_NUMBER_0_LOWER_ID;
    lowerImageIds[1] = BITMAP_NUMBER_1_LOWER_ID;
    lowerImageIds[2] = BITMAP_NUMBER_2_LOWER_ID;
    lowerImageIds[3] = BITMAP_NUMBER_3_LOWER_ID;
    lowerImageIds[4] = BITMAP_NUMBER_4_LOWER_ID;
    lowerImageIds[5] = BITMAP_NUMBER_5_LOWER_ID;
    lowerImageIds[6] = BITMAP_NUMBER_6_LOWER_ID;
    lowerImageIds[7] = BITMAP_NUMBER_7_LOWER_ID;
    lowerImageIds[8] = BITMAP_NUMBER_8_LOWER_ID;
    lowerImageIds[9] = BITMAP_NUMBER_9_LOWER_ID;

    // 초기 이미지 설정
    setDigitImages(0);

    // TextureMapper 초기화
    tensFlipUpper.setVisible(false);
    tensFlipLower.setVisible(false);
    onesFlipUpper.setVisible(false);
    onesFlipLower.setVisible(false);

    // 경계선을 Box로 구현
    // 십의 자리 중간 경계선
    tensMiddleLine.setPosition(0, 89, 90, 2);  // X, Y, Width, Height
    tensMiddleLine.setColor(touchgfx::Color::getColorFromRGB(80, 80, 80));  // 어두운 회색
    tensMiddleLine.setAlpha(255);

    // 일의 자리 중간 경계선
    onesMiddleLine.setPosition(110, 89, 90, 2);  // X, Y, Width, Height
    onesMiddleLine.setColor(touchgfx::Color::getColorFromRGB(80, 80, 80));  // 어두운 회색
    onesMiddleLine.setAlpha(255);

    // flipContainer에 직접 추가 (컨테이너 이름 문제 회피)
    flipContainer.add(tensMiddleLine);
    flipContainer.add(onesMiddleLine);

    // 경계선을 맨 앞으로 가져오기
    flipContainer.remove(tensMiddleLine);
    flipContainer.add(tensMiddleLine);
    flipContainer.remove(onesMiddleLine);
    flipContainer.add(onesMiddleLine);

    // tickHandler를 화면에 추가 (타이머 이벤트를 받기 위해)
    add(tickHandler);
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::incrementCounter()
{
    // 애니메이션 중이면 무시
    if(isAnimatingTens || isAnimatingOnes) return;

    // 다음 숫자 계산
    targetNumber = currentNumber + 1;
    if(targetNumber > 99) targetNumber = 0;

    // 애니메이션 시작
    startFlipAnimation(targetNumber);
}

void Screen1View::startFlipAnimation(uint8_t newNumber)
{
    // 현재/목표 자릿수 계산
    uint8_t currentTens = currentNumber / 10;
    uint8_t currentOnes = currentNumber % 10;
    uint8_t targetTens = newNumber / 10;
    uint8_t targetOnes = newNumber % 10;

    // 일의 자리 애니메이션 설정
    if(currentOnes != targetOnes)
    {
        isAnimatingOnes = true;

        // 현재 숫자의 위쪽이 접힐 준비
        onesFlipUpper.setBitmap(Bitmap(upperImageIds[currentOnes]));
        onesFlipUpper.setVisible(false);
        onesFlipUpper.updateXAngle(0);

        // 아래쪽은 사용 안함
        onesFlipLower.setVisible(false);
    }

    // 십의 자리 애니메이션 설정
    if(currentTens != targetTens)
    {
        isAnimatingTens = true;

        // 현재 숫자의 위쪽이 접힐 준비
        tensFlipUpper.setBitmap(Bitmap(upperImageIds[currentTens]));
        tensFlipUpper.setVisible(false);
        tensFlipUpper.updateXAngle(0);

        // 아래쪽은 사용 안함
        tensFlipLower.setVisible(false);
    }

    // 애니메이션 시작
    animationStep = 0;

    // tickHandler를 타이머에 등록
    Application::getInstance()->registerTimerWidget(&tickHandler);
}

void Screen1View::handleTickEvent()
{
    if(!isAnimatingTens && !isAnimatingOnes) return;

    animationStep++;
    updateFlipAnimation();

    if(animationStep >= ANIMATION_DURATION)
    {
        // 애니메이션 완료
        currentNumber = targetNumber;
        setDigitImages(currentNumber);

        // TextureMapper 숨기기
        tensFlipUpper.setVisible(false);
        tensFlipLower.setVisible(false);
        onesFlipUpper.setVisible(false);
        onesFlipLower.setVisible(false);

        // 각도 리셋
        tensFlipUpper.updateXAngle(0);
        tensFlipLower.updateXAngle(0);
        onesFlipUpper.updateXAngle(0);
        onesFlipLower.updateXAngle(0);

        // 상태 초기화
        isAnimatingTens = false;
        isAnimatingOnes = false;

        // 타이머 해제
        Application::getInstance()->unregisterTimerWidget(&tickHandler);
    }
}

void Screen1View::updateFlipAnimation()
{
    // 진행률 계산 (0.0 ~ 1.0)
    float progress = (float)animationStep / ANIMATION_DURATION;

    // Ease-in-out 함수로 변경 (더 부드러운 시작과 끝)
    float eased;
    if(progress < 0.5f)
    {
        // 천천히 시작 (ease-in)
        eased = 2.0f * progress * progress;
    }
    else
    {
        // 천천히 끝남 (ease-out)
        eased = 1.0f - pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;
    }

    // 회전 각도 (0 ~ 90도)
    float angle = eased * (M_PI / 2.0f);

    // 일의 자리 애니메이션
    if(isAnimatingOnes)
    {
        uint8_t targetOnes = targetNumber % 10;

        // 새 숫자를 미리 설정 (뒤에 보이도록)
        onesUpper.setBitmap(Bitmap(upperImageIds[targetOnes]));
        onesUpper.setVisible(true);
        onesUpper.invalidate();

        if(progress < 0.5f)
        {
            // 전반부: 현재 숫자의 위쪽이 아래로 접힘
            onesFlipUpper.setBitmap(Bitmap(upperImageIds[currentNumber % 10]));
            onesFlipUpper.setVisible(true);

            // X축 회전 (0도에서 90도까지)
            onesFlipUpper.updateXAngle(angle * 2.0f);
            onesFlipUpper.invalidate();
        }
        else
        {
            // 후반부: 위쪽 플립 숨기고 아래쪽 새 숫자 표시
            onesFlipUpper.setVisible(false);

            // 아래쪽에 새 숫자 표시
            onesLower.setBitmap(Bitmap(lowerImageIds[targetOnes]));
            onesLower.setVisible(true);
            onesLower.invalidate();
        }
    }

    // 십의 자리 애니메이션 (동일한 로직)
    if(isAnimatingTens)
    {
        uint8_t targetTens = targetNumber / 10;

        // 새 숫자를 미리 설정 (뒤에 보이도록)
        tensUpper.setBitmap(Bitmap(upperImageIds[targetTens]));
        tensUpper.setVisible(true);
        tensUpper.invalidate();

        if(progress < 0.5f)
        {
            // 전반부: 현재 숫자의 위쪽이 아래로 접힘
            tensFlipUpper.setBitmap(Bitmap(upperImageIds[currentNumber / 10]));
            tensFlipUpper.setVisible(true);

            // X축 회전 (0도에서 90도까지)
            tensFlipUpper.updateXAngle(angle * 2.0f);
            tensFlipUpper.invalidate();
        }
        else
        {
            // 후반부: 위쪽 플립 숨기고 아래쪽 새 숫자 표시
            tensFlipUpper.setVisible(false);

            // 아래쪽에 새 숫자 표시
            tensLower.setBitmap(Bitmap(lowerImageIds[targetTens]));
            tensLower.setVisible(true);
            tensLower.invalidate();
        }
    }
}

void Screen1View::setDigitImages(uint8_t number)
{
    uint8_t tens = number / 10;
    uint8_t ones = number % 10;

    // 십의 자리
    tensUpper.setBitmap(Bitmap(upperImageIds[tens]));
    tensLower.setBitmap(Bitmap(lowerImageIds[tens]));
    tensUpper.setVisible(true);
    tensLower.setVisible(true);

    // 일의 자리
    onesUpper.setBitmap(Bitmap(upperImageIds[ones]));
    onesLower.setBitmap(Bitmap(lowerImageIds[ones]));
    onesUpper.setVisible(true);
    onesLower.setVisible(true);

    // 다시 그리기
    tensUpper.invalidate();
    tensLower.invalidate();
    onesUpper.invalidate();
    onesLower.invalidate();
}
