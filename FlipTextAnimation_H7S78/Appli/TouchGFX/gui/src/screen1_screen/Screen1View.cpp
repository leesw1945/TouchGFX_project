#include <gui/screen1_screen/Screen1View.hpp>
#include <BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <math.h>

// ============ 이미지 ID 배열 정의 ============
const uint16_t numberImages[10] = {
    BITMAP_NUMBER_0_FULL_ID, BITMAP_NUMBER_1_FULL_ID, BITMAP_NUMBER_2_FULL_ID, BITMAP_NUMBER_3_FULL_ID,
    BITMAP_NUMBER_4_FULL_ID, BITMAP_NUMBER_5_FULL_ID, BITMAP_NUMBER_6_FULL_ID, BITMAP_NUMBER_7_FULL_ID,
    BITMAP_NUMBER_8_FULL_ID, BITMAP_NUMBER_9_FULL_ID
};

const uint16_t numberTopImages[10] = {
    BITMAP_NUMBER_0_UPPER_ID, BITMAP_NUMBER_1_UPPER_ID, BITMAP_NUMBER_2_UPPER_ID, BITMAP_NUMBER_3_UPPER_ID,
    BITMAP_NUMBER_4_UPPER_ID, BITMAP_NUMBER_5_UPPER_ID, BITMAP_NUMBER_6_UPPER_ID, BITMAP_NUMBER_7_UPPER_ID,
    BITMAP_NUMBER_8_UPPER_ID, BITMAP_NUMBER_9_UPPER_ID
};

const uint16_t numberBottomImages[10] = {
    BITMAP_NUMBER_0_LOWER_ID, BITMAP_NUMBER_1_LOWER_ID, BITMAP_NUMBER_2_LOWER_ID, BITMAP_NUMBER_3_LOWER_ID,
    BITMAP_NUMBER_4_LOWER_ID, BITMAP_NUMBER_5_LOWER_ID, BITMAP_NUMBER_6_LOWER_ID, BITMAP_NUMBER_7_LOWER_ID,
    BITMAP_NUMBER_8_LOWER_ID, BITMAP_NUMBER_9_LOWER_ID
};

// ============ 생성자 ============
Screen1View::Screen1View()
    : currentNumber(0)
    , tensDigit(0)
    , onesDigit(0)
    , isAnimating(false)
    , animationStep(0)
    , maxAnimationSteps(30)  // 30프레임으로 부드러운 애니메이션
    , animatingTens(false)
    , animatingOnes(false)
    , nextTensDigit(0)
    , nextOnesDigit(0)
    , use3DEffect(false)     // 기본적으로 2D 모드 (안전함)
    , imageSwapped(false)
    , imgCurrentTens(nullptr)
    , imgTopTens(nullptr)
    , imgBottomTens(nullptr)
    , imgNextTens(nullptr)
    , imgCurrentOnes(nullptr)
    , imgTopOnes(nullptr)
    , imgBottomOnes(nullptr)
    , imgNextOnes(nullptr)
{
}

// ============ 화면 설정 ============
void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    // Container 내부 위젯들 찾기
    findContainerChildren();

    // 초기 숫자 표시
    updateDigitDisplay();

    // 3D 플리퍼 설정 (필요시)
    if (use3DEffect) {
        setup3DFlippers();
    }
}

// ============ 화면 정리 ============
void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

// ============ Container 내부 위젯들 찾기 ============
void Screen1View::findContainerChildren()
{
    // TouchGFX 4.25.0에서는 getChild가 없으므로 직접 찾아야 함
    // 이 함수는 TouchGFX Designer에서 위젯을 추가한 순서대로 찾음

    // 십의 자리 Container 내부 위젯들
    if (containerTens.getNumberOfChildren() >= 4) {
        imgCurrentTens = (Image*)&containerTens[0];  // 첫 번째 추가된 Image
        imgTopTens = (Image*)&containerTens[1];      // 두 번째 추가된 Image
        imgBottomTens = (Image*)&containerTens[2];   // 세 번째 추가된 Image
        imgNextTens = (Image*)&containerTens[3];     // 네 번째 추가된 Image
    }

    // 일의 자리 Container 내부 위젯들
    if (containerOnes.getNumberOfChildren() >= 4) {
        imgCurrentOnes = (Image*)&containerOnes[0];  // 첫 번째 추가된 Image
        imgTopOnes = (Image*)&containerOnes[1];      // 두 번째 추가된 Image
        imgBottomOnes = (Image*)&containerOnes[2];   // 세 번째 추가된 Image
        imgNextOnes = (Image*)&containerOnes[3];     // 네 번째 추가된 Image
    }
}

