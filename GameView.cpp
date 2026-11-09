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
        float x,float y,float width,float height,bool flip=false,DWORD color=0xFFFFFFFF)
    {
        if(x+width<0 || x>GameConfig::ScreenWidth) return;
        const VEC2 position(x,y),scale((flip?-1.0F:1.0F)*width/(source.right-source.left),
            height/(source.bottom-source.top));
        g2_Draw2D(r.Texture(texture),&source,&position,&scale,nullptr,0,color);
    }
    void DirectedSprite(const GameResources& r,GameTexture texture,const RECT& source,
        float x,float y,float w,float h,float dx,float dy,DWORD color=0xFFFFFFFF)
    {
        if(x+w<0||x-w>GameConfig::ScreenWidth||y+w<0||y-w>GameConfig::ScreenHeight) return;
        if(std::abs(dy)<0.01F)
        { SourceSprite(r,texture,source,x-w*0.5F,y-h*0.5F,w,h,dx<0,color); return; }
        // 탄환 방향 표시
        const float length=std::hypot(dx,dy),ux=dx/length,uy=dy/length;
        const bool flash=texture==GameTexture::Muzzle;
        constexpr int parts=7;
        const float step=w/parts;
        for(int i=0;i<parts;++i)
        {
            const float offset=(i+0.5F-parts*0.5F)*step;
            const float profile=flash?1.0F-std::abs(i-3)*0.23F:0.24F+i*0.095F;
            const float cross=h*profile*(flash?0.77F:0.92F);
            const float pw=std::abs(ux)*step+std::abs(uy)*cross+1.0F;
            const float ph=std::abs(uy)*step+std::abs(ux)*cross+1.0F;
            const DWORD tail=flash?0xA6EB7D26:0x80DA7926;
            const DWORD core=flash?0xFFFFD16B:(color==0xFFFFFFFF?0xFFFFD66E:color);
            const DWORD tint=i<2?tail:i==parts-2?0xFFFFF4C7:core;
            Panel(r,std::floor(x+ux*offset-pw*0.5F),std::floor(y+uy*offset-ph*0.5F),pw,ph,tint);
        }
    }
    void Text(int font,int x,int y,const char* text,DWORD color=0xFFFFE5B7)
    {
        g2_FontDrawText(font,VEC2i{x+1,y+2},0xE0101720,"%s",text);
        g2_FontDrawText(font,VEC2i{x,y},color,"%s",text);
    }

    const char* AttackHint(AttackKind kind)
    {
        switch(kind)
        {
        case AttackKind::Mortar: return "박격포 · 표시된 구역에서 벗어나세요";
        case AttackKind::Sweep: return "낮은 탄막 · 점프로 피하세요";
        case AttackKind::Cannon: return "높은 포격 · 앉아서 피하세요";
        case AttackKind::Siege: return "조준 포격 · 이동해서 피하세요";
        case AttackKind::Fan: return "연속 포격 · 이동해서 피하세요";
        default: return "";
        }
    }

    void DangerZone(const GameResources& r,float screenX,float time)
    {
        constexpr float width=Combat::MortarRadius*2;
        const float x=screenX-Combat::MortarRadius;
        if(x+width<0||x>GameConfig::ScreenWidth) return;
        const DWORD alpha=static_cast<DWORD>(105+37*(1+std::sin(time*14)))<<24;
        const float y=GameConfig::GroundY;
        Panel(r,x,y-17,width,17,0x55F05832);
        Panel(r,x,y-3,width,3,alpha|0x00FF9161);
        Panel(r,x,y-17,3,14,alpha|0x00FF9161);
        Panel(r,x+width-3,y-17,3,14,alpha|0x00FF9161);
        for(int i=0;i<6;++i) Panel(r,x+12+26.0F*i,y-12,9,4,alpha|0x00FFA573);
    }

    void AimGuide(const GameResources& r,Combat::Point from,Combat::Point to,float camera)
    {
        const float length=std::hypot(to.x-from.x,to.y-from.y);
        const int count=std::clamp(static_cast<int>(length/17),1,45);
        for(int i=1;i<=count;++i)
        {
            const auto p=Combat::Interpolate(from,to,static_cast<float>(i)/count);
            if(p.x-camera<0||p.x-camera>GameConfig::ScreenWidth||p.y<90||p.y>GameConfig::GroundY) continue;
            Panel(r,p.x-camera-2,p.y-1,4,2,0xC0FF9264);
        }
    }

    void DrawWarnings(const GameSession& g,const GameResources& r)
    {
        const float camera=g.Stage().CameraX();
        for(const auto& e:g.Enemies()) if(e.hp>0&&e.attack.Active())
        {
            const auto& a=e.attack;
            if(a.kind==AttackKind::Rifle) continue;
            for(std::size_t i=0;i<a.shotCount;++i)
            {
                const auto& shot=a.shots[i];
                if(shot.type==ProjectileType::Mortar)
                    DangerZone(r,shot.landingX-camera,g.Time());
                else
                {
                    // 탄환 경로 예고
                    AimGuide(r,{shot.x,shot.y},{shot.x+shot.vx*shot.life,shot.y+shot.vy*shot.life},camera);
                }
            }
        }
        for(const auto& p:g.Projectiles()) if(p.type==ProjectileType::Mortar)
            DangerZone(r,p.landingX-camera,g.Time());
    }

    void DrawEffect(const GameEffect& effect,const GameSession& g,const GameResources& r,int font)
    {
        const float x=effect.x-g.Stage().CameraX();
        if(x<-100||x>GameConfig::ScreenWidth+100) return;
        const float progress=effect.Progress();
        const DWORD alpha=static_cast<DWORD>((1-progress)*255)<<24;
        if(effect.kind==EffectKind::Dust)
        {
            const float w=25+25*progress,h=11+13*progress;
            const DWORD tint=g.Stage().Id()==StageId::Desert?0x00FFFFFF:0x00BEC5D0;
            Sprite(r,GameTexture::Dust,x-w/2,effect.y-h,w,h,false,
                (static_cast<DWORD>((1-progress)*136)<<24)|tint);
            return;
        }
        if(effect.kind==EffectKind::Health||effect.kind==EffectKind::Grenades||effect.kind==EffectKind::MachineGun)
        {
            const char* label=effect.kind==EffectKind::Health?"체력":effect.kind==EffectKind::Grenades?"수류탄":"기관총 탄약";
            const DWORD tint=effect.kind==EffectKind::Health?0x00FFC28A:0x009EE8D7;
            g2_FontDrawText(font,VEC2i{static_cast<int>(x)-30,static_cast<int>(effect.y-progress*32)},
                alpha|tint,"+%d %s",effect.amount,label);
            return;
        }
        const bool blast=effect.kind==EffectKind::Explosion;
        const float size=blast?75+progress*85:30+progress*20;
        Sprite(r,blast?GameTexture::Explosion:GameTexture::Impact,x-size/2,effect.y-size/2,size,size,false,alpha|0x00FFFFFF);
        for(int i=0;i<(blast?7:3);++i)
        {
            const float angle=i*2.39996F+effect.x*0.043F;
            const float distance=(blast?59.0F:24.0F)*progress;
            Panel(r,x+std::cos(angle)*distance,effect.y+std::sin(angle)*distance,blast?4.0F:3.0F,2,alpha|0x00FFC271);
        }
    }
}

