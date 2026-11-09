#include "GameSession.h"
#include "GameplayBot.h"
#include "ProjectileVisual.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

void Check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }

void CheckTimingAndTerrain()
{
    int shotCount=0;
    for(int fps:{30,60,144})
    {
        GameSession game;
        game.Restart();
        GameInput runAndFire;
        runAndFire.right=runAndFire.fire=true;
        int shots=0;
        for(int i=0;i<fps;++i)
        {
            game.Update(1.0F/fps,runAndFire);
            shots+=static_cast<int>(std::count(game.Sounds().begin(),game.Sounds().end(),GameSound::Shot));
        }
        if(fps==30) shotCount=shots;
        Check(shots==shotCount&&shots>=6,"fixed simulation preserves fire cadence at different render rates");
        Check(std::abs(game.Time()-1)<0.001F &&
            std::abs(game.Player().X()-GameConfig::StartX-GameConfig::MoveSpeed)<0.05F,
            "fixed simulation preserves run speed at different render rates");
    }

    GameSession game;
    game.Restart();
    GameInput tap;
    tap.jump=tap.grenade=true;
    game.Update(0.002F,tap);
    Check(game.Grenades()==3&&game.Player().IsGrounded(),"short render frame has no physics step");
    game.Update(1.0F/60,{});
    Check(game.Grenades()==2&&!game.Player().IsGrounded()&&
        std::count(game.Sounds().begin(),game.Sounds().end(),GameSound::Jump)==1,
        "jump and grenade edges survive until the next simulation step");

    game.Restart();
    game.Update(0.002F,tap);
    game.Suspend();
    GameInput resume;
    resume.pause=true;
    game.Update(1.0F/60,resume);
    game.Update(1.0F/60,{});
    Check(game.State()==PlayState::Playing&&game.Grenades()==3&&game.Player().IsGrounded(),
        "focus pause clears queued input before resuming");

    game.Restart();
    GameInput right;
    right.right=true;
    for(int i=0;i<78;++i) game.Update(1.0F/60,right);
    GameInput throwGrenade;
    throwGrenade.grenade=true;
    game.Update(1.0F/60,throwGrenade);
    for(int i=0;i<67;++i) game.Update(1.0F/60,{});
    Check(std::any_of(game.Effects().begin(),game.Effects().end(),[](const GameEffect& effect)
        { return effect.kind==EffectKind::Explosion && effect.x>700 && effect.x<956 && std::abs(effect.y-436)<0.05F; }),
        "descending grenade explodes on the first catwalk it touches");
    for(int i=0;i<74;++i) game.Update(1.0F/60,right);
    GameInput jump;
    jump.jump=true;
    game.Update(1.0F/60,jump);
    for(int i=0;i<70;++i) game.Update(1.0F/60,{});
    Check(game.Player().IsGrounded()&&game.Player().Y()==436,"drop test starts on the first catwalk");
    jump.crouch=true;
    game.Update(0.002F,jump);
    game.Update(1.0F/60,{});
    Check(!game.Player().IsGrounded()&&game.Player().Y()>436 && game.Sounds().empty(),
        "drop-through chord remains a drop even if released before the physics step");
}

void CheckMortarWarning(GameSession game)
{
    const auto warning=[&]() -> const EnemyAttack* {
        for(const Enemy& enemy:game.Enemies())
            if(enemy.kind==EnemyKind::Boss&&enemy.attack.kind==AttackKind::Mortar) return &enemy.attack;
        return nullptr;
    };
    for(int i=0;i<150&&!warning()&&game.State()==PlayState::Playing;++i)
        game.Update(1.0F/60,{});
    Check(warning()&&warning()->shotCount>0,"fortress warns before launching its mortar");
    const float landingX=warning()->shots[0].landingX;
    GameInput dodge;
    dodge.right=true;
    for(int i=0;i<29;++i) game.Update(1.0F/60,dodge);
    Check(warning()&&warning()->shots[0].landingX==landingX&&game.Player().X()-landingX>100,
        "mortar marker stays fixed as the player moves out of range");
    const int health=game.Health();
    bool released=false,landed=false;
    for(int i=0;i<105;++i)
    {
        game.Update(1.0F/60,{});
        for(const Projectile& shot:game.Projectiles())
            released|=shot.type==ProjectileType::Mortar&&shot.landingX==landingX;
        for(const GameEffect& effect:game.Effects())
            landed|=effect.kind==EffectKind::Explosion&&effect.x==landingX&&effect.y==GameConfig::GroundY;
    }
    Check(released&&landed&&game.Health()==health,"dodged mortar detonates at the marked point");
}

