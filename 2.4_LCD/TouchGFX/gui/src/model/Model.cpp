#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
    /* 매 프레임(TE 주기, 약 76Hz) 호출.
     * 키/CAN 주기 처리는 main()의 while 루프(AppMain_Poll)에서 하고,
     * 여기서는 추후 CAN_App_GetDisplayData() 등을 읽어 화면을 갱신한다. */
}
