#include "CApplication.h"
#include "glc2d.h"
#include "GameView.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

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
        FailInit("Could not find the executable folder.");
    executablePath.resize(pathLength);
    const auto executableDirectory = std::filesystem::path(executablePath).parent_path();
    if (!SetCurrentDirectoryW(executableDirectory.c_str()))
        FailInit("Could not open the executable folder.");

    if (g2_InitSdk() < 0)
        FailInit("GLC2D initialization failed.");
    m_sdkInitialized = true;

    g2_SetFrameMove(FrameMove);
    g2_SetRender(DrawScene);
    g2_SetStateShow(0);
    if (g2_CreateWin(100, 100, GameConfig::ScreenWidth, GameConfig::ScreenHeight,
        "GameCraft - GLC2D", true, 1.0F) < 0)
        FailInit("Window creation failed.");

    const char* failedPath = nullptr;
    if (!m_resources.Load(failedPath))
        FailInit(failedPath);
    if (!m_resources.PlayBackgroundSound())
        FailInit("Background sound playback failed.");

    m_font = g2_FontCreate("Arial", 16);
    m_titleFont = g2_FontCreate("Arial", 26);
    m_largeFont = g2_FontCreate("Arial", 42);
    if (m_font < 0 || m_titleFont < 0 || m_largeFont < 0)
        FailInit("Font creation failed.");

    m_lastUpdateTime = g2_TimeGetTime();
    std::puts("Enter: Start / Retry. A/D: Move. Space: Jump. S: Crouch. J: Fire. K: Grenade. P: Pause.");
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
    const long long now = g2_TimeGetTime();
    const float deltaSeconds = std::clamp(
        static_cast<float>(now - m_lastUpdateTime) * 0.001F, 0.0F, 0.05F);
    m_lastUpdateTime = now;


    if (GetForegroundWindow() != g2_GetHwnd())
    {
        return 0;
    }

    const KEYCODE* keys = g2_GetKeyboard();
    if(!keys) return 0;
    GameInput input;
    input.left=IsHeld(keys['A']); input.right=IsHeld(keys['D']);
    input.jump=keys[VK_SPACE]==EINPUT_DOWN; input.crouch=IsHeld(keys['S']);
    input.fire=IsHeld(keys['J']); input.grenade=keys['K']==EINPUT_DOWN;
    input.start=keys[VK_RETURN]==EINPUT_DOWN; input.retry=keys['R']==EINPUT_DOWN;
    input.pause=keys['P']==EINPUT_DOWN || keys[VK_ESCAPE]==EINPUT_DOWN;
    m_session.Update(deltaSeconds,input);
    for(GameSound sound:m_session.Sounds()) m_resources.Play(sound);
    return 0;
}

int CApplication::Render()
{
    DrawGame(m_session,m_resources,m_font,m_titleFont,m_largeFont);
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
    std::fprintf(stderr, "GameCraft could not start: %s\n", reason);
    Destroy();
    std::exit(EXIT_FAILURE);
}
