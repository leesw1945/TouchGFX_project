#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

/* 애플리케이션 주기 처리(하트비트/키/CAN) — Core/Src/app_main.c
 * 시뮬레이터 빌드에는 펌웨어 코드가 없으므로 SIMULATOR 가드로 제외한다. */
#ifndef SIMULATOR
extern "C" void AppMain_Poll();
#endif

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
    /* 매 프레임(TE 주기, 약 76Hz) 호출 — 메인 컨텍스트라 printf 사용 가능 */
#ifndef SIMULATOR
    AppMain_Poll();
#endif
}