// ============ 3D 플리퍼 설정 ============
void Screen1View::setup3DFlippers()
{
    // 십의 자리 3D 설정
    flipperTens.setXY(300, 200);
    flipperTens.setWidth(120);
    flipperTens.setHeight(180);
    flipperTens.setBitmap(Bitmap(numberImages[tensDigit]));
    flipperTens.setCameraDistance(600.0f);
    flipperTens.setOrigo(60.0f, 90.0f, 300.0f);
    flipperTens.setCamera(60.0f, 90.0f);
    flipperTens.updateAngles(0.0f, 0.0f, 0.0f);
    flipperTens.setRenderingAlgorithm(TextureMapper::BILINEAR_INTERPOLATION);

    // 일의 자리 3D 설정
    flipperOnes.setXY(450, 200);
    flipperOnes.setWidth(120);
    flipperOnes.setHeight(180);
    flipperOnes.setBitmap(Bitmap(numberImages[onesDigit]));
    flipperOnes.setCameraDistance(600.0f);
    flipperOnes.setOrigo(60.0f, 90.0f, 300.0f);
    flipperOnes.setCamera(60.0f, 90.0f);
    flipperOnes.updateAngles(0.0f, 0.0f, 0.0f);
    flipperOnes.setRenderingAlgorithm(TextureMapper::BILINEAR_INTERPOLATION);

    // 화면에 추가
    containerTens.setVisible(false);
    containerOnes.setVisible(false);

    add(flipperTens);
    add(flipperOnes);

    flipperTens.invalidate();
    flipperOnes.invalidate();
}

// ============ 2D/3D 모드 전환 ============
void Screen1View::toggle3DMode()
{
    use3DEffect = !use3DEffect;

    if (use3DEffect) {
        containerTens.setVisible(false);
        containerOnes.setVisible(false);
        setup3DFlippers();
    } else {
        remove(flipperTens);
        remove(flipperOnes);
        containerTens.setVisible(true);
        containerOnes.setVisible(true);
        updateDigitDisplay();
    }

    invalidate();
}

// ============ 숫자 증가 ============
void Screen1View::incrementNumber()
{
    updateNumber(1);
}

// ============ 숫자 감소 ============
void Screen1View::decrementNumber()
{
    updateNumber(-1);
}

// ============ 숫자 업데이트 통합 함수 ============
void Screen1View::updateNumber(int8_t delta)
{
    if (isAnimating) return;

    // 숫자 업데이트 (0~99 순환)
    int16_t newNumber = currentNumber + delta;
    if (newNumber < 0) newNumber = 99;
    if (newNumber > 99) newNumber = 0;
    currentNumber = (uint8_t)newNumber;

    // 자릿수 분리
    uint8_t newTens = currentNumber / 10;
    uint8_t newOnes = currentNumber % 10;

    // 변경된 자릿수 확인
    bool tensChanged = (newTens != tensDigit);
    bool onesChanged = (newOnes != onesDigit);

    if (tensChanged || onesChanged) {
        nextTensDigit = newTens;
        nextOnesDigit = newOnes;
        startFlipAnimation(tensChanged, onesChanged);
    }
}

