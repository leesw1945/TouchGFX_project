#include <gui/b_option_screen/B_optionView.hpp>
#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <math.h>

/* 캔버스 위젯(Circle)의 스캔라인 렌더링 작업 버퍼.
 * B 화면에는 Designer가 만든 캔버스 위젯이 없어 버퍼가 없으므로 직접 공급. */
static uint8_t canvasBuffer[3600];

/* 트랙 이미지(b_gauge_track.png) 내부의 원 중심/반지름 - 에셋 생성 스크립트와 동일 값.
 * 게이지의 화면 위치는 하드코딩하지 않고 GaugeSw/GaugeDet 위젯(Designer 배치)에서
 * 런타임에 읽어온다 → Designer에서 위치를 옮겨도 코드 수정 불필요. */
static const float ARC_CX = 37.0f;
static const float ARC_CY = 23.95f;
static const float ARC_R  = 17.0f;

B_optionView::B_optionView()
    : animTick(0)
{
}

void B_optionView::setupScreen()
{
    B_optionViewBase::setupScreen();

    touchgfx::CanvasWidgetRenderer::setupBuffer(canvasBuffer, sizeof(canvasBuffer));

    /* SWIVEL 게이지 (주황 #F5A623) */
    painterSw.setColor(touchgfx::Color::getColorFromRGB(245, 166, 35));
    initGauge(arcSw, painterSw, GaugeSw);

    /* DETECTOR 게이지 (하늘 #4CC2FF) */
    painterDet.setColor(touchgfx::Color::getColorFromRGB(76, 194, 255));
    initGauge(arcDet, painterDet, GaugeDet);

    /* 노브 (아크 끝점 표시) */
    knobSw.setBitmap(touchgfx::Bitmap(BITMAP_B_KNOB_ORANGE_ID));
    add(knobSw);
    knobDet.setBitmap(touchgfx::Bitmap(BITMAP_B_KNOB_BLUE_ID));
    add(knobDet);

    setGauge(arcSw, knobSw, GaugeSw, 0);
    setGauge(arcDet, knobDet, GaugeDet, 0);
}

void B_optionView::tearDownScreen()
{
    B_optionViewBase::tearDownScreen();
}

/* 아크 위젯을 트랙 이미지와 같은 위치/크기로 정렬하고 초기화 */
void B_optionView::initGauge(touchgfx::Circle &arc, touchgfx::PainterRGB565 &painter,
                             const touchgfx::Image &track)
{
    arc.setPosition(track.getX(), track.getY(), track.getWidth(), track.getHeight());
    arc.setCircle(ARC_CX, ARC_CY, ARC_R);
    arc.setLineWidth(4.5f);
    arc.setCapPrecision(10);                /* 라운드 캡 */
    arc.setPainter(painter);
    arc.setArc(ARC_START, ARC_START);       /* 빈 상태로 시작 */
    add(arc);
}

void B_optionView::handleTickEvent()
{
    animTick++;

    /* 왕복 위치 0..1 (삼각파) → smoothstep 이징으로 양 끝에서 자연스럽게 감속 */
    uint16_t phase = animTick % (2 * HALF_PERIOD_TICKS);
    float p = (phase < HALF_PERIOD_TICKS)
                  ? (float)phase / HALF_PERIOD_TICKS
                  : (float)(2 * HALF_PERIOD_TICKS - phase) / HALF_PERIOD_TICKS;
    p = p * p * (3.0f - 2.0f * p);

    int16_t sweep = (int16_t)(ARC_SPAN * p + 0.5f);
    setGauge(arcSw, knobSw, GaugeSw, sweep);
    setGauge(arcDet, knobDet, GaugeDet, sweep);
}

void B_optionView::setGauge(touchgfx::Circle &arc, touchgfx::Image &knob,
                            const touchgfx::Image &track, int16_t sweepDeg)
{
    int16_t endAngle = (int16_t)(ARC_START + sweepDeg);

    /* updateArcEnd는 바뀐 부채꼴 영역만 invalidate → 전송량 최소화 */
    arc.updateArcEnd(endAngle);

    /* 노브: 아크 끝점 좌표 (0도=12시, 시계방향 기준) - 트랙 위젯 좌표 기준 */
    float rad = (float)endAngle * 3.14159265f / 180.0f;
    float cx = (float)track.getX() + ARC_CX + ARC_R * sinf(rad);
    float cy = (float)track.getY() + ARC_CY - ARC_R * cosf(rad);
    knob.moveTo((int16_t)(cx - 4.0f + 0.5f), (int16_t)(cy - 4.0f + 0.5f));
}
