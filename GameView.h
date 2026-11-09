#pragma once
class GameSession;
class GameResources;

struct GameFonts
{
    bool Load();
    int body=-1;
    int subtitle=-1;
    int heading=-1;
};

void DrawGame(const GameSession& game,const GameResources& resources,const GameFonts& fonts);
