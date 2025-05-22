#include <gui/main_screen/MainView.hpp>
#include <touchgfx/utils.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

void MainView::buttonUpClicked()
{
    touchgfx_printf("buttonUpClicked\n");

    counter++;
    Unicode::snprintf(textCounterBuffer, TEXTCOUNTER_SIZE, "%d", counter);
    // 반드시 .invalidate()를 호출해야 TouchGFX가 이 텍스트가 바뀌었음을 인식하고 화면을 다시 그립니다.
    textCounter.invalidate();
}

void MainView::buttonDownClicked()
{
    touchgfx_printf("buttonDownClicked\n");

    counter--;
    Unicode::snprintf(textCounterBuffer, TEXTCOUNTER_SIZE, "%d", counter);
    textCounter.invalidate();
}
