#include "GameSession.h"
#include "GameView.h"
#include "GameplayBot.h"
#include "glc2d.h"
#include <filesystem>
#include <cstdio>

namespace
{
    std::array<GameSession,13> frames;
    const std::array<const char*,13> labels={"시작","위 사격","아래 사격","이동 중 사격","앉아 사격",
        "1단계 완료","철제 보급기지","기지 공성포","야간 요새","로켓 장갑차","모든 작전 완료",
        "2단계 재도전","박격포 경고"};
    GameResources resources;
    GameFonts fonts;
    int index=0;
    int Update()
    {
        const KEYCODE* keys=g2_GetKeyboard();
        for(int key='0';key<='9';++key) if(keys[key]==EINPUT_DOWN)
        {
            index=key=='0'?10:key-'0';
            g2_SetWindowTitle("화면 검증 · %s",labels[index]);
        }
        if(keys['M']==EINPUT_DOWN)
        { index=12; g2_SetWindowTitle("화면 검증 · %s",labels[index]); }
        if(keys[VK_SPACE]==EINPUT_DOWN)
        { index=(index+1)%static_cast<int>(frames.size()); g2_SetWindowTitle("화면 검증 · %s",labels[index]); }
        return 0;
    }
    int Render() { DrawGame(frames[index],resources,fonts); return 0; }
}
int main()
{
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path();
    SetCurrentDirectoryW(root.c_str());
    GameSession game;
    auto capture=[&](int index,GameInput input)
    {
        GameSession test; test.Restart();
        for(int step=0;step<12;++step)
        {
            GameInput current=input;
            if(step>0) { current.jump=false; current.grenade=false; }
            test.Update(1.0F/60,current);
        }
        frames[index]=test;
    };
    GameInput up; up.aimUp=true; up.fire=true; capture(1,up);
    GameInput down; down.jump=true; down.crouch=true; down.fire=true; capture(2,down);
    GameInput run; run.right=true; run.fire=true; capture(3,run);
    GameInput crouch; crouch.crouch=true; crouch.fire=true; capture(4,crouch);
    game.Restart();
    bool mortarCaptured=false;
    for(int stage=1;stage<=StageCount;++stage)
    {
        if(stage>1) frames[stage==2?6:8]=game;
        if(stage==2)
        {
            auto failed=game;
            for(int i=0;i<4000&&failed.State()==PlayState::Playing;++i) failed.Update(1.0F/60,{});
            frames[11]=failed;
        }
        float engaged=-1; bool bossCaptured=false;
        for(int step=0;step<18000&&game.State()==PlayState::Playing;++step)
        {
            game.Update(1.0F/60,VerificationInput(game,engaged));
            if(stage==3&&!mortarCaptured&&game.BossEngaged())
            {
                auto hold=game;
                for(int i=0;i<150&&!mortarCaptured&&hold.State()==PlayState::Playing;++i)
                {
                    for(const Enemy& e:hold.Enemies()) if(e.kind==EnemyKind::Boss&&e.attack.kind==AttackKind::Mortar)
                    { frames[12]=hold; mortarCaptured=true; break; }
                    hold.Update(1.0F/60,{});
                }
            }
            if(stage>1&&!bossCaptured) for(const Enemy& e:game.Enemies())
                if(e.kind==EnemyKind::Boss && e.hp>0 && e.hp<e.maxHp && std::abs(e.x-game.Player().X())<550)
                { frames[stage==2?7:9]=game; bossCaptured=true; break; }
        }
        if(game.State()!=(stage==StageCount?PlayState::Clear:PlayState::StageClear)) return 2;
        if(stage==1) frames[5]=game;
        if(stage<StageCount) { GameInput next; next.start=true; game.Update(1.0F/60,next); }
    }
    if(!mortarCaptured) return 6;
    frames[10]=game;
    if(g2_InitSdk()<0) return 3;
    g2_SetStateShow(0); g2_SetFrameMove(Update); g2_SetRender(Render);
    if(g2_CreateWin(100,100,960,600,"화면 검증 · Space로 다음 장면",true)<0) return 4;
    const char* failed=nullptr;
    if(!resources.Load(failed)) { std::fprintf(stderr,"%s\n",failed); g2_DestroyWin(); return 5; }
    if(!fonts.Load()) { resources.Unload(); g2_DestroyWin(); return 7; }
    g2_Run(); resources.Unload(); g2_DestroyWin(); return 0;
}