static void DrawWorld(const GameSession& g,const GameResources& r,int font)
{
    const float camera=g.Stage().CameraX();
    const auto& stage=g.Stage().Data();
    g.Stage().RenderBackdrop(r);
    for(int i=0;i<19;++i)
    {
        const float x=std::fmod(i*139.0F+g.Time()*(14+i%5)*2+9600,960.0F);
        const float y=205+std::fmod(i*61.0F+std::sin(g.Time()*1.5F+i)*19+640,302.0F);
        Panel(r,x,y,i%3==0?4.0F:2.0F,2,0x38000000|(stage.accent&0x00FFFFFF));
    }
    DrawWarnings(g,r);
    if(!g.BossDead())
    {
        Sprite(r,GameTexture::MetalDoor,stage.gate-camera+17,361,87,159);
        Sprite(r,GameTexture::WarningLight,stage.gate-camera+39,342,40,35,false,
            std::sin(g.Time()*9)>0?0xFFFFFFFF:0xFF99716A);
    }
    for(const Pickup& p:g.Pickups()) if(!p.taken)
    {
        const auto item=p.kind==SupplyKind::Grenades?GameTexture::GrenadePickup:
            p.kind==SupplyKind::MachineGun?GameTexture::MachineGunPickup:GameTexture::HealthKit;
        const bool gun=p.kind==SupplyKind::MachineGun;
        Sprite(r,item,p.x-camera-(gun?36.0F:24.0F),p.y-53+std::sin(g.Time()*3)*3,gun?72.0F:48.0F,48);
        if(gun && std::abs(p.x-g.Player().X())<500)
            Text(font,static_cast<int>(p.x-camera)-25,static_cast<int>(p.y)-65,"기관총",0xFF8CE7DE);
    }
    for(const Enemy& e:g.Enemies())
    {
        if(e.hp<=0 && e.deathRemaining<=0) continue;
        GameTexture texture=GameTexture::MachineAtlas;
        float width=128,height=128;
        RECT cell{};
        bool sheet=false;
        if(e.kind==EnemyKind::Soldier)
            texture=e.hp<=0?GameTexture::SoldierDeath:e.flash>0?GameTexture::SoldierShoot:
                (e.attack.Active()||std::abs(e.x-g.Player().X())<240)?GameTexture::SoldierIdle:
                (static_cast<int>(e.phase*6)%2?GameTexture::SoldierWalk1:GameTexture::SoldierWalk2);
        else if(e.kind==EnemyKind::Turret)
        { width=120; height=110; texture=e.hp<=0?GameTexture::TurretDestroyed:e.flash>0?GameTexture::TurretShoot:GameTexture::TurretIdle; }
        else if(e.kind==EnemyKind::Drone)
        { width=113; height=70; cell=e.flash>0?RECT{677,162,1225,486}:RECT{77,162,597,483}; sheet=true; }
        else if(g.Stage().Id()==StageId::Depot)
        { width=218; height=168; cell={27,714,600,1156}; sheet=true; }
        else if(g.Stage().Id()==StageId::Fortress)
        { width=286; height=214; cell={637,707,1244,1160}; sheet=true; }
        else
        { width=256; height=176; texture=e.hp<=0?GameTexture::TankWreck:e.hurt>0?GameTexture::TankDamaged:e.flash>0?GameTexture::TankShoot:GameTexture::TankIdle; }
        const DWORD alpha=e.hp<=0?static_cast<DWORD>(std::clamp(e.deathRemaining/0.25F,0.0F,1.0F)*255):255;
        const DWORD tint=e.hp<=0&&sheet?0x008C8993:e.hurt>0?0x00FFB8A2:
            e.attack.Active()?0x00FFE1AC:0x00FFFFFF;
        const DWORD color=(alpha<<24)|tint;
        const bool flip=e.facing>0;
        const float x=e.x-camera-width*0.5F;
        if(sheet) SourceSprite(r,texture,cell,x,e.y-height,width,height,flip,color);
        else Sprite(r,texture,x,e.y-height,width,height,flip,color);
        if(e.hp>0 && e.attack.Active() && e.x-camera>=4 && e.x-camera<946)
            Text(font,static_cast<int>(e.x-camera)-4,static_cast<int>(e.y-height)-20,"!",0xFFFFBA67);
    }
    const GamePlayer& player=g.Player();
    const DWORD playerAlpha=g.Invulnerability()>0&&g.State()==PlayState::Playing?
        static_cast<DWORD>(210+42*std::sin(g.Time()*27)):255;
    const DWORD playerTint=g.HurtFlash()>0&&g.State()==PlayState::Playing?0x00FFB8A0:0x00FFFFFF;
    player.Render(r,camera,g.VisualFrame(),(playerAlpha<<24)|playerTint);
    if(g.Shooting())
        DirectedSprite(r,GameTexture::Muzzle,RECT{0,9,128,118},g.WeaponX()-camera+g.AimX()*12,
            g.WeaponY()+g.AimY()*12,34,25,g.AimX(),g.AimY());
    for(const Projectile& b:g.Projectiles())
    {
        const ProjectileAppearance art=ProjectileArt(b.Explosive(),b.Hostile());
        const RECT source{art.left,art.top,art.right,art.bottom};
        if(b.type==ProjectileType::Grenade)
            SourceSprite(r,art.texture,source,b.x-camera-art.width*0.5F,b.y-art.height*0.5F,art.width,art.height,b.vx<0);
        else
        {
            const DWORD color=b.Hostile()?0xFFFF9C75:b.Heavy()?0xFFB6FFE9:0xFFFFFFFF;
            DirectedSprite(r,art.texture,source,b.x-camera,b.y,b.Heavy()?40.0F:art.width,
                art.height,b.vx,b.vy,color);
        }
    }
    for(const GameEffect& effect:g.Effects()) DrawEffect(effect,g,r,font);
    g.Stage().RenderForeground(r);
    if(g.State()==PlayState::Playing&&g.HurtFlash()>0)
    {
        const DWORD alpha=static_cast<DWORD>(g.HurtFlash()/0.19F*108)<<24;
        const DWORD color=alpha|0x00ED543A;
        Panel(r,0,0,960,6,color);
        Panel(r,0,0,7,537,color);
        Panel(r,953,0,7,537,color);
    }
}

