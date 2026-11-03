#include "GameSession.h"
#include <algorithm>
#include <cmath>

namespace
{
    bool Hits(float x,float y,float cx,float feet,float halfWidth,float height)
    { return std::abs(x-cx)<halfWidth && y>feet-height && y<feet+4; }
}

void GameSession::Restart()
{
    m_player=GamePlayer{}; m_stage=GameStage{}; m_state=PlayState::Playing;
    m_health=3; m_grenades=3; m_score=0; m_kills=0; m_time=0;
    m_fireCooldown=m_grenadeCooldown=m_invulnerable=m_muzzle=0; m_crouch=false;
    m_aimTime=m_throwTime=0; m_bossDefeated=false;
    m_projectiles.clear(); m_effects.clear(); m_pickups.clear(); m_enemies.clear(); m_sounds.clear();
    const float soldiers[]={640,1160,1880,2440,3180,3640};
    for(float x:soldiers) m_enemies.push_back({EnemyKind::Soldier,x,GameConfig::GroundY,x,1.2F,0,0,0,2,2});
    for(float x:{1510.0F,2780.0F,3980.0F})
        m_enemies.push_back({EnemyKind::Turret,x,GameConfig::GroundY,x,1.6F,0,0,0,5,5});
    m_enemies.push_back({EnemyKind::Boss,4630,GameConfig::GroundY,4630,1.5F,0,0,0,36,36});
    m_pickups={{1010,GameConfig::GroundY,false},{2050,GameConfig::GroundY,true},
        {2670,GameConfig::GroundY,false},{3500,GameConfig::GroundY,true},{4200,GameConfig::GroundY,false}};
}

bool GameSession::BossDead() const
{
    return m_bossDefeated;
}

PlayerPose GameSession::Pose() const
{
    if(m_state==PlayState::GameOver) return PlayerPose::Dead;
    if(!m_player.IsGrounded()) return PlayerPose::Jump;
    if(m_crouch) return Aiming()?PlayerPose::CrouchShoot:PlayerPose::Crouch;
    if(Throwing()) return PlayerPose::Throw;
    if(m_player.IsMoving()) return Aiming()?PlayerPose::RunShoot:PlayerPose::Run;
    return Aiming()?PlayerPose::Shoot:PlayerPose::Idle;
}

float GameSession::WeaponX() const
{
    const int frame=PlayerArt::Frame(Pose(),m_player.RunFrame(),m_time);
    return m_player.X()+(m_player.IsFacingLeft()?-1.0F:1.0F)*PlayerArt::WeaponOffsetX(frame);
}
float GameSession::WeaponY() const
{
    const int frame=PlayerArt::Frame(Pose(),m_player.RunFrame(),m_time);
    return m_player.Y()-PlayerArt::WeaponHeight(frame);
}

void GameSession::DamagePlayer()
{
    if(m_invulnerable>0 || m_state!=PlayState::Playing) return;
    --m_health; m_invulnerable=1.35F; m_sounds.push_back(GameSound::Hurt);
    m_effects.push_back({m_player.X(),m_player.Y()-65,0.2F,false});
    if(m_health<=0) m_state=PlayState::GameOver;
}

void GameSession::DamageEnemy(Enemy& enemy,int damage)
{
    if(enemy.hp<=0) return;
    enemy.hp-=damage; enemy.hurt=0.12F;
    if(enemy.hp<=0)
    {
        enemy.deathRemaining=enemy.kind==EnemyKind::Soldier?0.55F:0.9F;
        if(enemy.kind==EnemyKind::Boss) m_bossDefeated=true;
        ++m_kills; m_score+=enemy.kind==EnemyKind::Boss?2000:enemy.kind==EnemyKind::Turret?300:100;
        const bool large=enemy.kind!=EnemyKind::Soldier;
        m_effects.push_back({enemy.x,enemy.y-48,large?0.65F:0.3F,large});
        if(large) m_sounds.push_back(GameSound::Explosion);
    }
}

void GameSession::Explode(float x,float y,bool hostile)
{
    m_effects.push_back({x,y,0.55F,true}); m_sounds.push_back(GameSound::Explosion);
    if(hostile)
    {
        if(std::hypot(m_player.X()-x,m_player.Y()-45-y)<90) DamagePlayer();
    }
    else for(Enemy& enemy:m_enemies)
        if(std::hypot(enemy.x-x,enemy.y-50-y)<155) DamageEnemy(enemy,8);
}

