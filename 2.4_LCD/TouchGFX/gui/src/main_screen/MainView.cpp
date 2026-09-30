#include <gui/main_screen/MainView.hpp>
#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>
#include <touchgfx/Matrix3x3.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

using namespace touchgfx;

/* 캔버스 위젯(Circle) 스캔라인 작업 버퍼 - 107x130 카드 크기 원호 2개 */
static uint8_t canvasBuffer[16384];

/* 그라디언트 페인터용 1x1024 ARGB8888 텍스처 (RAM, 화면 진입 시 생성) */
static uint32_t texBlue[1024];

/* 색상 (디자인 계측값) */
static const colortype COLOR_READY_GREEN = Color::getColorFromRGB(0x36, 0xD2, 0x60);
static const colortype COLOR_EMERGENCY   = Color::getColorFromRGB(0xEB, 0x4B, 0x4B);
static const colortype COLOR_VALUE_NAVY  = Color::getColorFromRGB(0x25, 0x3D, 0x4A);
static const colortype COLOR_VALUE_BLUE  = Color::getColorFromRGB(0x00, 0x77, 0xC0);
static const colortype COLOR_TRACK       = Color::getColorFromRGB(0xE6, 0xE6, 0xE6);
static const uint32_t  GRAD_BLUE_FROM = 0x0077C0;   /* 원호 아래쪽(시작점) */
static const uint32_t  GRAD_BLUE_TO   = 0x62B8ED;   /* 원호 위쪽 */

/* 게이지 애니메이션 시간(프레임 수). 새 값이 오면 직전 갱신과의 간격만큼의 시간 동안 등속 이동한다.
 * 메인이 300ms마다 보내면 약 18~23프레임 → 다음 값이 도착할 때 딱 목표에 닿아 움직임이 이어진다 */
static const uint16_t ANIM_MIN_FRAMES   = 4;    /* 아주 빠른 갱신에도 최소 이만큼은 보간 */
static const uint16_t ANIM_MAX_FRAMES   = 45;   /* 오래 멈췄다 바뀐 값은 최대 0.6초 안에 도달 */
static const uint16_t ANIM_FIRST_FRAMES = 18;   /* 대시(값 없음)에서 첫 값이 올 때 차오르는 시간 */

/* 통신 두절 표시 */
static const char* DASH_LINEAR = "---";
static const char* DASH_ANGLE  = "--";