// ============ 플립 애니메이션 시작 ============
void Screen1View::startFlipAnimation(bool tens, bool ones)
{
    isAnimating = true;
    animationStep = 0;
    animatingTens = tens;
    animatingOnes = ones;
    imageSwapped = false;

    if (!use3DEffect) {
        // 2D 모드: 다음 숫자 이미지 설정
        if (tens && imgNextTens) {
            setDigitImages(true, nextTensDigit, true);
            imgNextTens->setVisible(false);
        }

        if (ones && imgNextOnes) {
            setDigitImages(false, nextOnesDigit, true);
            imgNextOnes->setVisible(false);
        }
    }
}

// ============ 애니메이션 업데이트 ============
void Screen1View::updateFlipAnimation()
{
    if (!isAnimating) return;

    animationStep++;

    // 애니메이션 진행률 계산
    float progress = (float)animationStep / maxAnimationSteps;
    progress = applyCubicEaseOut(progress);

    // 각도 계산
    float angle = progress * 180.0f;

    if (use3DEffect) {
        update3DFlipAnimation(angle);
    } else {
        update2DFlipAnimation(angle);
    }

    // 애니메이션 완료 체크
    if (animationStep >= maxAnimationSteps) {
        finishFlipAnimation();
    }

    invalidate();
}

// ============ 3D 플립 애니메이션 업데이트 ============
void Screen1View::update3DFlipAnimation(float angle)
{
    float angleRad = angle * 3.14159f / 180.0f;

    if (animatingTens) {
        flipperTens.updateXAngle(angleRad);
        flipperTens.invalidate();
    }

    if (animatingOnes) {
        flipperOnes.updateXAngle(angleRad);
        flipperOnes.invalidate();
    }

    // 90도 지점에서 이미지 교체
    if (angle > 90.0f && !imageSwapped) {
        if (animatingTens) {
            flipperTens.setBitmap(Bitmap(numberImages[nextTensDigit]));
        }
        if (animatingOnes) {
            flipperOnes.setBitmap(Bitmap(numberImages[nextOnesDigit]));
        }
        imageSwapped = true;
    }
}

// ============ 2D 플립 애니메이션 업데이트 ============
void Screen1View::update2DFlipAnimation(float angle)
{
    if (angle <= 90.0f) {
        // 첫 번째 반: 현재 숫자가 위로 접힘
        if (animatingTens && imgTopTens && imgBottomTens && imgCurrentTens) {
            imgTopTens->setVisible(true);
            imgBottomTens->setVisible(true);
            imgCurrentTens->setVisible(false);
            if (imgNextTens) imgNextTens->setVisible(false);

            // 상단 이미지 높이 조정으로 3D 효과 시뮬레이션
            float scale = cos(angle * 3.14159f / 180.0f);
            int newHeight = (int)(90 * scale);
            if (newHeight < 1) newHeight = 1;

            imgTopTens->setHeight(newHeight);
        }

        if (animatingOnes && imgTopOnes && imgBottomOnes && imgCurrentOnes) {
            imgTopOnes->setVisible(true);
            imgBottomOnes->setVisible(true);
            imgCurrentOnes->setVisible(false);
            if (imgNextOnes) imgNextOnes->setVisible(false);

            float scale = cos(angle * 3.14159f / 180.0f);
            int newHeight = (int)(90 * scale);
            if (newHeight < 1) newHeight = 1;

            imgTopOnes->setHeight(newHeight);
        }
    } else {
        // 두 번째 반: 새 숫자가 아래서 펼쳐짐
        if (animatingTens) {
            setDigitImages(true, nextTensDigit);
            if (imgTopTens && imgBottomTens && imgCurrentTens) {
                imgTopTens->setVisible(true);
                imgBottomTens->setVisible(true);
                imgCurrentTens->setVisible(false);

                float scale = cos((180.0f - angle) * 3.14159f / 180.0f);
                int newHeight = (int)(90 * scale);
                if (newHeight < 1) newHeight = 1;

                imgTopTens->setHeight(newHeight);
                imgTopTens->setY(90 - newHeight);
            }
        }

        if (animatingOnes) {
            setDigitImages(false, nextOnesDigit);
            if (imgTopOnes && imgBottomOnes && imgCurrentOnes) {
                imgTopOnes->setVisible(true);
                imgBottomOnes->setVisible(true);
                imgCurrentOnes->setVisible(false);

                float scale = cos((180.0f - angle) * 3.14159f / 180.0f);
                int newHeight = (int)(90 * scale);
                if (newHeight < 1) newHeight = 1;

                imgTopOnes->setHeight(newHeight);
                imgTopOnes->setY(90 - newHeight);
            }
        }
    }
}

