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
static uint32_t texGray[1024];

/* 색상 (디자인 계측값) */
static const colortype COLOR_READY_GREEN = Color::getColorFromRGB(0x36, 0xD2, 0x60);
static const colortype COLOR_EMERGENCY   = Color::getColorFromRGB(0xEB, 0x4B, 0x4B);
static const colortype COLOR_VALUE_NAVY  = Color::getColorFromRGB(0x25, 0x3D, 0x4A);
static const colortype COLOR_VALUE_BLUE  = Color::getColorFromRGB(0x00, 0x77, 0xC0);
static const colortype COLOR_TRACK       = Color::getColorFromRGB(0xE6, 0xE6, 0xE6);
static const uint32_t  GRAD_BLUE_FROM = 0x0077C0;   /* 원호 아래쪽(시작점) */
static const uint32_t  GRAD_BLUE_TO   = 0x62B8ED;   /* 원호 위쪽 */
static const uint32_t  GRAD_GRAY_FROM = 0x8C8C8C;
static const uint32_t  GRAD_GRAY_TO   = 0xBDBDBD;

MainView::MainView()
    : state(STATE_READY), demoTick(0), demoLastDeg(-1)
{
    for (int i = 0; i < 4; i++)
    {
        cardActive[i] = false;
    }
}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    CanvasWidgetRenderer::setupBuffer(canvasBuffer, sizeof(canvasBuffer));

    trackPainter.setColor(COLOR_TRACK);

    buildGradient(texBlue, GRAD_BLUE_FROM, GRAD_BLUE_TO);
    buildGradient(texGray, GRAD_GRAY_FROM, GRAD_GRAY_TO);
    /* 세로 그라디언트: 원호 아래끝(y=111) 색 -> 원호 위끝(y=40) 색. 좌표는 위젯(카드) 기준 */
    bluePainter.setGradientTexture(texBlue, true);
    bluePainter.setGradientEndPoints(53.5f, 111.0f, 53.5f, 40.0f, (float)CARD_W, 130.0f, Matrix3x3());
    bluePainter.setWidgetWidth(CARD_W);
    grayPainter.setGradientTexture(texGray, true);
    grayPainter.setGradientEndPoints(53.5f, 111.0f, 53.5f, 40.0f, (float)CARD_W, 130.0f, Matrix3x3());
    grayPainter.setWidgetWidth(CARD_W);

    initGauge(trackArm, progArm, CARD_LEFT_X);
    initGauge(trackDet, progDet, CARD_RIGHT_X);

    /* ---- 목업 초기 상태 (첨부 디자인 기준) ----
     * SID는 cm 표기(인치 기호 없음), ARM UP/DOWN은 inch 표기(120cm = 47.2inch)
     * ARM UP/DOWN 카드와 ARM 회전 게이지가 동작 중(파랑) */
    setState(STATE_READY);
    setActive(CARD_SID, false);
    setActive(CARD_ARM_UD, true);
    setActive(CARD_ARM_DEG, true);
    setActive(CARD_DET_DEG, false);
    setSidCm(180);
    setArmUpDownInch(472);
    setArmDeg(140);
    setDetectorDeg(68);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

/* [2.4_LCD_Test 전용] 조이스틱 CENTER(키 0) -> READY/EMERGENCY 토글 */
void MainView::handleKeyEvent(uint8_t key)
{
    /* 보드: ButtonController가 CENTER를 키 0으로 전달. 시뮬레이터: 키보드 '0'은 SDL 키코드 48('0')로 들어온다 */
    if (key == 0 || key == '0')
    {
        setState(state == STATE_READY ? STATE_EMERGENCY : STATE_READY);
    }
}

/* [2.4_LCD_Test 전용] ARM 게이지 데모: 0 -> GAUGE_VALUE_MAX -> 0 왕복, 양끝에서 감속(smoothstep).
 * 값이 실제로 바뀌는 틱에만 setArmDeg()를 호출해 불필요한 재전송을 피한다. */
void MainView::handleTickEvent()
{
    demoTick++;
    const uint16_t phase = demoTick % (2 * DEMO_HALF_PERIOD_TICKS);
    float p = (phase < DEMO_HALF_PERIOD_TICKS)
                  ? (float)phase / DEMO_HALF_PERIOD_TICKS
                  : (float)(2 * DEMO_HALF_PERIOD_TICKS - phase) / DEMO_HALF_PERIOD_TICKS;
    p = p * p * (3.0f - 2.0f * p);
    const int16_t deg = (int16_t)(GAUGE_VALUE_MAX * p + 0.5f);
    if (deg != demoLastDeg)
    {
        demoLastDeg = deg;
        setArmDeg(deg);
    }
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

/* ---------------------------------------------------------------- 회전 각도 값 + 게이지 */
void MainView::setArmDeg(int16_t deg)
{
    setAngleValue(valArmDeg, valArmDegBuffer, VALARMDEG_SIZE, markDegArm, progArm, CARD_LEFT_X, deg);
}

void MainView::setDetectorDeg(int16_t deg)
{
    setAngleValue(valDetDeg, valDetDegBuffer, VALDETDEG_SIZE, markDegDet, progDet, CARD_RIGHT_X, deg);
}

void MainView::setAngleValue(TextAreaWithOneWildcard& val, Unicode::UnicodeChar* buf, uint16_t bufSize,
                             TextArea& mark, Circle& prog, int16_t cardX, int16_t deg)
{
    val.invalidate();
    mark.invalidate();

    Unicode::snprintf(buf, bufSize, "%d", deg);

    /* 숫자는 두 자리든 세 자리든 게이지(=카드) 중심에 가운데 정렬, 도 기호는 숫자 오른쪽에 붙어 따라간다 */
    const int16_t textW = val.getTextWidth();
    val.setX(cardX);
    const int16_t textLeft = cardX + (CARD_W - textW) / 2;
    mark.setXY(textLeft + textW + MARK_DEG_DX, val.getY() + MARK_DEG_DY);

    val.invalidate();
    mark.invalidate();

    /* 원호: 시각적 시작 -128도에서 값 비율만큼 채움. 양끝 라운드 캡(약 5.7도씩)을 빼고 실제 끝각을 계산 */
    float visualSweep = 256.0f * (float)deg / (float)GAUGE_VALUE_MAX;
    float endAngle = (float)ARC_START + visualSweep - 11.4f;
    if (endAngle < (float)ARC_START)
    {
        endAngle = (float)ARC_START;
    }
    if (endAngle > (float)ARC_END)
    {
        endAngle = (float)ARC_END;
    }
    prog.setVisible(deg > 0);
    prog.updateArcEnd(endAngle);
}

/* ---------------------------------------------------------------- 동작 중 표시 */
void MainView::setActive(Card card, bool active)
{
    cardActive[card] = active;
    const Bitmap frame(active ? BITMAP_CARD_ACT_ID : BITMAP_CARD_NRM_ID);
    const colortype valueColor = active ? COLOR_VALUE_BLUE : COLOR_VALUE_NAVY;
    AbstractPainter& gaugePainter = active ? static_cast<AbstractPainter&>(bluePainter)
                                           : static_cast<AbstractPainter&>(grayPainter);

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
        progArm.setPainter(gaugePainter);
        progArm.invalidate();
        break;
    case CARD_DET_DEG:
        cardDetDeg.setBitmap(frame);
        cardDetDeg.invalidate();
        progDet.setPainter(gaugePainter);
        progDet.invalidate();
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
    prog.setPainter(grayPainter);
    prog.setArc(ARC_START, ARC_START);
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
