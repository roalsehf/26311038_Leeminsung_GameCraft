#include "GameSession.h"
#include <cmath>

namespace
{
    float Decay(float value,float dt) { return (std::max)(0.0F,value-dt); }

    Projectile AimedRound(Combat::Point origin,Combat::Point target,float speed,float spread=0)
    {
        const float angle=std::atan2(target.y-origin.y,target.x-origin.x)+spread;
        return {ProjectileType::EnemyRound,origin.x,origin.y,std::cos(angle)*speed,std::sin(angle)*speed,3.1F};
    }

    Projectile MortarRound(Combat::Point origin,float targetX)
    {
        const float flight=Combat::MortarFlight;
        const float vy=(GameConfig::GroundY-origin.y-0.5F*Combat::Gravity*flight*flight)/flight;
        return {ProjectileType::Mortar,origin.x,origin.y,(targetX-origin.x)/flight,vy,flight,targetX};
    }

    float GroundContact(Combat::Point from,Combat::Point to)
    {
        const float ground=GameConfig::GroundY;
        if(from.y>=ground) return 0;
        if(to.y<ground||to.y<=from.y) return Combat::Miss;
        return (ground-from.y)/(to.y-from.y);
    }

    float PlatformContact(Combat::Point from,Combat::Point to,const std::vector<StagePlatform>& platforms)
    {
        if(to.y<=from.y) return Combat::Miss;
        float first=Combat::Miss;
        for(const auto& platform:platforms)
        {
            if(from.y>platform.top||to.y<platform.top) continue;
            const float contact=(platform.top-from.y)/(to.y-from.y);
            const float x=Combat::Interpolate(from,to,contact).x;
            if(x>=platform.left&&x<=platform.left+platform.width) first=(std::min)(first,contact);
        }
        return first;
    }
}

Combat::Box GameSession::PlayerBox() const
{
    return Combat::Body({m_player.X(),m_player.Y()},21,Crouching()?54.0F:108.0F);
}

Combat::Box GameSession::EnemyBox(const Enemy& e) const
{
    const Combat::Point feet{e.x,e.y};
    switch(e.kind)
    {
    case EnemyKind::Drone: return Combat::Body(feet,49,58);
    case EnemyKind::Turret: return Combat::Body(feet,44,108);
    case EnemyKind::Boss:
        if(m_stage.Id()==StageId::Depot) return Combat::Body(feet,97,145);
        if(m_stage.Id()==StageId::Fortress) return Combat::Body(feet,123,184);
        return Combat::Body(feet,113,150);
    default: return Combat::Body(feet,28,110);
    }
}

void GameSession::DamagePlayer()
{
    if(m_action.invulnerable>0||m_state!=PlayState::Playing) return;
    --m_run.health;
    m_action.invulnerable=1.35F;
    m_action.hurtFlash=0.19F;
    m_sounds.push_back(GameSound::Hurt);
    AddEffect(EffectKind::Impact,{m_player.X(),m_player.Y()-65},0.20F);
    if(m_run.health<=0)
    {
        m_state=PlayState::GameOver;
        m_action.flash=m_action.aim=m_action.throwing=0;
    }
}

void GameSession::DamageEnemy(Enemy& e,int damage)
{
    if(e.hp<=0) return;
    if(e.kind==EnemyKind::Boss) m_bossEngaged=true;
    e.hp=(std::max)(0,e.hp-damage);
    e.hurt=0.12F;
    if(e.hp>0) return;

    e.attack=EnemyAttack{};
    e.deathRemaining=e.kind==EnemyKind::Soldier?0.55F:0.9F;
    if(e.kind==EnemyKind::Boss) m_bossDefeated=true;
    ++m_run.kills;
    m_run.score+=e.kind==EnemyKind::Boss?2000:e.kind==EnemyKind::Turret?300:e.kind==EnemyKind::Drone?250:100;
    const bool large=e.kind!=EnemyKind::Soldier;
    AddEffect(large?EffectKind::Explosion:EffectKind::Impact,{e.x,e.y-48},large?0.65F:0.30F);
    if(large) m_sounds.push_back(GameSound::Explosion);
}

void GameSession::Explode(Combat::Point at,const Projectile& p)
{
    AddEffect(EffectKind::Explosion,at,0.55F);
    m_sounds.push_back(GameSound::Explosion);
    if(p.Hostile())
    {
        if(Combat::InsideBlast(at,Combat::MortarRadius,PlayerBox())) DamagePlayer();
    }
    else for(auto& enemy:m_enemies)
        if(enemy.hp>0&&Combat::InsideBlast(at,Combat::GrenadeRadius,EnemyBox(enemy)))
            DamageEnemy(enemy,p.Damage());
}

