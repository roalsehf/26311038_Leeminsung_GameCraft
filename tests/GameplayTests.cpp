#include "GameSession.h"
#include "GameplayBot.h"
#include "ProjectileVisual.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

void Check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main()
{
    try
    {
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
        Check(poses.Grenades()==2&&poses.Projectiles().size()==1&&poses.Projectiles()[0].explosive,"crouch grenade spawn");
        Check(grenadeArt.width==20&&grenadeArt.height==22,"grenade display dimensions are fixed");
        Check(ProjectileArt(false,false).width>=30,"rifle bullet must remain readable");
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
        GameInput start; start.start=true; game.Update(0.016F,start);
        Check(game.State()==PlayState::Playing&&game.Enemies().size()==10,"start spawns complete stage");
        GameInput pause; pause.pause=true; game.Update(0.016F,pause); const float pausedTime=game.Time();
        for(int i=0;i<60;++i) game.Update(0.016F,{});
        Check(game.State()==PlayState::Paused&&game.Time()==pausedTime,"pause freezes gameplay");
        game.Update(0.016F,pause);
        // An ordinary player strategy: advance, crouch and fire, grenade the boss.
        float engagedHome=-1;
        for(int i=0;i<18000 && game.State()==PlayState::Playing;++i)
        {
            GameInput input; const Enemy* target=nullptr;
            for(const Enemy& e:game.Enemies()) if(e.hp>0&&e.x>game.Player().X()-80
                &&(!target||e.x<target->x)) target=&e;
            if(target && (target->home==engagedHome || target->x-game.Player().X()<(target->kind==EnemyKind::Boss?400:600)))
            {
                engagedHome=target->home;
                input.crouch=true; input.fire=true;
                input.grenade=target->kind==EnemyKind::Boss && game.Grenades()>0;
                if(game.Player().Y()<target->y-54)
                { input={}; input.right=true; }
                else if(game.Player().IsFacingLeft())
                { input.crouch=false; input.right=true; }
            }
            else input.right=true;
            if(!input.crouch && game.Player().IsGrounded())
                for(const Projectile& b:game.Projectiles())
                    if(b.hostile && std::abs(b.x-game.Player().X())<160 && std::abs(b.y-game.Player().Y()+65)<90)
                        input.jump=true;
            const int previousHp=game.Health();
            if(i%3000==0) std::printf("Bot x=%.1f y=%.1f facing=%d target=%.1f hp=%d left=%d right=%d fire=%d\n",game.Player().X(),game.Player().Y(),game.Player().IsFacingLeft(),target?target->x:0,target?target->hp:0,input.left,input.right,input.fire);
            game.Update(1.0F/60,input);
            if(game.Health()!=previousHp) std::printf("Damage t=%.1f x=%.1f y=%.1f hp=%d kills=%d\n",game.Time(),game.Player().X(),game.Player().Y(),game.Health(),game.Kills());
        }
        std::printf("Full run: state=%d hp=%d kills=%d score=%d time=%.1f x=%.1f\n",
            static_cast<int>(game.State()),game.Health(),game.Kills(),game.Score(),game.Time(),game.Player().X());
        Check(game.State()==PlayState::Clear&&game.BossDead(),"normal gameplay must reach clear");
        for(const Enemy& e:game.Enemies()) Check(e.kind!=EnemyKind::Boss,"clear must persist after boss corpse removal");
        Check(game.Kills()==10&&game.Score()>=4000,"combat and scoring");
        GameInput retry; retry.retry=true; game.Update(0.016F,retry);
        Check(game.State()==PlayState::Playing&&game.Health()==3&&game.Grenades()==3&&game.Score()==0&&game.Time()==0,"clear restart resets all state");
        Check(game.Projectiles().empty()&&game.Kills()==0&&!game.BossDead(),"restart clears entities and boss");
        for(int i=0;i<3000&&game.State()==PlayState::Playing;++i) game.Update(1.0F/60,{});
        Check(game.State()==PlayState::GameOver,"enemy fire causes game over");
        game.Update(0.016F,retry);
        Check(game.State()==PlayState::Playing&&game.Health()==3,"game over restart");
        std::puts("PASS: movement, poses, muzzle, projectiles, death cleanup, platforms, camera, full clear, restart.");
        return 0;
    }
    catch(const std::exception& e) { std::fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
}
