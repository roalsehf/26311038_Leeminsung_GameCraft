#include "GameSession.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    constexpr double SimulationStep=1.0/120.0;
    constexpr double MaxFrameTime=0.1;

    void Tick(float& timer,float dt) { timer=(std::max)(0.0F,timer-dt); }
}

void GameSession::Restart()
{
    m_run=RunStats{};
    LoadStage(StageId::Desert);
}

void GameSession::ResetClock()
{
    m_accumulator=0;
    m_jumpQueued=m_dropQueued=m_grenadeQueued=false;
}

void GameSession::Suspend()
{
    if(m_state==PlayState::Playing) m_state=PlayState::Paused;
    ResetClock();
}

void GameSession::LoadStage(StageId id)
{
    m_player=GamePlayer{};
    m_stage=GameStage{id};
    m_state=PlayState::Playing;
    m_checkpoint=m_run;
    m_action=PlayerAction{};
    m_stageTime=0;
    m_bossDefeated=m_bossEngaged=false;
    ResetClock();

    m_projectiles.clear();
    m_effects.clear();
    m_sounds.clear();
    m_enemies.clear();
    m_pickups.clear();
    m_enemies.reserve(m_stage.Data().enemies.size());
    m_pickups.reserve(m_stage.Data().supplies.size());

    for(const EnemySpawn& spawn:m_stage.Data().enemies)
    {
        const float firstShot=spawn.kind==EnemyKind::Soldier?1.1F:1.35F;
        m_enemies.push_back({spawn.kind,spawn.x,spawn.feet,spawn.x,spawn.feet,spawn.hp,spawn.hp,firstShot});
    }
    for(const SupplySpawn& spawn:m_stage.Data().supplies)
        m_pickups.push_back({spawn.x,spawn.feet,spawn.kind});
}

void GameSession::RetryStage()
{
    m_run=m_checkpoint;
    m_run.health=3;
    m_run.grenades=(std::max)(3,m_run.grenades);
    LoadStage(m_stage.Id());
}

bool GameSession::HandleState(const GameInput& input)
{
    switch(m_state)
    {
    case PlayState::Title:
        if(input.start) Restart();
        return true;
    case PlayState::GameOver:
        if(input.retry||input.start) RetryStage();
        return true;
    case PlayState::Clear:
        if(input.retry||input.start) Restart();
        return true;
    case PlayState::StageClear:
        if(input.start)
        {
            m_run.health=(std::min)(3,m_run.health+1);
            m_run.grenades=(std::max)(2,m_run.grenades);
            LoadStage(static_cast<StageId>(StageNumber()));
        }
        return true;
    case PlayState::Paused:
        if(input.pause) m_state=PlayState::Playing;
        return true;
    case PlayState::Playing:
        if(!input.pause) return false;
        Suspend();
        return true;
    }
    return true;
}

void GameSession::Update(float seconds,const GameInput& input)
{
    m_sounds.clear();
    if(HandleState(input)) return;

    // 짧은 입력 저장
    if(input.jump)
    {
        m_jumpQueued=true;
        m_dropQueued=input.crouch&&!input.aimUp;
    }
    m_grenadeQueued|=input.grenade;
    if(!std::isfinite(seconds)||seconds<=0) return;
    m_accumulator+=std::clamp(static_cast<double>(seconds),0.0,MaxFrameTime);

    while(m_accumulator+1e-8>=SimulationStep && m_state==PlayState::Playing)
    {
        m_accumulator=(std::max)(0.0,m_accumulator-SimulationStep);
        GameInput stepInput=input;
        stepInput.jump=std::exchange(m_jumpQueued,false);
        stepInput.grenade=std::exchange(m_grenadeQueued,false);
        Step(static_cast<float>(SimulationStep),stepInput,std::exchange(m_dropQueued,false));
    }
    if(m_state!=PlayState::Playing) ResetClock();
}

GameMusic GameSession::Music() const
{
    if(m_state!=PlayState::Playing && m_state!=PlayState::Paused) return GameMusic::Silence;
    return BossEngaged()?GameMusic::Boss:GameMusic::Stage;
}

