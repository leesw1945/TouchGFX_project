#include <gui/flipscreen_screen/FlipScreenView.hpp>
#include <touchgfx/Color.hpp>

FlipScreenView::FlipScreenView() : tickCounter(0), totalSeconds(0), animationTimer(0)
{
}

void FlipScreenView::setupScreen()
{
    FlipScreenViewBase::setupScreen();
    
    // 카드 초기화 (위치 좌표 정확히 설정)
    initializeFlipCard(minuteTens, &minuteTensMain, &minuteTensUpper, &minuteTensLower, 280, 190, 5);
    initializeFlipCard(minuteUnits, &minuteUnitsMain, &minuteUnitsUpper, &minuteUnitsLower, 350, 190, 9);
    initializeFlipCard(secondTens, &secondTensMain, &secondTensUpper, &secondTensLower, 450, 190, 5);
    initializeFlipCard(secondUnits, &secondUnitsMain, &secondUnitsUpper, &secondUnitsLower, 520, 190, 9);
    
    // 마스크 박스들을 화면에 추가
    add(minuteTens.upperMask);
    add(minuteTens.lowerMask);
    add(minuteUnits.upperMask);
    add(minuteUnits.lowerMask);
    add(secondTens.upperMask);
    add(secondTens.lowerMask);
    add(secondUnits.upperMask);
    add(secondUnits.lowerMask);
    
    updateTime();
}

void FlipScreenView::tearDownScreen()
{
    FlipScreenViewBase::tearDownScreen();
}

void FlipScreenView::initializeFlipCard(NaturalFlipCard& card, 
                                       touchgfx::Image* main,
                                       touchgfx::Image* upper,
                                       touchgfx::Image* lower,
                                       int16_t x, int16_t y,
                                       int maxVal)
{
    card.mainImage = main;
    card.upperImage = upper;
    card.lowerImage = lower;
    card.cardX = x;
    card.cardY = y;
    card.maxValue = maxVal;
    
    // 메인 이미지 설정
    updateCardImage(card);
    card.mainImage->setVisible(true);
    card.upperImage->setVisible(false);
    card.lowerImage->setVisible(false);
    
    // 상단 마스크 박스 설정 (수정된 색상 설정)
    card.upperMask.setPosition(x, y + 25, 60, 25);
    card.upperMask.setColor(0x2020); // RGB565 형태로 어두운 회색 (0x202020과 비슷)
    card.upperMask.setVisible(false);
    
    // 하단 마스크 박스 설정 (수정된 색상 설정)
    card.lowerMask.setPosition(x, y + 50, 60, 25);
    card.lowerMask.setColor(0x2020); // RGB565 형태로 어두운 회색
    card.lowerMask.setVisible(false);
}

