#pragma once

#include "GamePlayer.h"
#include "GameResources.h"
#include "GameStage.h"
#include "GameSession.h"

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
    int m_font = -1;
    int m_titleFont = -1;
    int m_largeFont = -1;
    long long m_lastUpdateTime = 0;
    bool m_sdkInitialized = false;
};

extern CApplication g_app;
