#include <gui/containers/Connecting.hpp>
#include <images/BitmapDatabase.hpp>

Connecting::Connecting()
    : spinnerFrame(0),
      tickDivider(0)
{
}

void Connecting::initialize()
{
    ConnectingBase::initialize();
}

void Connecting::tick()
{
    if (!isVisible())
    {
        return;
    }

    /* 매 틱 돌리면 너무 빠르므로 TICKS_PER_FRAME마다 한 프레임 전진 */
    if (++tickDivider < SPINNER_TICKS_PER_FRAME)
    {
        return;
    }
    tickDivider = 0;

    spinnerFrame = (uint8_t)((spinnerFrame + 1) % SPINNER_FRAME_COUNT);

    /* spinner_f00~f23은 이름이 연속이라 BitmapDatabase에서 ID도 연속 배정됨
     * (에셋 추가/이름 변경 시 이 전제가 깨질 수 있으니 주의) */
    spinnerImg.setBitmap(touchgfx::Bitmap(BITMAP_SPINNER_F00_ID + spinnerFrame));
    spinnerImg.invalidate();   /* 32x32 영역만 재전송 - 부담 거의 없음 */
}
