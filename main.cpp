#include <glc2d.h>

namespace
{
    // 프로젝트 연결 확인용 콜백입니다. 게임 로직은 아직 구현하지 않습니다.
    int Update()
    {
        return 0;
    }

    int Render()
    {
        return 0;
    }
}

int main()
{
    if (g2_InitSdk() < 0)
        return 1;

    g2_SetFrameMove(Update);
    g2_SetRender(Render);

    // 참조 프로젝트와 같은 960 x 600 논리 화면, 창 모드.
    if (g2_CreateWin(100, 100, 960, 600, "GameCraft - GLC2D", true, 1.0F) < 0)
    {
        g2_DestroyWin();
        return 1;
    }

    const int result = g2_Run();
    g2_DestroyWin();
    return result < 0 ? 1 : 0;
}
