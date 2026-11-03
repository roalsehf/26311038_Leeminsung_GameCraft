#include "GameView.h"
#include "GameSession.h"
#include "PlayerAnimation.h"
#include "ProjectileVisual.h"
#include "glc2d.h"
#include <algorithm>
#include <cmath>

namespace
{
    void Sprite(const GameResources& r,GameTexture texture,float x,float y,float w,float h,
        bool flip=false,DWORD color=0xFFFFFFFF)
    {
        if(x+w<0 || x>GameConfig::ScreenWidth) return;
        const int id=r.Texture(texture);
        const VEC2 position(x,y), scale((flip?-1.0F:1.0F)*w/g2_TextureWidth(id),h/g2_TextureHeight(id));
        g2_Draw2D(id,nullptr,&position,&scale,nullptr,0,color);
    }
    void Panel(const GameResources& r,float x,float y,float w,float h,DWORD color)
    {
        const int id=r.Texture(GameTexture::HudPanel);
        const VEC2 position(x,y),scale(w/g2_TextureWidth(id),h/g2_TextureHeight(id));
        g2_Draw2D(id,nullptr,&position,&scale,nullptr,0,color,true);
    }
    void SourceSprite(const GameResources& r,GameTexture texture,const RECT& source,
        float x,float y,float width,float height,bool flip=false)
    {
        if(x+width<0 || x>GameConfig::ScreenWidth) return;
        const VEC2 position(x,y),scale((flip?-1.0F:1.0F)*width/(source.right-source.left),
            height/(source.bottom-source.top));
        g2_Draw2D(r.Texture(texture),&source,&position,&scale);
    }
    void Text(int font,int x,int y,const char* text,DWORD color=0xFFFFE5B7)
    {
        g2_FontDrawText(font,VEC2i{x+1,y+2},0xE0101720,"%s",text);
        g2_FontDrawText(font,VEC2i{x,y},color,"%s",text);
    }
}