void GameSession::BeginAttack(Enemy& e)
{
    EnemyAttack a;
    a.facing=e.facing;
    const float f=static_cast<float>(a.facing);
    a.target={m_player.X(),m_player.Y()-(Crouching()?30.0F:63.0F)};
    a.phaseTwo=e.hp*2<e.maxHp;
    if(e.kind==EnemyKind::Soldier||e.kind==EnemyKind::Turret)
    {
        a.kind=AttackKind::Rifle;
        a.origin={e.x+f*35,e.y-72};
        a.duration=e.kind==EnemyKind::Soldier?0.34F:0.42F;
    }
    else if(e.kind==EnemyKind::Drone)
    {
        a.kind=AttackKind::Aimed;
        a.origin={e.x+f*22,e.y-9};
        a.duration=0.42F;
    }
    else if(m_stage.Id()==StageId::Depot)
    {
        a.kind=AttackKind::Siege;
        a.origin={e.x+f*68,e.y-79};
        a.duration=a.phaseTwo?0.54F:0.62F;
    }
    else
    {
        const bool fortress=m_stage.Id()==StageId::Fortress;
        if((fortress&&e.volley%3==0)||(!fortress&&e.volley%3==1))
        {
            a.kind=AttackKind::Mortar;
            a.origin={e.x+f*(fortress?63.0F:50.0F),e.y-(fortress?125.0F:95.0F)};
            a.target={std::clamp(m_player.X(),90.0F,m_stage.Data().width-90),GameConfig::GroundY};
            a.duration=0.68F;
        }
        else if((fortress&&e.volley%3==1)||(!fortress&&a.phaseTwo))
        {
            a.kind=AttackKind::Sweep;
            a.origin={e.x+f*(fortress?88.0F:41.0F),e.y-29};
            a.duration=0.53F;
        }
        else
        {
            a.kind=fortress?AttackKind::Fan:AttackKind::Cannon;
            a.origin={e.x+f*(fortress?86.0F:46.0F),e.y-(fortress?101.0F:72.0F)};
            a.duration=fortress?0.57F:0.44F;
        }
    }
    a.remaining=a.duration;
    PlanVolley(e,a);
    e.attack=a;
}

void GameSession::PlanVolley(const Enemy& e,EnemyAttack& a) const
{
    const float f=static_cast<float>(a.facing);
    const bool fortress=m_stage.Id()==StageId::Fortress;
    const auto horizontal=[&](float y,float speed)
    { a.Queue({ProjectileType::EnemyRound,a.origin.x,y,f*speed,0,3.1F}); };

    switch(a.kind)
    {
    case AttackKind::Rifle:
        horizontal(a.origin.y,e.kind==EnemyKind::Turret?244.0F:232.0F);
        a.recovery=e.kind==EnemyKind::Turret?1.96F:1.81F;
        break;
    case AttackKind::Aimed:
        a.Queue(AimedRound(a.origin,a.target,208));
        a.recovery=fortress?1.88F:2.23F;
        break;
    case AttackKind::Siege:
        a.Queue(AimedRound(a.origin,a.target,248));
        a.Queue(AimedRound({a.origin.x,a.origin.y-23},a.target,228,(e.volley+1)%2?0.17F:-0.17F));
        a.recovery=a.phaseTwo?1.29F:1.49F;
        break;
    case AttackKind::Mortar:
        a.Queue(MortarRound(a.origin,a.target.x));
        if(fortress&&a.phaseTwo)
        {
            const float extra=std::clamp(a.target.x-f*104,90.0F,m_stage.Data().width-90);
            a.Queue(MortarRound({a.origin.x,a.origin.y-12},extra));
        }
        a.recovery=fortress?(a.phaseTwo?0.97F:1.16F):1.15F;
        break;
    case AttackKind::Sweep:
        horizontal(a.origin.y,fortress?258.0F:302.0F);
        if(fortress) horizontal(a.origin.y-47,306);
        a.recovery=fortress?(a.phaseTwo?0.98F:1.22F):1.05F;
        break;
    case AttackKind::Fan:
        a.Queue(AimedRound(a.origin,a.target,280));
        if(a.phaseTwo)
        {
            a.Queue(AimedRound(a.origin,a.target,253,-0.20F));
            a.Queue(AimedRound(a.origin,a.target,253,0.20F));
        }
        a.recovery=a.phaseTwo?0.86F:1.10F;
        break;
    case AttackKind::Cannon:
        horizontal(a.origin.y,280);
        a.recovery=1.24F;
        break;
    default: break;
    }
}