bool GameSession::Shooting() const
{
    return m_action.flash>0 && Aim()==m_action.flashDirection && m_player.IsFacingLeft()==m_action.flashLeft;
}

PlayerPose GameSession::Pose() const
{
    if(m_state==PlayState::GameOver) return PlayerPose::Dead;
    if(m_state==PlayState::StageClear||m_state==PlayState::Clear) return PlayerPose::Idle;
    if(!Throwing() && Aim()==AimDirection::Up) return PlayerPose::AimUp;
    if(!Throwing() && Aim()==AimDirection::Down) return PlayerPose::AimDown;
    if(!m_player.IsGrounded()) return PlayerPose::Jump;
    if(Crouching()) return Aiming()?PlayerPose::CrouchShoot:PlayerPose::Crouch;
    if(Throwing()) return PlayerPose::Throw;
    if(m_player.IsMoving()) return Aiming()?PlayerPose::RunShoot:PlayerPose::Run;
    return Aiming()?PlayerPose::Shoot:PlayerPose::Idle;
}

PlayerArt::FrameSpec GameSession::VisualFrame() const
{
    return PlayerArt::Select(Pose(),m_player.RunFrame(),m_player.IsMoving(),m_player.IsGrounded(),Time());
}

float GameSession::AimX() const
{ return Aim()==AimDirection::Forward?(m_player.IsFacingLeft()?-1.0F:1.0F):0.0F; }

float GameSession::AimY() const
{ return Aim()==AimDirection::Up?-1.0F:Aim()==AimDirection::Down?1.0F:0.0F; }

float GameSession::WeaponX() const
{ return m_player.X()+(m_player.IsFacingLeft()?-1.0F:1.0F)*VisualFrame().WeaponOffsetX(); }

float GameSession::WeaponY() const
{ return m_player.Y()-VisualFrame().WeaponHeight(); }

void GameSession::UpdatePlayer(float dt,const GameInput& input,bool dropRequested)
{
    const bool wasGrounded=m_player.IsGrounded();
    const bool down=input.crouch && !input.aimUp;
    const bool drop=dropRequested && wasGrounded && m_player.Y()<GameConfig::GroundY-1;
    const bool crouch=down && wasGrounded && !drop;
    const float limit=BossDead()?m_stage.Data().width-GameConfig::PlayerWidth*0.5F:m_stage.Data().gate;
    if(m_player.Update(dt,input.left&&!crouch,input.right&&!crouch,input.jump,
        m_stage.Platforms(),m_stage.Data().width,drop,limit))
        m_sounds.push_back(GameSound::Jump);

    m_action.crouch=down && m_player.IsGrounded();
    m_action.direction=input.aimUp&&!input.crouch?AimDirection::Up:
        down&&!m_player.IsGrounded()?AimDirection::Down:AimDirection::Forward;
    m_stage.Update(dt,m_player.X());

    Tick(m_action.footDust,dt);
    if(m_player.IsGrounded() && (!wasGrounded||(m_player.IsMoving()&&m_action.footDust<=0)))
    {
        const float behind=m_player.IsFacingLeft()?15.0F:-15.0F;
        AddEffect(EffectKind::Dust,{m_player.X()+behind,m_player.Y()-3},wasGrounded?0.26F:0.33F);
        m_action.footDust=0.17F;
    }
}

