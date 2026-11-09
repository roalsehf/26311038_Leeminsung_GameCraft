// GLC2D 연결
#if defined(_DEBUG)
#if defined(_M_X64)
#pragma comment(lib, "glc2d_x64_debug.lib")
#elif defined(_M_IX86)
#pragma comment(lib, "glc2d_win32_debug.lib")
#endif
#else
#if defined(_M_X64)
#pragma comment(lib, "glc2d_x64_release.lib")
#elif defined(_M_IX86)
#pragma comment(lib, "glc2d_win32_release.lib")
#endif
#endif

// 게임 헤더
#include "glc2d.h"
#include "CApplication.h"

CApplication g_app;

int main()
{
    g_app.Init();
    g2_Run();
    g_app.Destroy();
    return 0;
}