// ============ 플립 애니메이션 완료 ============
void Screen1View::finishFlipAnimation()
{
    isAnimating = false;
    animationStep = 0;
    imageSwapped = false;

    tensDigit = nextTensDigit;
    onesDigit = nextOnesDigit;

    if (use3DEffect) {
        if (animatingTens) {
            flipperTens.updateAngles(0.0f, 0.0f, 0.0f);
            flipperTens.setBitmap(Bitmap(numberImages[tensDigit]));
            flipperTens.invalidate();
        }

        if (animatingOnes) {
            flipperOnes.updateAngles(0.0f, 0.0f, 0.0f);
            flipperOnes.setBitmap(Bitmap(numberImages[onesDigit]));
            flipperOnes.invalidate();
        }
    } else {
        updateDigitDisplay();
    }

    animatingTens = false;
    animatingOnes = false;
}

// ============ 숫자 표시 업데이트 ============
void Screen1View::updateDigitDisplay()
{
    if (use3DEffect) return;

    // 십의 자리 표시
    setDigitImages(true, tensDigit);
    if (imgCurrentTens) imgCurrentTens->setVisible(true);
    if (imgTopTens) imgTopTens->setVisible(false);
    if (imgBottomTens) imgBottomTens->setVisible(false);
    if (imgNextTens) imgNextTens->setVisible(false);

    // 일의 자리 표시
    setDigitImages(false, onesDigit);
    if (imgCurrentOnes) imgCurrentOnes->setVisible(true);
    if (imgTopOnes) imgTopOnes->setVisible(false);
    if (imgBottomOnes) imgBottomOnes->setVisible(false);
    if (imgNextOnes) imgNextOnes->setVisible(false);

    invalidate();
}

// ============ 개별 숫자 이미지 설정 ============
void Screen1View::setDigitImages(bool isTens, uint8_t digit, bool isNext)
{
    if (use3DEffect) return;

    if (isTens) {
        if (isNext && imgNextTens) {
            imgNextTens->setBitmap(Bitmap(numberImages[digit]));
        } else {
            if (imgCurrentTens) imgCurrentTens->setBitmap(Bitmap(numberImages[digit]));
            if (imgTopTens) imgTopTens->setBitmap(Bitmap(numberTopImages[digit]));
            if (imgBottomTens) imgBottomTens->setBitmap(Bitmap(numberBottomImages[digit]));

            // 크기 리셋
            if (imgTopTens) {
                imgTopTens->setHeight(90);
                imgTopTens->setY(0);
            }
        }
    } else {
        if (isNext && imgNextOnes) {
            imgNextOnes->setBitmap(Bitmap(numberImages[digit]));
        } else {
            if (imgCurrentOnes) imgCurrentOnes->setBitmap(Bitmap(numberImages[digit]));
            if (imgTopOnes) imgTopOnes->setBitmap(Bitmap(numberTopImages[digit]));
            if (imgBottomOnes) imgBottomOnes->setBitmap(Bitmap(numberBottomImages[digit]));

            // 크기 리셋
            if (imgTopOnes) {
                imgTopOnes->setHeight(90);
                imgTopOnes->setY(0);
            }
        }
    }
}

// ============ Cubic Ease-Out 함수 ============
float Screen1View::applyCubicEaseOut(float progress)
{
    return 1.0f - (1.0f - progress) * (1.0f - progress) * (1.0f - progress);
}

// ============ 타이머 콜백 ============
void Screen1View::handleTickEvent()
{
    if (isAnimating) {
        updateFlipAnimation();
    }
}
