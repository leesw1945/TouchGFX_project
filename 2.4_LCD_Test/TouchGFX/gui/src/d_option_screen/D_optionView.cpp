#include <gui/d_option_screen/D_optionView.hpp>
#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <math.h>

/* 캔버스 위젯(Circle) 렌더링 작업 버퍼 - 이 화면 전용으로 공급 */
static uint8_t canvasBuffer[3600];

/* 트랙 이미지(b_gauge_track.png) 내부의 원 중심/반지름 (B안과 동일 에셋).
 * 게이지의 화면 위치는 하드코딩하지 않고 TrackL/TrackR 위젯(Designer 배치)에서
 * 런타임에 읽어온다 → Designer에서 위치를 옮겨도 항상 트랙과 정확히 일치. */
static const float ARC_CX = 37.0f;
static const float ARC_CY = 23.95f;
static const float ARC_R  = 17.0f;

D_optionView::D_optionView()
    : animTick(0)
{
}

void D_optionView::setupScreen()
{
    D_optionViewBase::setupScreen();

    touchgfx::CanvasWidgetRenderer::setupBuffer(canvasBuffer, sizeof(canvasBuffer));

    /* SWIVEL ROT (주황 #F5A623) / DETECTOR ROT (하늘 #4CC2FF) */
    painterL.setColor(touchgfx::Color::getColorFromRGB(245, 166, 35));
    initGauge(arcL, painterL, knobL, TrackL, BITMAP_B_KNOB_ORANGE_ID);

    painterR.setColor(touchgfx::Color::getColorFromRGB(76, 194, 255));
    initGauge(arcR, painterR, knobR, TrackR, BITMAP_B_KNOB_BLUE_ID);

    setGauge(arcL, knobL, TrackL, 0);
    setGauge(arcR, knobR, TrackR, 0);
}

void D_optionView::tearDownScreen()
{
    D_optionViewBase::tearDownScreen();
}

/* 아크/노브를 트랙 이미지 위젯의 실제 좌표에 정렬해 초기화 */
void D_optionView::initGauge(touchgfx::Circle &arc, touchgfx::PainterRGB565 &painter,
                             touchgfx::Image &knob, const touchgfx::Image &track,
                             uint16_t knobBitmapId)
{
    arc.setPosition(track.getX(), track.getY(), track.getWidth(), track.getHeight());
    arc.setCircle(ARC_CX, ARC_CY, ARC_R);
    arc.setLineWidth(4.5f);
    arc.setCapPrecision(10);                /* 라운드 캡 */
    arc.setPainter(painter);
    arc.setArc(ARC_START, ARC_START);       /* 빈 상태로 시작 */
    add(arc);

    knob.setBitmap(touchgfx::Bitmap(knobBitmapId));
    add(knob);
}

void D_optionView::handleTickEvent()
{
    animTick++;

    /* 왕복 위치 0..1 (삼각파) → smoothstep 이징으로 양 끝에서 자연스럽게 감속 */
    uint16_t phase = animTick % (2 * HALF_PERIOD_TICKS);
    float p = (phase < HALF_PERIOD_TICKS)
                  ? (float)phase / HALF_PERIOD_TICKS
                  : (float)(2 * HALF_PERIOD_TICKS - phase) / HALF_PERIOD_TICKS;
    p = p * p * (3.0f - 2.0f * p);

    int16_t sweep = (int16_t)(ARC_SPAN * p + 0.5f);
    setGauge(arcL, knobL, TrackL, sweep);
    setGauge(arcR, knobR, TrackR, sweep);
}

void D_optionView::setGauge(touchgfx::Circle &arc, touchgfx::Image &knob,
                            const touchgfx::Image &track, int16_t sweepDeg)
{
    int16_t endAngle = (int16_t)(ARC_START + sweepDeg);

    /* updateArcEnd는 바뀐 부채꼴 영역만 invalidate → 전송량 최소화 */
    arc.updateArcEnd(endAngle);

    /* 노브: 아크 끝점 좌표 (0도=12시, 시계방향) - 트랙 위젯 좌표 기준 */
    float rad = (float)endAngle * 3.14159265f / 180.0f;
    float cx = (float)track.getX() + ARC_CX + ARC_R * sinf(rad);
    float cy = (float)track.getY() + ARC_CY - ARC_R * cosf(rad);
    knob.moveTo((int16_t)(cx - 4.0f + 0.5f), (int16_t)(cy - 4.0f + 0.5f));
}