static void DrawCombatHud(const GameSession& g,const GameResources& r,int font)
{
    if(g.BossEngaged()) for(const Enemy& e:g.Enemies()) if(e.kind==EnemyKind::Boss&&e.hp>0)
    {
        const auto& stage=g.Stage().Data();
        Text(font,510,539,stage.bossName,stage.accent);
        if(e.hp*2<e.maxHp) Text(font,860,539,"2단계",0xFFFFA17E);
        Sprite(r,GameTexture::BossBarEmpty,510,561,418,8);
        const int texture=r.Texture(GameTexture::BossBar);
        const float ratio=static_cast<float>(e.hp)/e.maxHp;
        const RECT source{0,0,static_cast<LONG>(g2_TextureWidth(texture)*ratio),g2_TextureHeight(texture)};
        const VEC2 position(510,561),scale(418.0F/g2_TextureWidth(texture),8.0F/g2_TextureHeight(texture));
        if(source.right>0) g2_Draw2D(texture,&source,&position,&scale);
        if(e.attack.Active()) Text(font,510,572,AttackHint(e.attack.kind),0xFFFFA17E);
        return;
    }

    if(g.BossDead()&&g.State()==PlayState::Playing)
    {
        Text(font,510,546,"보스 격파",0xFFFFD29B);
        Text(font,510,570,"오른쪽 끝의 깃발로 이동하세요",0xFFE1DBC8);
        return;
    }

    Text(font,510,546,"A/D 이동   W 위 조준   Space 점프   P 정지",0xFFE1DBC8);
    Text(font,510,570,"S 앉기·공중 아래 조준   J 사격   K 수류탄",0xFFE1DBC8);
}

