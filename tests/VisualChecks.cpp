#include "GameSession.h"
#include "GameView.h"
#include "GameplayBot.h"
#include "glc2d.h"
#include <filesystem>
#include <cstdio>

namespace
{
    std::vector<GameSession> frames;
    std::vector<const char*> labels;
    GameResources resources;
    int index=0,font=-1,title=-1,large=-1;
    int Update()
    {
        const KEYCODE* keys=g2_GetKeyboard();
        for(int key='0';key<='9';++key) if(keys[key]==EINPUT_DOWN)
        {
            index=key=='0'?10:key-'0';
            g2_SetWindowTitle("GameCraft Visual Check - %s",labels[index]);
        }
        if(g2_GetKeyboard()[VK_SPACE]==EINPUT_DOWN)
        { index=(index+1)%static_cast<int>(frames.size()); g2_SetWindowTitle("GameCraft Visual Check - %s",labels[index]); }
        return 0;
    }
    int Render() { DrawGame(frames[index],resources,font,title,large); return 0; }
}
int main()
{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path();
    SetCurrentDirectoryW(root.c_str());
    GameSession game; frames.push_back(game); labels.push_back("TITLE");
    auto capture=[&](const char* label,GameInput input)
    {
        GameSession test; test.Restart();
        for(int step=0;step<12;++step)
        {
            GameInput current=input;
            if(step>0) { current.jump=false; current.grenade=false; }
            test.Update(1.0F/60,current);
        }
        frames.push_back(test); labels.push_back(label);
    };
    capture("IDLE",{});
    GameInput run; run.right=true; capture("RUN",run);
    GameInput shoot; shoot.fire=true; capture("STANDING FIRE",shoot);
    run.fire=true; capture("RUN AND FIRE",run);
    run.right=false; run.left=true; capture("LEFT RUN AND FIRE",run);
    GameInput crouch; crouch.crouch=true; capture("CROUCH",crouch);
    crouch.fire=true; capture("CROUCH AND FIRE",crouch);
    crouch.fire=false; crouch.grenade=true; capture("CROUCH AND GRENADE",crouch);
    GameInput grenade; grenade.grenade=true; capture("STANDING GRENADE",grenade);
    GameInput jump; jump.jump=true; jump.fire=true; capture("JUMP AND FIRE",jump);
    game.Restart(); float engaged=-1; int checkpoint=0;
    for(int step=0;step<18000&&game.State()==PlayState::Playing;++step)
    {
        game.Update(1.0F/60,VerificationInput(game,engaged));
        if(checkpoint<3 && game.Player().X()>(checkpoint==0?900:checkpoint==1?2300:4230))
        { frames.push_back(game); labels.push_back("STAGE PROGRESS"); ++checkpoint; }
        for(const Enemy& enemy:game.Enemies())
            if(checkpoint==3 && enemy.kind==EnemyKind::Boss && enemy.hp>0 && enemy.hp<20)
            { frames.push_back(game); labels.push_back("BOSS FIGHT"); ++checkpoint; break; }
    }
    if(game.State()!=PlayState::Clear) return 2;
    frames.push_back(game); labels.push_back("CLEAR");
    game.Restart(); for(int i=0;i<4000&&game.State()==PlayState::Playing;++i) game.Update(1.0F/60,{});
    frames.push_back(game); labels.push_back("GAME OVER");
    if(g2_InitSdk()<0) return 3;
    g2_SetStateShow(0); g2_SetFrameMove(Update); g2_SetRender(Render);
    if(g2_CreateWin(100,100,960,600,"GameCraft Visual Check - SPACE: next",true)<0) return 4;
    const char* failed=nullptr;
    if(!resources.Load(failed)) { std::fprintf(stderr,"%s\n",failed); g2_DestroyWin(); return 5; }
    font=g2_FontCreate("Arial",16); title=g2_FontCreate("Arial",26); large=g2_FontCreate("Arial",42);
    g2_Run(); resources.Unload(); g2_DestroyWin(); return 0;
}
