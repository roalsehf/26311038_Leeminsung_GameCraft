#include "CApplication.h"
#include "glc2d.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

namespace
{
    bool IsHeld(KEYCODE state)
    {
        return state == EINPUT_DOWN || state == EINPUT_PRESS;
    }
}

void CApplication::Init()
{
    std::wstring executablePath(32768, L'\0');
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath.data(),
        static_cast<DWORD>(executablePath.size()));
    if (pathLength == 0 || pathLength >= executablePath.size())
        FailInit("실행 파일 폴더를 찾을 수 없습니다.");
    executablePath.resize(pathLength);
    const auto executableDirectory = std::filesystem::path(executablePath).parent_path();
    if (!SetCurrentDirectoryW(executableDirectory.c_str()))
        FailInit("실행 파일 폴더를 열 수 없습니다.");

    if (g2_InitSdk() < 0)
        FailInit("GLC2D 초기화에 실패했습니다.");
    m_sdkInitialized = true;

    g2_SetFrameMove(FrameMove);
    g2_SetRender(DrawScene);
    g2_SetStateShow(0);
    if (g2_CreateWin(100, 100, GameConfig::ScreenWidth, GameConfig::ScreenHeight,
        "더스트파이어 작전", true, 1.0F) < 0)
        FailInit("게임 창을 열 수 없습니다.");

    const char* failedPath = nullptr;
    if (!m_resources.Load(failedPath))
        FailInit(failedPath);

    if (!m_fonts.Load())
        FailInit("게임 글꼴을 불러올 수 없습니다.");

    m_lastUpdateTime = std::chrono::steady_clock::now();
}

int CApplication::FrameMove()
{
    return g_app.Update();
}

int CApplication::DrawScene()
{
    return g_app.Render();
}

int CApplication::Update()
{
    const auto now=std::chrono::steady_clock::now();
    const float deltaSeconds=std::chrono::duration<float>(now-m_lastUpdateTime).count();
    m_lastUpdateTime = now;

    if (GetForegroundWindow() != g2_GetHwnd())
    {
        m_session.Suspend();
        return 0;
    }

    const KEYCODE* keys = g2_GetKeyboard();
    if(!keys) return 0;
    GameInput input;
    input.left=IsHeld(keys['A'])||IsHeld(keys[VK_LEFT]); input.right=IsHeld(keys['D'])||IsHeld(keys[VK_RIGHT]);
    input.jump=keys[VK_SPACE]==EINPUT_DOWN; input.crouch=IsHeld(keys['S'])||IsHeld(keys[VK_DOWN]);
    input.aimUp=IsHeld(keys['W'])||IsHeld(keys[VK_UP]);
    input.fire=IsHeld(keys['J']); input.grenade=keys['K']==EINPUT_DOWN;
    input.start=keys[VK_RETURN]==EINPUT_DOWN; input.retry=keys['R']==EINPUT_DOWN;
    input.pause=keys['P']==EINPUT_DOWN || keys[VK_ESCAPE]==EINPUT_DOWN;
    m_session.Update(deltaSeconds,input);
    if (!m_resources.SetMusic(m_session.Music()))
        std::fputs("배경 음악을 재생할 수 없습니다.\n",stderr);
    std::array<bool,static_cast<std::size_t>(GameSound::Count)> played{};
    for(GameSound sound:m_session.Sounds())
        if(!std::exchange(played[static_cast<std::size_t>(sound)],true)) m_resources.Play(sound);
    return 0;
}

int CApplication::Render()
{
    DrawGame(m_session,m_resources,m_fonts);
    return 0;
}

void CApplication::Destroy()
{
    if (!m_sdkInitialized)
        return;

    g2_SetFrameMove(nullptr);
    g2_SetRender(nullptr);
    m_resources.Unload();
    g2_DestroyWin();
    m_sdkInitialized = false;
}

void CApplication::FailInit(const char* reason)
{
    std::fprintf(stderr, "게임을 시작할 수 없습니다. %s\n", reason);
    Destroy();
    std::exit(EXIT_FAILURE);
}