void GameSession::ReleaseAttack(Enemy& e)
{
    const auto& a=e.attack;
    m_projectiles.insert(m_projectiles.end(),a.shots.begin(),a.shots.begin()+a.shotCount);
    e.fireTimer=a.recovery;
    e.attack=EnemyAttack{};
    e.flash=0.13F;
    ++e.volley;
    if(e.kind==EnemyKind::Boss) m_sounds.push_back(GameSound::Shot);
}

void GameSession::UpdateEnemy(Enemy& e,float dt)
{
    if(e.hp<=0) { e.deathRemaining-=dt; return; }
    e.hurt=Decay(e.hurt,dt);
    e.flash=Decay(e.flash,dt);
    const float distance=std::abs(e.x-m_player.X());
    if(distance<850) e.phase+=dt;
    if(!e.attack.Active()&&e.flash<=0)
    {
        if(e.kind==EnemyKind::Soldier&&distance>240) e.x=e.home+std::sin(e.phase*1.3F)*55;
        if(e.kind==EnemyKind::Drone)
        {
            e.x=e.home+std::sin(e.phase*1.65F)*42;
            e.y=e.homeFeet+std::sin(e.phase*2.6F)*16;
        }
        e.facing=m_player.X()<e.x?-1:1;
    }

    const auto box=EnemyBox(e);
    const float camera=m_stage.CameraX();
    const bool fullyVisible=box.left>=camera+10 && box.right<=camera+GameConfig::ScreenWidth-10;
    const bool canAttack=fullyVisible&&distance<(e.kind==EnemyKind::Drone?530.0F:730.0F);
    if(!canAttack)
    {
        if(e.attack.Active()) { e.attack=EnemyAttack{}; e.fireTimer=0.26F; }
    }
    else
    {
        if(e.kind==EnemyKind::Boss) m_bossEngaged=true;
        if(e.attack.Active())
        {
            e.attack.remaining-=dt;
            if(e.attack.remaining<=0) ReleaseAttack(e);
        }
        else
        {
            e.fireTimer-=dt;
            if(e.fireTimer<=0) BeginAttack(e);
        }
    }

    // 공성포 접촉 피해 제외
    if(e.kind==EnemyKind::Boss&&m_stage.Id()==StageId::Depot) return;
    Combat::Box contact=box;
    contact.top+=12;
    contact.left+=7;
    contact.right-=7;
    if(Combat::Overlaps(contact,PlayerBox())) DamagePlayer();
}

void GameSession::UpdateProjectiles(float dt)
{
    for(auto& p:m_projectiles)
    {
        if(p.life<=0) continue;
        const float span=(std::min)(p.life,dt);
        const float gravity=p.Explosive()?Combat::Gravity:0.0F;
        const Combat::Point from{p.x,p.y};
        const Combat::Point to{p.x+p.vx*span,p.y+p.vy*span+0.5F*gravity*span*span};
        p.vy+=gravity*span;
        p.life-=span;

        float impact=GroundContact(from,to);
        // 수류탄 발판 충돌
        if(p.type==ProjectileType::Grenade)
            impact=(std::min)(impact,PlatformContact(from,to,m_stage.Platforms()));

        Enemy* target=nullptr;
        bool playerHit=false;
        if(p.type==ProjectileType::EnemyRound)
        {
            const float hit=Combat::Sweep(from,to,PlayerBox());
            if(hit<impact) { impact=hit; playerHit=true; }
        }
        else if(!p.Hostile())
        {
            for(auto& enemy:m_enemies) if(enemy.hp>0)
            {
                const float hit=Combat::Sweep(from,to,EnemyBox(enemy));
                if(hit<impact) { impact=hit; target=&enemy; }
            }
        }

        const bool touched=impact<=1;
        auto end=touched?Combat::Interpolate(from,to,impact):to;
        if(p.type==ProjectileType::Mortar && (touched||p.life<=0))
            end={p.landingX,GameConfig::GroundY};
        else if(touched && !target && !playerHit)
            end.y=(std::min)(end.y,GameConfig::GroundY);
        p.x=end.x;
        p.y=end.y;
        if(!touched&&p.life>0) continue;
        if(p.Explosive()) Explode(end,p);
        else if(touched)
        {
            if(playerHit) DamagePlayer();
            if(target) DamageEnemy(*target,p.Damage());
            AddEffect(EffectKind::Impact,end,0.17F);
        }
        p.life=0;
        if(m_state!=PlayState::Playing) break;
    }
}