int main()
{
    try
    {
        CheckTimingAndTerrain();
        for(int fps:{30,60,144})
        {
            GamePlayer player; GameStage stage;
            for(int i=0;i<fps;++i) player.Update(1.0F/fps,false,true,false,stage.Platforms());
            Check(std::abs(player.X()-(GameConfig::StartX+GameConfig::MoveSpeed))<0.05F,"frame-independent movement");
            const float x=player.X(); player.Update(0.016F,true,true,false,stage.Platforms());
            Check(player.X()==x,"opposite inputs must cancel");
            for(int i=0;i<2500;++i) player.Update(0.016F,true,false,false,stage.Platforms());
            Check(player.X()==64,"world left boundary");
            for(int i=0;i<2500;++i) player.Update(0.016F,false,true,false,stage.Platforms());
            Check(player.X()==GameConfig::WorldWidth-64,"world right boundary");
        }
        GameStage stage; GamePlayer jumper;
        for(int i=0;i<150;++i) jumper.Update(1.0F/60,false,true,false,stage.Platforms());
        Check(jumper.X()>700 && jumper.X()<950,"jump test setup");
        float highest=jumper.Y();
        for(int i=0;i<80;++i)
        { jumper.Update(1.0F/60,false,false,i==0,stage.Platforms()); highest=(std::min)(highest,jumper.Y()); }
        Check(highest<GameConfig::GroundY-100,"jump height");
        Check(jumper.IsGrounded()&&std::abs(jumper.Y()-436)<0.1F,"one-way platform landing");
        for(int i=0;i<100;++i) jumper.Update(1.0F/60,true,false,false,stage.Platforms());
        Check(jumper.IsGrounded()&&jumper.Y()==GameConfig::GroundY,"walk off platform and land");
        for(int i=0;i<300;++i) stage.Update(1.0F/60,GameConfig::WorldWidth-64);
        Check(std::abs(stage.CameraX()-(GameConfig::WorldWidth-GameConfig::ScreenWidth))<0.1F,"camera end boundary");
        GameSession poses; poses.Restart();
        GameInput movingFire; movingFire.right=true; movingFire.fire=true;
        poses.Update(1.0F/60,movingFire);
        Check(poses.Pose()==PlayerPose::RunShoot&&poses.Shooting(),"running must keep armed firing pose");
        Check(!poses.Projectiles().empty()&&std::abs(poses.Projectiles()[0].y-poses.WeaponY())<0.01F,"shot and muzzle must share height");
        poses.Restart(); GameInput crouchFire; crouchFire.crouch=true; crouchFire.fire=true;
        poses.Update(1.0F/60,crouchFire);
        Check(poses.Pose()==PlayerPose::CrouchShoot&&poses.Shooting()&&!poses.Player().IsMoving(),"crouch firing must use short armed pose");
        Check(std::abs(poses.WeaponY()-(GameConfig::GroundY-50.5F))<1.0F,"crouched muzzle height");
        const auto grenadeArt=ProjectileArt(true,false);
        poses.Restart(); GameInput throwCrouched; throwCrouched.crouch=true; throwCrouched.grenade=true;
        poses.Update(1.0F/60,throwCrouched);
        Check(poses.Pose()==PlayerPose::Crouch&&poses.Throwing(),"crouched grenade must not switch to tall standing pose");
        Check(poses.Grenades()==2&&poses.Projectiles().size()==1&&poses.Projectiles()[0].type==ProjectileType::Grenade,"crouch grenade spawn");
        Check(grenadeArt.width==20&&grenadeArt.height==22,"grenade display dimensions are fixed");
        Check(ProjectileArt(false,false).width>=30,"rifle bullet must remain readable");
        poses.Restart(); GameInput fireUp; fireUp.aimUp=true; fireUp.fire=true;
        poses.Update(1.0F/60,fireUp);
        Check(poses.Pose()==PlayerPose::AimUp && poses.VisualFrame().Vertical() && poses.Shooting(),"aim up uses matching vertical sprite");
        Check(poses.Projectiles().size()==1 && poses.Projectiles().front().vx==0 && poses.Projectiles().front().vy<0,"up projectile velocity");
        poses.Restart(); GameInput takeoff; takeoff.jump=true; poses.Update(1.0F/60,takeoff);
        for(int i=0;i<17;++i) poses.Update(1.0F/60,{});
        poses.Update(1.0F/60,crouchFire);
        Check(poses.Pose()==PlayerPose::AimDown && !poses.Crouching() && poses.Shooting(),"air S aims down, not crouch");
        Check(poses.Projectiles().size()==1 && poses.Projectiles().front().vx==0 && poses.Projectiles().front().vy>0,"down projectile velocity");
        GameSession dive; dive.Restart(); GameInput forward; forward.right=true;
        for(int i=0;i<200&&dive.Player().X()<dive.Enemies().front().x-105;++i) dive.Update(1.0F/60,forward);
        forward.jump=true; dive.Update(1.0F/60,forward); forward.jump=false;
        for(int i=0;i<30&&dive.Player().X()<dive.Enemies().front().x-5;++i) dive.Update(1.0F/60,forward);
        for(int i=0;i<16;++i) dive.Update(1.0F/60,crouchFire);
        Check(dive.Enemies().front().hp<2,"airborne down fire hits a ground soldier");
        GameSession lifecycle; lifecycle.Restart();
        GameInput kill; kill.fire=true;
        bool corpseObserved=false; float corpseX=0,corpseY=0; int corpseFacing=0;
        for(int step=0;step<180&&!corpseObserved;++step)
        {
            lifecycle.Update(1.0F/60,kill);
            for(const Enemy& e:lifecycle.Enemies()) if(e.home==640&&e.hp<=0)
            { corpseObserved=true; corpseX=e.x; corpseY=e.y; corpseFacing=e.facing; }
        }
        Check(corpseObserved,"death animation must appear briefly");
        for(int step=0;step<70;++step)
        {
            GameInput move; move.right=true; lifecycle.Update(1.0F/60,move);
            for(const Enemy& e:lifecycle.Enemies()) if(e.home==640)
                Check(e.hp<=0&&e.x==corpseX&&e.y==corpseY&&e.facing==corpseFacing,"corpse must stay at death position and keep facing");
        }
        for(const Enemy& e:lifecycle.Enemies()) Check(e.home!=640,"expired corpse must be removed from world");
        GameSession game;
        Check(game.State()==PlayState::Title,"title screen");
        Check(game.Music()==GameMusic::Silence,"title has no stage or boss loop");
        GameInput start; start.start=true; game.Update(0.016F,start);
        Check(game.State()==PlayState::Playing&&game.Enemies().size()==10,"start spawns complete stage");
        Check(game.Music()==GameMusic::Stage,"deploy starts the stage loop");
        auto waiting=game;
        bool visibleWarning=false;
        for(int i=0;i<220;++i)
        {
            waiting.Update(1.0F/60,{});
            for(const Enemy& enemy:waiting.Enemies())
            {
                if(enemy.home==640) visibleWarning|=enemy.attack.Active();
                if(enemy.home>=1160) Check(!enemy.attack.Active(),"offscreen enemies cannot wind up a shot");
            }
        }
        Check(visibleWarning,"visible soldiers still warn and shoot normally");
        GameInput pause; pause.pause=true; game.Update(0.016F,pause); const float pausedTime=game.Time();
        for(int i=0;i<60;++i) game.Update(0.016F,{});
        Check(game.State()==PlayState::Paused&&game.Time()==pausedTime,"pause freezes gameplay");
        Check(game.Music()==GameMusic::Stage,"pause keeps the selected music");
        game.Update(0.016F,pause);
        for(int stageIndex=1;stageIndex<=StageCount;++stageIndex)
        {
            Check(game.StageNumber()==stageIndex&&game.Stage().CameraX()==0,"stage starts from its own spawn");
            if(stageIndex==2)
            {
                auto checkpoint=game;
                const int score=checkpoint.Score();
                for(int i=0;i<3000&&checkpoint.State()==PlayState::Playing;++i) checkpoint.Update(1.0F/60,{});
                Check(checkpoint.State()==PlayState::GameOver,"depot checkpoint can fail normally");
                Check(checkpoint.Music()==GameMusic::Silence,"game over stops music");
                checkpoint.Update(1.0F/60,start);
                Check(checkpoint.StageNumber()==2&&checkpoint.State()==PlayState::Playing&&checkpoint.Score()==score&&checkpoint.Health()==3,
                    "retry restores current stage and entry score");
                Check(checkpoint.Music()==GameMusic::Stage,"checkpoint retry restores stage music");
            }
            float engaged=-1; bool upward=false,acquiredAmmo=false,killedDrone=false;
            bool bossMusic=false,stageMusicAfterBoss=false,mortarChecked=false;
            for(int i=0;i<18000&&game.State()==PlayState::Playing;++i)
            {
                const auto input=VerificationInput(game,engaged);
                game.Update(1.0F/60,input);
                bossMusic|=game.Music()==GameMusic::Boss;
                stageMusicAfterBoss|=bossMusic&&game.BossDead()&&game.Music()==GameMusic::Stage;
                upward|=game.Shooting()&&game.Aim()==AimDirection::Up;
                acquiredAmmo|=game.MachineGunAmmo()>0;
                for(const Enemy& e:game.Enemies()) killedDrone|=e.kind==EnemyKind::Drone&&e.hp<=0;
                if(stageIndex==3 && game.BossEngaged() && !mortarChecked)
                {
                    CheckMortarWarning(game);
                    mortarChecked=true;
                }
            }
            std::printf("Stage %d: state=%d hp=%d stage kills=%d score=%d stage time=%.1f x=%.1f y=%.1f up=%d ammo=%d drone=%d\n",
                game.StageNumber(),static_cast<int>(game.State()),game.Health(),game.StageKills(),game.Score(),game.StageTime(),game.Player().X(),game.Player().Y(),upward,acquiredAmmo,killedDrone);
            if(game.State()==PlayState::Playing) for(const Enemy& e:game.Enemies())
                std::printf("Remaining: kind=%d x=%.1f y=%.1f hp=%d\n",static_cast<int>(e.kind),e.x,e.y,e.hp);
            Check(game.State()==(stageIndex<StageCount?PlayState::StageClear:PlayState::Clear)&&game.BossDead(),"normal input must beat every stage boss and reach exit");
            Check(bossMusic&&stageMusicAfterBoss&&game.Music()==GameMusic::Silence,"every stage switches to boss, back to stage, and then stops on results");
            for(const Enemy& e:game.Enemies()) Check(e.kind!=EnemyKind::Boss,"clear must persist after boss corpse removal");
            if(stageIndex==2) Check(upward&&acquiredAmmo&&killedDrone,"depot uses vertical combat, drone and machine gun supplies");
            if(stageIndex==3) Check(mortarChecked,"fortress mortar regression ran");
            if(stageIndex<StageCount)
            {
                const int score=game.Score();
                game.Update(1.0F/60,start);
                Check(game.State()==PlayState::Playing&&game.Score()==score&&!game.BossDead()&&game.Projectiles().empty(),"next stage carries score and clears stage state");
                Check(game.Music()==GameMusic::Stage,"next stage starts its own stage loop");
            }
        }
        GameInput retry; retry.retry=true; game.Update(0.016F,retry);
        Check(game.State()==PlayState::Playing&&game.Health()==3&&game.Grenades()==3&&game.Score()==0&&game.Time()==0,"clear restart resets all state");
        Check(game.Projectiles().empty()&&game.Kills()==0&&!game.BossDead(),"restart clears entities and boss");
        for(int i=0;i<3000&&game.State()==PlayState::Playing;++i) game.Update(1.0F/60,{});
        Check(game.State()==PlayState::GameOver,"enemy fire causes game over");
        game.Update(0.016F,retry);
        Check(game.State()==PlayState::Playing&&game.Health()==3,"game over restart");
        std::puts("PASS: frame timing, buffered input, catwalks, enemy warnings, directional combat, three-stage campaign and retry.");
        return 0;
    }
    catch(const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
}