MainView::MainView()
    : state(STATE_READY)
{
    for (int i = 0; i < 4; i++)
    {
        cardActive[i] = false;
    }
    frameCount = 0;
    for (int g = 0; g < GAUGE_COUNT; g++)
    {
        gaugeCur[g] = (float)ARC_START;
        gaugeFrom[g] = (float)ARC_START;
        gaugeTarget[g] = (float)ARC_START;
        gaugeAnimLen[g] = 0;
        gaugeAnimPos[g] = 0;
        gaugeLastSetFrame[g] = 0;
        gaugeHasValue[g] = false;
    }
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    CanvasWidgetRenderer::setupBuffer(canvasBuffer, sizeof(canvasBuffer));

    trackPainter.setColor(COLOR_TRACK);

    buildGradient(texBlue, GRAD_BLUE_FROM, GRAD_BLUE_TO);
    /* 세로 그라디언트: 원호 아래끝(y=111) 색 -> 원호 위끝(y=40) 색. 좌표는 위젯(카드) 기준 */
    bluePainter.setGradientTexture(texBlue, true);
    bluePainter.setGradientEndPoints(53.5f, 111.0f, 53.5f, 40.0f, (float)CARD_W, 130.0f, Matrix3x3());
    bluePainter.setWidgetWidth(CARD_W);

    initGauge(trackArm, progArm, CARD_LEFT_X);
    initGauge(trackDet, progDet, CARD_RIGHT_X);

    /* 초기 화면: 데이터가 오기 전까지 READY + 모든 값 대시 + 강조 없음.
     * 실제 값은 Presenter가 Model 통지를 받아 채운다 */
    setState(STATE_READY);
    setActive(CARD_SID, false);
    setActive(CARD_ARM_UD, false);
    setActive(CARD_ARM_DEG, false);
    setActive(CARD_DET_DEG, false);
    setSidDash();
    setArmUpDownDash();
    setArmDegDash();
    setDetectorDegDash();
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

/* 매 프레임: 게이지 원호를 목표각까지 부드럽게 이동 */
void MainView::handleTickEvent()
{
    frameCount++;
    animateGauge(GAUGE_ARM, progArm);
    animateGauge(GAUGE_DET, progDet);
}

/* ---------------------------------------------------------------- 상태바 */
void MainView::setState(State s)
{
    state = s;
    topBar.setColor(s == STATE_READY ? COLOR_READY_GREEN : COLOR_EMERGENCY);
    topBar.invalidate();

    stateText.setTypedText(TypedText(s == STATE_READY ? T_MAIN_READY : T_MAIN_EMERGENCY));
    /* 텍스트는 박스 오른끝에 정렬되므로, 점은 텍스트 왼끝에서 DOT_GAP 왼쪽으로 따라간다 */
    const int16_t textLeft = stateText.getX() + stateText.getWidth() - stateText.getTextWidth();
    stateDot.setX(textLeft - DOT_GAP);
    stateText.invalidate();
    stateDot.invalidate();
}

/* ---------------------------------------------------------------- 직선 이동 값 */
void MainView::setSidCm(uint16_t cm)
{
    setLinearValue(valSid, valSidBuffer, VALSID_SIZE, markInchSid, unitSid, CARD_LEFT_X, cm, false);
}

void MainView::setSidInch(uint16_t inch10)
{
    setLinearValue(valSid, valSidBuffer, VALSID_SIZE, markInchSid, unitSid, CARD_LEFT_X, inch10, true);
}

void MainView::setArmUpDownCm(uint16_t cm)
{
    setLinearValue(valArmUd, valArmUdBuffer, VALARMUD_SIZE, markInchArmUd, unitArmUd, CARD_RIGHT_X, cm, false);
}

void MainView::setArmUpDownInch(uint16_t inch10)
{
    setLinearValue(valArmUd, valArmUdBuffer, VALARMUD_SIZE, markInchArmUd, unitArmUd, CARD_RIGHT_X, inch10, true);
}

void MainView::setSidDash()
{
    setLinearDash(valSid, valSidBuffer, VALSID_SIZE, markInchSid, CARD_LEFT_X);
}

void MainView::setArmUpDownDash()
{
    setLinearDash(valArmUd, valArmUdBuffer, VALARMUD_SIZE, markInchArmUd, CARD_RIGHT_X);
}

void MainView::setLinearValue(TextAreaWithOneWildcard& val, Unicode::UnicodeChar* buf, uint16_t bufSize,
                              TextArea& mark, TextArea& unit, int16_t cardX, uint16_t value, bool inch)
{
    val.invalidate();
    mark.invalidate();

    if (inch)
    {
        Unicode::snprintf(buf, bufSize, "%d.%d", value / 10, value % 10);   /* 2자리 + 소수 1자리 */
    }
    else
    {
        Unicode::snprintf(buf, bufSize, "%d", value);                       /* 정수 최대 3자리 */
    }

    /* 숫자(+인치 기호)를 한 덩어리로 카드 중앙에 놓는다.
     * TextArea는 자기 박스 안에서 가운데 정렬이므로, inch일 때는 박스를 기호 폭의 절반만큼 왼쪽으로 민다 */
    const int16_t textW = val.getTextWidth();
    const int16_t shift = inch ? (int16_t)((mark.getWidth() + MARK_INCH_DX) / 2) : 0;
    val.setX(cardX - shift);
    const int16_t textLeft = val.getX() + (val.getWidth() - textW) / 2;
    mark.setXY(textLeft + textW + MARK_INCH_DX, val.getY() + MARK_INCH_DY);
    mark.setVisible(inch);

    unit.setTypedText(TypedText(inch ? T_MAIN_INCH : T_MAIN_CM));

    val.invalidate();
    mark.invalidate();
    unit.invalidate();
}

void MainView::setLinearDash(TextAreaWithOneWildcard& val, Unicode::UnicodeChar* buf, uint16_t bufSize,
                             TextArea& mark, int16_t cardX)
{
    val.invalidate();
    mark.invalidate();

    Unicode::strncpy(buf, DASH_LINEAR, bufSize);
    val.setX(cardX);          /* 대시는 기호 없이 카드 중앙 */
    mark.setVisible(false);

    val.invalidate();
    mark.invalidate();
}

/* ---------------------------------------------------------------- 회전 각도 값 + 게이지 */
void MainView::setArmDeg(int16_t deg)
{
    setAngleValue(valArmDeg, valArmDegBuffer, VALARMDEG_SIZE, markDegArm, GAUGE_ARM, CARD_LEFT_X,
                  deg, ARM_DEG_MIN, ARM_DEG_MAX);
}

void MainView::setDetectorDeg(int16_t deg)
{
    setAngleValue(valDetDeg, valDetDegBuffer, VALDETDEG_SIZE, markDegDet, GAUGE_DET, CARD_RIGHT_X,
                  deg, DET_DEG_MIN, DET_DEG_MAX);
}

void MainView::setArmDegDash()
{
    setAngleDash(valArmDeg, valArmDegBuffer, VALARMDEG_SIZE, markDegArm, GAUGE_ARM, CARD_LEFT_X);
}

void MainView::setDetectorDegDash()
{
    setAngleDash(valDetDeg, valDetDegBuffer, VALDETDEG_SIZE, markDegDet, GAUGE_DET, CARD_RIGHT_X);
}

void MainView::setAngleValue(TextAreaWithOneWildcard& val, Unicode::UnicodeChar* buf, uint16_t bufSize,
                             TextArea& mark, Gauge g, int16_t cardX, int16_t deg, int16_t minDeg, int16_t maxDeg)
{
    val.invalidate();
    mark.invalidate();

    Unicode::snprintf(buf, bufSize, "%d", deg);

    /* 숫자는 두 자리든 세 자리든 게이지(=카드) 중심에 가운데 정렬, 도 기호는 숫자 오른쪽에 붙어 따라간다 */
    const int16_t textW = val.getTextWidth();
    val.setX(cardX);
    const int16_t textLeft = cardX + (CARD_W - textW) / 2;
    mark.setXY(textLeft + textW + MARK_DEG_DX, val.getY() + MARK_DEG_DY);
    mark.setVisible(true);

    val.invalidate();
    mark.invalidate();

    /* 원호 목표각: [minDeg, maxDeg]를 시각적 -128도..+128도에 선형 대응.
     * 양끝 라운드 캡(약 5.7도씩)만큼 실제 끝각을 줄여 그린다. 실제 이동은 handleTickEvent가 담당 */
    float f = (float)(deg - minDeg) / (float)(maxDeg - minDeg);
    if (f < 0.0f)
    {
        f = 0.0f;
    }
    if (f > 1.0f)
    {
        f = 1.0f;
    }
    float endAngle = (float)ARC_START + 256.0f * f - 11.4f;
    if (endAngle < (float)ARC_START)
    {
        endAngle = (float)ARC_START;
    }
    if (endAngle > (float)ARC_END)
    {
        endAngle = (float)ARC_END;
    }
    /* 이동 시간 = 직전 값 갱신과의 간격. 값이 300ms마다 조금씩 오면 그 간격에 맞춰
     * 등속으로 움직여 다음 값이 올 때 목표에 닿는다. 대시에서 첫 값이면 고정 시간으로 차오른다 */
    uint16_t frames;
    if (!gaugeHasValue[g])
    {
        frames = ANIM_FIRST_FRAMES;
        gaugeCur[g] = (float)ARC_START;
    }
    else
    {
        const uint32_t interval = frameCount - gaugeLastSetFrame[g];
        frames = (interval < ANIM_MIN_FRAMES) ? ANIM_MIN_FRAMES
               : (interval > ANIM_MAX_FRAMES) ? ANIM_MAX_FRAMES : (uint16_t)interval;
    }
    gaugeLastSetFrame[g] = frameCount;
    gaugeFrom[g]     = gaugeCur[g];      /* 이동 중이었다면 현재 위치에서 이어서 */
    gaugeTarget[g]   = endAngle;
    gaugeAnimLen[g]  = frames;
    gaugeAnimPos[g]  = 0;
    gaugeHasValue[g] = (f > 0.0f);
}

void MainView::setAngleDash(TextAreaWithOneWildcard& val, Unicode::UnicodeChar* buf, uint16_t bufSize,
                            TextArea& mark, Gauge g, int16_t cardX)
{
    val.invalidate();
    mark.invalidate();

    Unicode::strncpy(buf, DASH_ANGLE, bufSize);
    val.setX(cardX);
    mark.setVisible(false);

    val.invalidate();
    mark.invalidate();

    gaugeHasValue[g] = false;   /* 원호는 handleTickEvent에서 숨긴다 */
}

void MainView::animateGauge(Gauge g, Circle& prog)
{
    if (!gaugeHasValue[g])
    {
        if (prog.isVisible())
        {
            prog.setVisible(false);
            prog.invalidate();
        }
        gaugeCur[g] = (float)ARC_START;   /* 다음 값은 바닥에서 차오르게 */
        gaugeAnimPos[g] = gaugeAnimLen[g];
        return;
    }

    bool moved = false;
    if (gaugeAnimPos[g] < gaugeAnimLen[g])
    {
        /* 등속 선형 보간: from → target 을 gaugeAnimLen 프레임에 나눠 이동 */
        gaugeAnimPos[g]++;
        const float t = (float)gaugeAnimPos[g] / (float)gaugeAnimLen[g];
        gaugeCur[g] = gaugeFrom[g] + (gaugeTarget[g] - gaugeFrom[g]) * t;
        moved = true;
    }
    else if (gaugeCur[g] != gaugeTarget[g])
    {
        gaugeCur[g] = gaugeTarget[g];
        moved = true;
    }

    if (!prog.isVisible())
    {
        prog.setVisible(true);
        prog.updateArcEnd(gaugeCur[g]);
        prog.invalidate();
    }
    else if (moved)
    {
        prog.updateArcEnd(gaugeCur[g]);   /* 바뀐 부채꼴 영역만 무효화 */
    }
}

/* ---------------------------------------------------------------- 동작 중 표시 */
void MainView::setActive(Card card, bool active)
{
    cardActive[card] = active;
    const Bitmap frame(active ? BITMAP_CARD_ACT_ID : BITMAP_CARD_NRM_ID);
    const colortype valueColor = active ? COLOR_VALUE_BLUE : COLOR_VALUE_NAVY;

    switch (card)
    {
    case CARD_SID:
        cardSid.setBitmap(frame);
        cardSid.invalidate();
        valSid.setColor(valueColor);
        valSid.invalidate();
        markInchSid.setColor(valueColor);     /* 인치 기호(텍스트)도 값 색을 따라간다 */
        markInchSid.invalidate();
        break;
    case CARD_ARM_UD:
        cardArmUd.setBitmap(frame);
        cardArmUd.invalidate();
        valArmUd.setColor(valueColor);
        valArmUd.invalidate();
        markInchArmUd.setColor(valueColor);
        markInchArmUd.invalidate();
        break;
    case CARD_ARM_DEG:
        cardArmDeg.setBitmap(frame);
        cardArmDeg.invalidate();
        valArmDeg.setColor(valueColor);       /* 각도 숫자와 도 기호도 값 색을 따라간다 */
        valArmDeg.invalidate();
        markDegArm.setColor(valueColor);
        markDegArm.invalidate();
        break;
    case CARD_DET_DEG:
        cardDetDeg.setBitmap(frame);
        cardDetDeg.invalidate();
        valDetDeg.setColor(valueColor);
        valDetDeg.invalidate();
        markDegDet.setColor(valueColor);
        markDegDet.invalidate();
        break;
    }
}

/* ---------------------------------------------------------------- 내부 */
void MainView::initGauge(Circle& track, Circle& prog, int16_t cardX)
{
    track.setPosition(cardX, GAUGE_ROW_Y, CARD_W, 130);
    track.setCircle(53.5f, 84.0f, 40.0f);
    track.setLineWidth(8);
    track.setCapPrecision(10);
    track.setPainter(trackPainter);
    track.setArc(ARC_START, ARC_END);
    add(track);

    prog.setPosition(cardX, GAUGE_ROW_Y, CARD_W, 130);
    prog.setCircle(53.5f, 84.0f, 40.0f);
    prog.setLineWidth(8);
    prog.setCapPrecision(10);
    prog.setPainter(bluePainter);   /* 원호는 동작 여부와 관계없이 항상 파랑 */
    prog.setArc(ARC_START, ARC_START);
    prog.setVisible(false);
    add(prog);
}

void MainView::buildGradient(uint32_t* tex, uint32_t fromRGB, uint32_t toRGB)
{
    const int32_t r0 = (fromRGB >> 16) & 0xFF, g0 = (fromRGB >> 8) & 0xFF, b0 = fromRGB & 0xFF;
    const int32_t r1 = (toRGB >> 16) & 0xFF,   g1 = (toRGB >> 8) & 0xFF,   b1 = toRGB & 0xFF;
    for (int32_t i = 0; i < 1024; i++)
    {
        const uint32_t r = (uint32_t)(r0 + (r1 - r0) * i / 1023);
        const uint32_t g = (uint32_t)(g0 + (g1 - g0) * i / 1023);
        const uint32_t b = (uint32_t)(b0 + (b1 - b0) * i / 1023);
        tex[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}
