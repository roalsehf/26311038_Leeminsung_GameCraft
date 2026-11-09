#pragma once
#include "GameCombat.h"
#include "GamePlayer.h"
#include "GameResources.h"
#include <vector>

enum class PlayState { Title, Playing, Paused, GameOver, StageClear, Clear };
struct GameInput
{
    bool left=false, right=false, jump=false, fire=false, grenade=false, crouch=false;
    bool start=false, pause=false, retry=false, aimUp=false;
};
struct Pickup { float x,y; SupplyKind kind; bool taken=false; };

// 게임 진행 상태
class GameSession
{
public:
    void Update(float seconds,const GameInput& input);
    void Suspend();
    void Restart();

    PlayState State() const { return m_state; }
    GameMusic Music() const;
    const GamePlayer& Player() const { return m_player; }
    const GameStage& Stage() const { return m_stage; }
    const std::vector<Enemy>& Enemies() const { return m_enemies; }
    const std::vector<Projectile>& Projectiles() const { return m_projectiles; }
    const std::vector<GameEffect>& Effects() const { return m_effects; }
    const std::vector<Pickup>& Pickups() const { return m_pickups; }
    const std::vector<GameSound>& Sounds() const { return m_sounds; }
    int Health() const { return m_run.health; }
    int Grenades() const { return m_run.grenades; }
    int Score() const { return m_run.score; }
    int Kills() const { return m_run.kills; }
    int StageKills() const { return Kills()-m_checkpoint.kills; }
    int StageNumber() const { return static_cast<int>(m_stage.Id())+1; }
    int MachineGunAmmo() const { return m_run.ammo; }
    float Time() const { return m_run.seconds; }
    float StageTime() const { return m_stageTime; }
    bool Crouching() const { return m_action.crouch; }
    bool Shooting() const;
    bool Aiming() const { return m_action.aim>0; }
    bool Throwing() const { return m_action.throwing>0; }
    PlayerPose Pose() const;
    PlayerArt::FrameSpec VisualFrame() const;
    AimDirection Aim() const { return m_action.direction; }
    float AimX() const;
    float AimY() const;
    float WeaponX() const;
    float WeaponY() const;
    float Invulnerability() const { return m_action.invulnerable; }
    float HurtFlash() const { return m_action.hurtFlash; }
    bool BossDead() const { return m_bossDefeated; }
    bool BossEngaged() const { return m_bossEngaged && !m_bossDefeated; }

private:
    struct RunStats
    {
        int health=3,grenades=3,ammo=0,score=0,kills=0;
        float seconds=0;
    };
    struct PlayerAction
    {
        float rifleCooldown=0,grenadeCooldown=0,invulnerable=0;
        float flash=0,aim=0,throwing=0,hurtFlash=0,footDust=0;
        AimDirection direction=AimDirection::Forward,flashDirection=AimDirection::Forward;
        bool crouch=false,flashLeft=false;
    };

    bool HandleState(const GameInput& input);
    void ResetClock();
    void LoadStage(StageId id);
    void RetryStage();
    void Step(float dt,const GameInput& input,bool dropRequested);
    void UpdatePlayer(float dt,const GameInput& input,bool dropRequested);
    void FireWeapons(const GameInput& input);
    void CollectSupplies();
    void CleanUp(float dt);
    void UpdateEnemy(Enemy& enemy,float dt);
    void BeginAttack(Enemy& enemy);
    void PlanVolley(const Enemy& enemy,EnemyAttack& attack) const;
    void ReleaseAttack(Enemy& enemy);
    void UpdateProjectiles(float dt);
    void DamagePlayer();
    void DamageEnemy(Enemy& enemy,int damage);
    void Explode(Combat::Point at,const Projectile& projectile);
    Combat::Box PlayerBox() const;
    Combat::Box EnemyBox(const Enemy& enemy) const;
    void AddEffect(EffectKind kind,Combat::Point at,float duration,int amount=0);

    GamePlayer m_player;
    GameStage m_stage;
    PlayState m_state=PlayState::Title;
    RunStats m_run,m_checkpoint;
    PlayerAction m_action;
    std::vector<Enemy> m_enemies;
    std::vector<Projectile> m_projectiles;
    std::vector<GameEffect> m_effects;
    std::vector<Pickup> m_pickups;
    std::vector<GameSound> m_sounds;
    float m_stageTime=0;
    double m_accumulator=0;
    bool m_jumpQueued=false,m_dropQueued=false,m_grenadeQueued=false;
    bool m_bossDefeated=false,m_bossEngaged=false;
};