void GameSession::Update(float dt,const GameInput& input)
{
    m_sounds.clear();
    if(m_state==PlayState::Title) { if(input.start) Restart(); return; }
    if(m_state==PlayState::GameOver || m_state==PlayState::Clear)
    { if(input.retry || input.start) Restart(); return; }
    if(input.pause) { m_state=m_state==PlayState::Paused?PlayState::Playing:PlayState::Paused; return; }
    if(m_state==PlayState::Paused) return;
    dt=std::clamp(dt,0.0F,0.05F);
    if(dt<=0) return;
    m_time+=dt; m_fireCooldown-=dt; m_grenadeCooldown-=dt;
    m_invulnerable=(std::max)(0.0F,m_invulnerable-dt); m_muzzle-=dt;
    m_aimTime=(std::max)(0.0F,m_aimTime-dt); m_throwTime=(std::max)(0.0F,m_throwTime-dt);
    m_crouch=input.crouch && m_player.IsGrounded();
    m_player.Update(dt,input.left&&!m_crouch,input.right&&!m_crouch,input.jump,m_stage.Platforms());
    m_crouch=input.crouch && m_player.IsGrounded();
    if(!BossDead() && m_player.X()>4800) m_player.RestrictX(64,4800);
    m_stage.Update(dt,m_player.X());
    const float facing=m_player.IsFacingLeft()?-1.0F:1.0F;
    if(input.jump && !m_player.IsGrounded()) m_sounds.push_back(GameSound::Jump);
    if(input.grenade && m_grenades>0 && m_grenadeCooldown<=0)
    {
        --m_grenades; m_grenadeCooldown=0.7F; m_throwTime=0.22F; m_aimTime=0; m_muzzle=0;
        m_projectiles.push_back({m_player.X()+facing*48,m_player.Y()-(m_crouch?46.0F:85.0F),
            facing*370,-350,1.15F,false,true,8});
    }
    if(input.fire && m_throwTime<=0)
    {
        m_aimTime=0.18F;
        if(m_fireCooldown<=0)
        {
            m_fireCooldown=0.15F; m_muzzle=0.055F;
            m_projectiles.push_back({WeaponX(),WeaponY(),facing*850,0,0.9F,false,false,1});
            m_sounds.push_back(GameSound::Shot);
        }
    }
    for(Enemy& enemy:m_enemies)
    {
        if(enemy.hp<=0) { enemy.deathRemaining-=dt; continue; }
        enemy.hurt=(std::max)(0.0F,enemy.hurt-dt); enemy.phase+=dt; enemy.windup-=dt;
        if(std::abs(enemy.x-m_player.X())>730) continue;
        const float direction=m_player.X()<enemy.x?-1.0F:1.0F;
        enemy.facing=direction<0?-1:1;
        if(enemy.kind==EnemyKind::Soldier && std::abs(enemy.x-m_player.X())>240)
            enemy.x=enemy.home+std::sin(enemy.phase*1.3F)*55;
        enemy.fireTimer-=dt;
        if(enemy.fireTimer<0.4F) enemy.windup=0.4F;
        if(enemy.fireTimer<=0)
        {
            enemy.fireTimer=enemy.kind==EnemyKind::Boss?1.55F:enemy.kind==EnemyKind::Turret?2.4F:2.1F;
            const float speed=enemy.kind==EnemyKind::Boss?280.0F:230.0F;
            const float shotY=enemy.y-(enemy.kind==EnemyKind::Boss?64.0F:72.0F);
            m_projectiles.push_back({enemy.x+direction*35,shotY,direction*speed,0,3.1F,true,false,1});
            if(enemy.kind==EnemyKind::Boss)
            {
                m_projectiles.push_back({enemy.x-50,enemy.y-95,-245.0F,-190.0F,1.3F,true,true,1});
                if(enemy.hp<enemy.maxHp/2)
                    m_projectiles.push_back({enemy.x-40,enemy.y-34,-320,0,2.8F,true,false,1});
            }
        }
        if(std::abs(enemy.x-m_player.X())<(enemy.kind==EnemyKind::Boss?96:34)
            && m_player.Y()>enemy.y-90) DamagePlayer();
    }
    for(Projectile& bullet:m_projectiles)
    {
        if(bullet.life<=0) continue;
        bullet.life-=dt;
        if(bullet.explosive) bullet.vy+=680*dt;
        bullet.x+=bullet.vx*dt; bullet.y+=bullet.vy*dt;
        if(bullet.explosive && (bullet.life<=0 || bullet.y>=GameConfig::GroundY-8))
        { Explode(bullet.x,(std::min)(bullet.y,GameConfig::GroundY-10),bullet.hostile); bullet.life=-1; continue; }
        if(bullet.explosive) continue;
        if(bullet.hostile)
        {
            if(Hits(bullet.x,bullet.y,m_player.X(),m_player.Y(),21.0F,m_crouch?54.0F:108.0F))
            { bullet.life=-1; DamagePlayer(); }
        }
        else for(Enemy& enemy:m_enemies)
            if(enemy.hp>0 && Hits(bullet.x,bullet.y,enemy.x,enemy.y,
                enemy.kind==EnemyKind::Boss?110.0F:enemy.kind==EnemyKind::Turret?44.0F:28.0F,
                enemy.kind==EnemyKind::Boss?148.0F:110.0F))
            {
                DamageEnemy(enemy,bullet.damage); bullet.life=-1;
                m_effects.push_back({bullet.x,bullet.y,0.15F,false}); break;
            }
    }
    m_projectiles.erase(std::remove_if(m_projectiles.begin(),m_projectiles.end(),[](const Projectile& b)
        { return b.life<=0 || b.x<0 || b.x>GameConfig::WorldWidth || b.y>650; }),m_projectiles.end());
    m_enemies.erase(std::remove_if(m_enemies.begin(),m_enemies.end(),[](const Enemy& enemy)
        { return enemy.hp<=0 && enemy.deathRemaining<=0; }),m_enemies.end());
    for(GameEffect& effect:m_effects) effect.time-=dt;
    m_effects.erase(std::remove_if(m_effects.begin(),m_effects.end(),[](const GameEffect& e)
        { return e.time<=0; }),m_effects.end());
    for(Pickup& pickup:m_pickups)
        if(!pickup.taken && std::abs(pickup.x-m_player.X())<40 && std::abs(pickup.y-m_player.Y())<55
            && (pickup.grenade || m_health<3))
        {
            pickup.taken=true;
            if(pickup.grenade) m_grenades+=2; else m_health=(std::min)(3,m_health+1);
            m_score+=50; m_sounds.push_back(GameSound::Pickup);
        }
    if(m_state==PlayState::Playing && BossDead() && m_stage.AtEnd(m_player.X()))
    { m_state=PlayState::Clear; m_score+=500; m_sounds.push_back(GameSound::Clear); }
}