void DrawGame(const GameSession& g,const GameResources& r,int font,int titleFont,int largeFont)
{
    const float camera=g.Stage().CameraX();
    g.Stage().RenderBackdrop(r);
    for(const Pickup& p:g.Pickups()) if(!p.taken)
        Sprite(r,p.grenade?GameTexture::GrenadePickup:GameTexture::HealthKit,
            p.x-camera-24,p.y-53+std::sin(g.Time()*3)*3,48,48);
    for(const Enemy& e:g.Enemies())
    {
        if(e.hp<=0 && e.deathRemaining<=0) continue;
        GameTexture texture;
        float width=128,height=128;
        if(e.kind==EnemyKind::Soldier)
            texture=e.hp<=0?GameTexture::SoldierDeath:e.windup>0?GameTexture::SoldierShoot:
                std::abs(e.x-g.Player().X())<240?GameTexture::SoldierIdle:
                (static_cast<int>(e.phase*6)%2?GameTexture::SoldierWalk1:GameTexture::SoldierWalk2);
        else if(e.kind==EnemyKind::Turret)
        { width=120; height=110; texture=e.hp<=0?GameTexture::TurretDestroyed:e.windup>0?GameTexture::TurretShoot:GameTexture::TurretIdle; }
        else
        { width=256; height=176; texture=e.hp<=0?GameTexture::TankWreck:e.hurt>0?GameTexture::TankDamaged:e.windup>0?GameTexture::TankShoot:GameTexture::TankIdle; }
        const DWORD alpha=e.hp<=0?static_cast<DWORD>(std::clamp(e.deathRemaining/0.25F,0.0F,1.0F)*255):255;
        const DWORD color=(alpha<<24)|(e.hp>0&&e.hurt>0?0x00FFB8A2:0x00FFFFFF);
        Sprite(r,texture,e.x-camera-width*0.5F,e.y-height,width,height,
            e.kind==EnemyKind::Soldier && e.facing>0,color);
    }
    const GamePlayer& player=g.Player();
    const bool blink=g.Invulnerability()>0 && static_cast<int>(g.Time()*14)%2==0;
    if(!blink || g.State()==PlayState::GameOver)
        player.Render(r,camera,PlayerArt::Frame(g.Pose(),player.RunFrame(),g.Time()));
    if(g.Shooting())
        Sprite(r,GameTexture::Muzzle,g.WeaponX()-camera+(player.IsFacingLeft()?-12.0F:12.0F)-16,
            g.WeaponY()-12,32,24,player.IsFacingLeft());
    for(const Projectile& b:g.Projectiles())
    {
        const ProjectileAppearance art=ProjectileArt(b.explosive,b.hostile);
        const RECT source{art.left,art.top,art.right,art.bottom};
        SourceSprite(r,art.texture,source,b.x-camera-art.width*0.5F,b.y-art.height*0.5F,
            art.width,art.height,b.vx<0);
    }
    for(const GameEffect& effect:g.Effects())
    {
        const float progress=1-std::clamp(effect.time/(effect.explosion?0.55F:0.2F),0.0F,1.0F);
        const float size=effect.explosion?75+progress*85:30+progress*20;
        const DWORD alpha=static_cast<DWORD>((1-progress)*255)<<24;
        Sprite(r,effect.explosion?GameTexture::Explosion:GameTexture::Impact,
            effect.x-camera-size/2,effect.y-size/2,size,size,false,alpha|0x00FFFFFF);
    }
    g.Stage().RenderForeground(r);

    Panel(r,16,14,382,66,0xCB14212B);
    Text(titleFont,30,21,"DUSTFIRE OPS");
    Text(font,31,55,g.Stage().SectionName(player.X()),0xFFE0DDC9);
    Panel(r,624,16,320,58,0xC014212B);
    Text(font,638,24,g.State()==PlayState::Clear?"OBJECTIVE / COMPLETE":g.BossDead()?"OBJECTIVE / REACH THE FLAG":"OBJECTIVE / DESTROY THE ARMORED BOSS");
    const int progress=g.State()==PlayState::Clear?100:std::clamp(static_cast<int>((player.X()-GameConfig::StartX)/(4880-GameConfig::StartX)*100),0,99);
    g2_FontDrawText(font,VEC2i{638,48},0xFFDFD8BD,"PROGRESS %d%%",progress);
    for(const Enemy& e:g.Enemies()) if(e.kind==EnemyKind::Boss && e.hp>0 && player.X()>3940)
    {
        Text(font,300,90,"ARMORED COMMANDER");
        Sprite(r,GameTexture::BossBarEmpty,300,113,360,12);
        const int texture=r.Texture(GameTexture::BossBar);
        const float ratio=static_cast<float>(e.hp)/e.maxHp;
        const RECT source{0,0,static_cast<LONG>(g2_TextureWidth(texture)*ratio),g2_TextureHeight(texture)};
        const VEC2 position(300,113),scale(360.0F/g2_TextureWidth(texture),12.0F/g2_TextureHeight(texture));
        if(source.right>0) g2_Draw2D(texture,&source,&position,&scale);
    }
    Panel(r,0,537,960,63,0xF0192229);
    SourceSprite(r,GameTexture::PlayerAtlas,RECT{118,28,207,126},14,544,44,44);
    for(int i=0;i<3;++i) Sprite(r,GameTexture::HealthIcon,68+i*26.0F,550,24,24,false,i<g.Health()?0xFFFFFFFF:0x44555555);
    Sprite(r,GameTexture::GrenadeIcon,166,547,28,28);
    g2_FontDrawText(font,VEC2i{201,552},0xFFFFE5B7,"x %d",g.Grenades());
    g2_FontDrawText(font,VEC2i{261,550},0xFFFFE5B7,"SCORE %06d   %02d:%02d",g.Score(),static_cast<int>(g.Time())/60,static_cast<int>(g.Time())%60);
    Text(font,513,546,"A/D MOVE   SPACE JUMP   S CROUCH",0xFFE1DBC8);
    Text(font,513,571,"J FIRE   K GRENADE   P / ESC PAUSE",0xFFE1DBC8);

    if(g.State()!=PlayState::Playing)
    {
        Panel(r,0,0,960,537,0x81101B25);
        Panel(r,196,136,568,314,0xEF15232D);
        const bool title=g.State()==PlayState::Title;
        const char* heading=title?"DUSTFIRE OPS":g.State()==PlayState::Paused?"MISSION PAUSED":
            g.State()==PlayState::Clear?"MISSION COMPLETE":"MISSION FAILED";
        Text(largeFont,title?282:255,167,heading);
        if(title)
        {
            Text(font,278,231,"ONE SOLDIER. ONE OUTPOST. ONE MISSION.",0xFFD0CEBA);
            Text(font,271,274,"Cross the desert and destroy the armored commander.");
            Text(font,292,302,"Collect supplies. Dodge fire. Reach the final flag.");
            Text(titleFont,330,373,"ENTER TO DEPLOY");
        }
        else if(g.State()==PlayState::Paused)
        {
            Text(font,311,269,"Take a breath. Your mission is waiting.");
            Text(titleFont,306,367,"P / ESC TO CONTINUE");
        }
        else
        {
            g2_FontDrawText(titleFont,VEC2i{313,246},0xFFFFE5B7,"SCORE  %06d",g.Score());
            g2_FontDrawText(font,VEC2i{313,286},0xFFD0CEBA,"TIME %02d:%02d     TARGETS %d / 10",static_cast<int>(g.Time())/60,static_cast<int>(g.Time())%60,g.Kills());
            Text(font,274,327,g.State()==PlayState::Clear?"Communications base secured. Good work, soldier.":"Crouch under high shots. Jump over low fire. Collect health.");
            Text(titleFont,305,381,"ENTER / R TO RESTART");
        }
    }
}