void FlipScreenView::handleTickEvent()
{
    tickCounter++;
    
    if(animationTimer > 0) {
        animationTimer--;
        int currentFrame = 30 - animationTimer; // 0~30
        
        if(currentFrame == 15) {
            // Phase 1 완료 → Phase 2 시작
            if(minuteTens.animationPhase == 1) {
                minuteTens.animationPhase = 2;
                minuteTens.upperImage->setVisible(false);
                minuteTens.upperMask.setVisible(false);
                minuteTens.lowerImage->setBitmap(touchgfx::Bitmap(getLowerImageId(minuteTens.nextValue)));
                minuteTens.lowerImage->setVisible(true);
                minuteTens.lowerMask.setVisible(true);
                // 하단 마스크를 아래쪽에서 시작
                minuteTens.lowerMask.setPosition(minuteTens.cardX, minuteTens.cardY + 75, 60, 25);
            }
            if(minuteUnits.animationPhase == 1) {
                minuteUnits.animationPhase = 2;
                minuteUnits.upperImage->setVisible(false);
                minuteUnits.upperMask.setVisible(false);
                minuteUnits.lowerImage->setBitmap(touchgfx::Bitmap(getLowerImageId(minuteUnits.nextValue)));
                minuteUnits.lowerImage->setVisible(true);
                minuteUnits.lowerMask.setVisible(true);
                minuteUnits.lowerMask.setPosition(minuteUnits.cardX, minuteUnits.cardY + 75, 60, 25);
            }
            if(secondTens.animationPhase == 1) {
                secondTens.animationPhase = 2;
                secondTens.upperImage->setVisible(false);
                secondTens.upperMask.setVisible(false);
                secondTens.lowerImage->setBitmap(touchgfx::Bitmap(getLowerImageId(secondTens.nextValue)));
                secondTens.lowerImage->setVisible(true);
                secondTens.lowerMask.setVisible(true);
                secondTens.lowerMask.setPosition(secondTens.cardX, secondTens.cardY + 75, 60, 25);
            }
            if(secondUnits.animationPhase == 1) {
                secondUnits.animationPhase = 2;
                secondUnits.upperImage->setVisible(false);
                secondUnits.upperMask.setVisible(false);
                secondUnits.lowerImage->setBitmap(touchgfx::Bitmap(getLowerImageId(secondUnits.nextValue)));
                secondUnits.lowerImage->setVisible(true);
                secondUnits.lowerMask.setVisible(true);
                secondUnits.lowerMask.setPosition(secondUnits.cardX, secondUnits.cardY + 75, 60, 25);
            }
        }
        
        // 자연스러운 클리핑 애니메이션
        if(currentFrame <= 15) {
            // Phase 1: 상단 "접기" - 마스크가 아래로 내려와서 이미지를 가림
            int maskY = (currentFrame * 25) / 15; // 0 → 25
            int maskHeight = 25 - maskY;
            if(maskHeight < 0) maskHeight = 0;
            
            if(minuteTens.animationPhase == 1) {
                minuteTens.upperMask.setPosition(minuteTens.cardX, minuteTens.cardY + 25 + maskY, 60, maskHeight);
                minuteTens.upperMask.invalidate();
            }
            if(minuteUnits.animationPhase == 1) {
                minuteUnits.upperMask.setPosition(minuteUnits.cardX, minuteUnits.cardY + 25 + maskY, 60, maskHeight);
                minuteUnits.upperMask.invalidate();
            }
            if(secondTens.animationPhase == 1) {
                secondTens.upperMask.setPosition(secondTens.cardX, secondTens.cardY + 25 + maskY, 60, maskHeight);
                secondTens.upperMask.invalidate();
            }
            if(secondUnits.animationPhase == 1) {
                secondUnits.upperMask.setPosition(secondUnits.cardX, secondUnits.cardY + 25 + maskY, 60, maskHeight);
                secondUnits.upperMask.invalidate();
            }
        } else {
            // Phase 2: 하단 "펼치기" - 마스크가 위로 올라가면서 이미지를 드러냄
            int phase2Frame = currentFrame - 15;
            int maskY = 25 - (phase2Frame * 25) / 15; // 25 → 0
            int maskHeight = 25 - maskY;
            if(maskY < 0) maskY = 0;
            if(maskHeight < 0) maskHeight = 0;
            
            if(minuteTens.animationPhase == 2) {
                minuteTens.lowerMask.setPosition(minuteTens.cardX, minuteTens.cardY + 50 + maskY, 60, maskHeight);
                minuteTens.lowerMask.invalidate();
            }
            if(minuteUnits.animationPhase == 2) {
                minuteUnits.lowerMask.setPosition(minuteUnits.cardX, minuteUnits.cardY + 50 + maskY, 60, maskHeight);
                minuteUnits.lowerMask.invalidate();
            }
            if(secondTens.animationPhase == 2) {
                secondTens.lowerMask.setPosition(secondTens.cardX, secondTens.cardY + 50 + maskY, 60, maskHeight);
                secondTens.lowerMask.invalidate();
            }
            if(secondUnits.animationPhase == 2) {
                secondUnits.lowerMask.setPosition(secondUnits.cardX, secondUnits.cardY + 50 + maskY, 60, maskHeight);
                secondUnits.lowerMask.invalidate();
            }
        }
        
        if(animationTimer == 0) {
            if(minuteTens.isAnimating) onAnimationComplete(minuteTens);
            if(minuteUnits.isAnimating) onAnimationComplete(minuteUnits);
            if(secondTens.isAnimating) onAnimationComplete(secondTens);
            if(secondUnits.isAnimating) onAnimationComplete(secondUnits);
        }
    }
    
    // 1초마다 시간 업데이트
    if(tickCounter % 60 == 0) {
        totalSeconds++;
        updateTime();
    }
}