static void DrawHud(const GameSession& g,const GameResources& r,int font)
{
    Panel(r,0,537,960,63,0xF0192229);
    SourceSprite(r,GameTexture::PlayerAtlas,RECT{118,28,207,126},14,544,44,44);
    for(int i=0;i<3;++i) Sprite(r,GameTexture::HealthIcon,68+i*26.0F,550,24,24,false,i<g.Health()?0xFFFFFFFF:0x44555555);
    Sprite(r,GameTexture::GrenadeIcon,166,547,28,28);
    g2_FontDrawText(font,VEC2i{201,552},0xFFFFE5B7,"× %d",g.Grenades());
    if(g.MachineGunAmmo()>0)
        g2_FontDrawText(font,VEC2i{258,546},0xFF9BE7DA,"기관총  %d발",g.MachineGunAmmo());
    else Text(font,258,546,"소총 · 무제한",0xFFDFD8BD);
    const int seconds=static_cast<int>(g.Time());
    g2_FontDrawText(font,VEC2i{258,569},0xFFFFE5B7,"점수 %06d   %02d:%02d",g.Score(),seconds/60,seconds%60);
    DrawCombatHud(g,r,font);
}

static void DrawOverlay(const GameSession& g,const GameResources& r,const GameFonts& fonts)
{
    const auto state=g.State();
    if(state==PlayState::Playing) return;

    const auto& stage=g.Stage().Data();
    const int font=fonts.body;
    Panel(r,0,0,960,537,0x81101B25);
    Panel(r,169,128,622,326,0xF016242F);
    Panel(r,169,128,622,4,stage.accent);
    const char* heading=state==PlayState::Title?"더스트파이어 작전":state==PlayState::Paused?"일시정지":
        state==PlayState::Clear?"모든 작전 완료":state==PlayState::StageClear?"스테이지 완료":"작전 실패";
    Text(fonts.heading,218,153,heading,stage.accent);

    if(state==PlayState::Title)
    {
        Text(font,219,222,"보스를 격파하고 오른쪽 끝의 깃발까지 도달하세요.");
        Text(font,219,263,"A/D 또는 ←/→ 이동    Space 점프    J 사격    K 수류탄");
        Text(font,219,291,"W + J 위로 사격    공중에서 S + J 아래로 사격");
        Text(font,219,319,"지상에서 S + J 앉아 사격    S + Space 발판 내려가기");
        Panel(r,218,365,230,46,0xFF314959);
        Text(fonts.subtitle,239,372,"Enter  작전 시작");
        return;
    }
    if(state==PlayState::Paused)
    {
        g2_FontDrawText(font,VEC2i{219,231},0xFFE1DBC8,"현재 위치 · %d단계 %s",g.StageNumber(),stage.name);
        Text(fonts.subtitle,218,372,"P 또는 Esc  계속하기");
        return;
    }

    const int seconds=static_cast<int>(g.Time());
    g2_FontDrawText(fonts.subtitle,VEC2i{219,232},0xFFFFE5B7,"점수  %06d",g.Score());
    g2_FontDrawText(font,VEC2i{219,276},0xFFD0CEBA,"시간 %02d:%02d   이 구간 %d / %d기 격파   누적 %d기",
        seconds/60,seconds%60,g.StageKills(),static_cast<int>(stage.enemies.size()),g.Kills());
    if(state==PlayState::StageClear)
    {
        const auto& next=GameStage(static_cast<StageId>(g.StageNumber())).Data();
        g2_FontDrawText(font,VEC2i{219,316},next.accent,"다음 스테이지 · %s   체력 1 회복",next.name);
        Text(fonts.subtitle,218,372,"Enter  다음 스테이지");
    }
    else if(state==PlayState::Clear)
    {
        Text(font,219,316,"세 스테이지를 모두 돌파했습니다.");
        Text(fonts.subtitle,218,372,"Enter 또는 R  새 작전");
    }
    else
    {
        g2_FontDrawText(font,VEC2i{219,316},stage.accent,"재도전 위치 · %d단계 %s 입구",g.StageNumber(),stage.name);
        Text(fonts.subtitle,218,372,"Enter 또는 R  재도전");
    }
}

bool GameFonts::Load()
{
    constexpr const char* family="Malgun Gothic";
    body=g2_FontCreate(family,17);
    subtitle=g2_FontCreate(family,26);
    heading=g2_FontCreate(family,42);
    return body>=0&&subtitle>=0&&heading>=0;
}

void DrawGame(const GameSession& game,const GameResources& resources,const GameFonts& fonts)
{
    DrawWorld(game,resources,fonts.body);
    DrawHud(game,resources,fonts.body);
    DrawOverlay(game,resources,fonts);
}
