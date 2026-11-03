#pragma once
#include "GamePlayer.h"
#include "GameResources.h"
#include "GameStage.h"
#include "PlayerAnimation.h"
#include <vector>

enum class PlayState { Title, Playing, Paused, GameOver, Clear };
enum class EnemyKind { Soldier, Turret, Boss };
struct GameInput
{
    bool left=false, right=false, jump=false, fire=false, grenade=false, crouch=false;
    bool start=false, pause=false, retry=false;
};
struct Enemy
{
    EnemyKind kind;
    float x, y, home, fireTimer, hurt=0, phase=0, windup=0;
    int hp, maxHp;
    int facing=-1;
    float deathRemaining=0;
};
struct Projectile
{
    float x, y, vx, vy, life;
    bool hostile=false, explosive=false;
    int damage=1;
};
struct GameEffect { float x,y,time; bool explosion; };
struct Pickup { float x,y; bool grenade, taken=false; };

// 입력과 경과 시간만으로 갱신하므로 렌더러 없이도 게임 진행을 검증할 수 있습니다.
class GameSession
{
public:
    void Update(float seconds, const GameInput& input);
    void Restart();
    PlayState State() const { return m_state; }
    const GamePlayer& Player() const { return m_player; }
    const GameStage& Stage() const { return m_stage; }
    const std::vector<Enemy>& Enemies() const { return m_enemies; }
    const std::vector<Projectile>& Projectiles() const { return m_projectiles; }
    const std::vector<GameEffect>& Effects() const { return m_effects; }
    const std::vector<Pickup>& Pickups() const { return m_pickups; }
    const std::vector<GameSound>& Sounds() const { return m_sounds; }
    int Health() const { return m_health; }
    int Grenades() const { return m_grenades; }
    int Score() const { return m_score; }
    int Kills() const { return m_kills; }
    float Time() const { return m_time; }
    bool Crouching() const { return m_crouch; }
    bool Shooting() const { return m_muzzle > 0; }
    bool Aiming() const { return m_aimTime>0; }
    bool Throwing() const { return m_throwTime>0; }
    PlayerPose Pose() const;
    float WeaponX() const;
    float WeaponY() const;
    float Invulnerability() const { return m_invulnerable; }
    bool BossDead() const;
private:
    void DamagePlayer();
    void DamageEnemy(Enemy& enemy,int damage);
    void Explode(float x,float y,bool hostile);
    GamePlayer m_player;
    GameStage m_stage;
    PlayState m_state=PlayState::Title;
    std::vector<Enemy> m_enemies;
    std::vector<Projectile> m_projectiles;
    std::vector<GameEffect> m_effects;
    std::vector<Pickup> m_pickups;
    std::vector<GameSound> m_sounds;
    int m_health=3, m_grenades=3, m_score=0, m_kills=0;
    float m_time=0,m_fireCooldown=0,m_grenadeCooldown=0,m_invulnerable=0,m_muzzle=0;
    bool m_crouch=false;
    bool m_bossDefeated=false;
    float m_aimTime=0,m_throwTime=0;
};