void FlipScreenView::updateTime()
{
    int minutes = (totalSeconds / 60) % 60;
    int seconds = totalSeconds % 60;
    
    int newMinuteTens = minutes / 10;
    int newMinuteUnits = minutes % 10;
    int newSecondTens = seconds / 10;
    int newSecondUnits = seconds % 10;
    
    bool needAnimation = false;
    
    if(newMinuteTens != minuteTens.currentValue && !minuteTens.isAnimating) {
        startFlipAnimation(minuteTens, newMinuteTens);
        needAnimation = true;
    }
    if(newMinuteUnits != minuteUnits.currentValue && !minuteUnits.isAnimating) {
        startFlipAnimation(minuteUnits, newMinuteUnits);
        needAnimation = true;
    }
    if(newSecondTens != secondTens.currentValue && !secondTens.isAnimating) {
        startFlipAnimation(secondTens, newSecondTens);
        needAnimation = true;
    }
    if(newSecondUnits != secondUnits.currentValue && !secondUnits.isAnimating) {
        startFlipAnimation(secondUnits, newSecondUnits);
        needAnimation = true;
    }
    
    if(needAnimation && animationTimer == 0) {
        animationTimer = 30;
    }
}

void FlipScreenView::startFlipAnimation(NaturalFlipCard& card, int newValue)
{
    if(card.isAnimating || newValue == card.currentValue) return;
    
    card.isAnimating = true;
    card.nextValue = newValue;
    card.animationPhase = 1;
    
    // 메인 이미지 숨기기
    card.mainImage->setVisible(false);
    
    // 상단 이미지 표시
    card.upperImage->setBitmap(touchgfx::Bitmap(getUpperImageId(card.currentValue)));
    card.upperImage->setVisible(true);
    
    // 상단 마스크 초기 설정 (마스크 안 보임)
    card.upperMask.setPosition(card.cardX, card.cardY + 25, 60, 25);
    card.upperMask.setVisible(true);
}

void FlipScreenView::onAnimationComplete(NaturalFlipCard& card)
{
    card.isAnimating = false;
    card.animationPhase = 0;
    card.currentValue = card.nextValue;
    
    // 모든 애니메이션 요소들 숨기기
    card.upperImage->setVisible(false);
    card.lowerImage->setVisible(false);
    card.upperMask.setVisible(false);
    card.lowerMask.setVisible(false);
    
    // 메인 이미지 복원
    updateCardImage(card);
    card.mainImage->setVisible(true);
}

void FlipScreenView::updateCardImage(NaturalFlipCard& card)
{
    card.mainImage->setBitmap(touchgfx::Bitmap(getFullImageId(card.currentValue)));
    card.mainImage->invalidate();
}

// 이미지 ID 함수들 (동일)
touchgfx::BitmapId FlipScreenView::getFullImageId(int number)
{
    touchgfx::BitmapId ids[10] = {
        BITMAP_NUMBER_0_FULL_ID, BITMAP_NUMBER_1_FULL_ID, BITMAP_NUMBER_2_FULL_ID,
        BITMAP_NUMBER_3_FULL_ID, BITMAP_NUMBER_4_FULL_ID, BITMAP_NUMBER_5_FULL_ID,
        BITMAP_NUMBER_6_FULL_ID, BITMAP_NUMBER_7_FULL_ID, BITMAP_NUMBER_8_FULL_ID,
        BITMAP_NUMBER_9_FULL_ID
    };
    return (number >= 0 && number <= 9) ? ids[number] : ids[0];
}

touchgfx::BitmapId FlipScreenView::getUpperImageId(int number)
{
    touchgfx::BitmapId ids[10] = {
        BITMAP_NUMBER_0_UPPER_ID, BITMAP_NUMBER_1_UPPER_ID, BITMAP_NUMBER_2_UPPER_ID,
        BITMAP_NUMBER_3_UPPER_ID, BITMAP_NUMBER_4_UPPER_ID, BITMAP_NUMBER_5_UPPER_ID,
        BITMAP_NUMBER_6_UPPER_ID, BITMAP_NUMBER_7_UPPER_ID, BITMAP_NUMBER_8_UPPER_ID,
        BITMAP_NUMBER_9_UPPER_ID
    };
    return (number >= 0 && number <= 9) ? ids[number] : ids[0];
}

touchgfx::BitmapId FlipScreenView::getLowerImageId(int number)
{
    touchgfx::BitmapId ids[10] = {
        BITMAP_NUMBER_0_LOWER_ID, BITMAP_NUMBER_1_LOWER_ID, BITMAP_NUMBER_2_LOWER_ID,
        BITMAP_NUMBER_3_LOWER_ID, BITMAP_NUMBER_4_LOWER_ID, BITMAP_NUMBER_5_LOWER_ID,
        BITMAP_NUMBER_6_LOWER_ID, BITMAP_NUMBER_7_LOWER_ID, BITMAP_NUMBER_8_LOWER_ID,
        BITMAP_NUMBER_9_LOWER_ID
    };
    return (number >= 0 && number <= 9) ? ids[number] : ids[0];
}