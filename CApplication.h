#pragma once

#include "GamePlayer.h"
#include "GameResources.h"
#include "GameStage.h"
#include "GameSession.h"
#include "GameView.h"
#include <chrono>

class CApplication
{
public:
    void Init();
    void Destroy();

private:
    static int FrameMove();
    static int DrawScene();
    int Update();
    int Render();
    [[noreturn]] void FailInit(const char* reason);

    GameResources m_resources;
    GameSession m_session;
    GameFonts m_fonts;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_sdkInitialized = false;
};

extern CApplication g_app;