void GameSession::FireWeapons(const GameInput& input)
{
    const float facing=m_player.IsFacingLeft()?-1.0F:1.0F;
    if(input.grenade && m_run.grenades>0 && m_action.grenadeCooldown<=0)
    {
        --m_run.grenades;
        m_action.grenadeCooldown=0.7F;
        m_action.throwing=0.22F;
        m_action.aim=m_action.flash=0;
        m_projectiles.push_back({ProjectileType::Grenade,m_player.X()+facing*48,
            m_player.Y()-(Crouching()?46.0F:85.0F),facing*370,-350,1.15F});
    }
    if(!input.fire||Throwing()) return;
    m_action.aim=0.18F;
    if(m_action.rifleCooldown>0) return;

    const bool heavy=m_run.ammo>0;
    if(heavy) --m_run.ammo;
    m_action.rifleCooldown=heavy?0.095F:0.15F;
    m_action.flash=0.055F;
    m_action.flashDirection=Aim();
    m_action.flashLeft=m_player.IsFacingLeft();
    const float speed=heavy?990.0F:850.0F;
    const auto frame=VisualFrame();
    m_projectiles.push_back({heavy?ProjectileType::MachineGun:ProjectileType::Rifle,
        m_player.X()+facing*frame.WeaponOffsetX(),m_player.Y()-frame.WeaponHeight(),
        AimX()*speed,AimY()*speed,0.9F});
    m_sounds.push_back(GameSound::Shot);
}

void GameSession::AddEffect(EffectKind kind,Combat::Point at,float duration,int amount)
{
    m_effects.push_back({kind,at.x,at.y,duration,duration,amount});
}

void GameSession::CollectSupplies()
{
    for(auto& supply:m_pickups)
    {
        if(supply.taken||std::abs(supply.x-m_player.X())>=40||std::abs(supply.y-m_player.Y())>=55)
            continue;

        EffectKind effect=EffectKind::Health;
        int amount=0;
        switch(supply.kind)
        {
        case SupplyKind::Health:
            if(m_run.health<3) { ++m_run.health; amount=1; }
            break;
        case SupplyKind::Grenades:
            effect=EffectKind::Grenades;
            amount=(std::min)(2,9-m_run.grenades);
            m_run.grenades+=amount;
            break;
        case SupplyKind::MachineGun:
            effect=EffectKind::MachineGun;
            amount=(std::min)(55,90-m_run.ammo);
            m_run.ammo+=amount;
            break;
        }
        if(!amount) continue; // 최대치면 보급품 유지
        supply.taken=true;
        m_run.score+=50;
        m_sounds.push_back(GameSound::Pickup);
        AddEffect(effect,{supply.x,supply.y-57},0.88F,amount);
    }
}

void GameSession::CleanUp(float dt)
{
    const float width=m_stage.Data().width;
    m_projectiles.erase(std::remove_if(m_projectiles.begin(),m_projectiles.end(),[width](const Projectile& b)
        { return b.life<=0||b.x<-32||b.x>width+32||b.y>650||b.y<-200; }),m_projectiles.end());
    m_enemies.erase(std::remove_if(m_enemies.begin(),m_enemies.end(),[](const Enemy& enemy)
        { return enemy.hp<=0&&enemy.deathRemaining<=0; }),m_enemies.end());
    for(auto& effect:m_effects) effect.remaining-=dt;
    m_effects.erase(std::remove_if(m_effects.begin(),m_effects.end(),[](const GameEffect& effect)
        { return effect.remaining<=0; }),m_effects.end());
}

void GameSession::Step(float dt,const GameInput& input,bool dropRequested)
{
    m_run.seconds+=dt;
    m_stageTime+=dt;
    Tick(m_action.rifleCooldown,dt);
    Tick(m_action.grenadeCooldown,dt);
    Tick(m_action.invulnerable,dt);
    Tick(m_action.flash,dt);
    Tick(m_action.aim,dt);
    Tick(m_action.throwing,dt);
    Tick(m_action.hurtFlash,dt);
    UpdatePlayer(dt,input,dropRequested);
    FireWeapons(input);
    for(auto& enemy:m_enemies)
    {
        UpdateEnemy(enemy,dt);
        if(m_state!=PlayState::Playing) break;
    }
    if(m_state==PlayState::Playing) UpdateProjectiles(dt);
    CleanUp(dt);
    if(m_state!=PlayState::Playing) return;
    CollectSupplies();

    if(BossDead() && m_stage.AtEnd(m_player.X()) && m_player.IsGrounded())
    {
        m_state=StageNumber()==StageCount?PlayState::Clear:PlayState::StageClear;
        m_action.flash=0;
        m_run.score+=500+(StageNumber()-1)*250;
        m_sounds.push_back(GameSound::Clear);
    }
}
