#pragma once

#include <array>
#include <cstddef>

enum class GameTexture : std::size_t
{
    Background,
    PlayerAtlas,
    SandTop,
    SandFill,
    PlatformLeft,
    PlatformMiddle,
    PlatformRight,
    Crate,
    Sandbags,
    Barrel,
    Wire,
    Radio,
    RoadSign,
    Tower,
    DeadTree,
    DryBush,
    BigMesa,
    FinishFlag,
    SoldierIdle, SoldierWalk1, SoldierWalk2, SoldierShoot, SoldierDeath,
    TurretIdle, TurretShoot, TurretDestroyed,
    TankIdle, TankShoot, TankDamaged, TankWreck,
    Bullet, Shell, GrenadeProjectile, Muzzle, Impact, Explosion,
    HealthKit, GrenadePickup, HealthIcon, GrenadeIcon, HudPanel,
    BossBar, BossBarEmpty,
    Count
};

enum class GameSound : std::size_t { Shot, Explosion, Hurt, Pickup, Clear, Jump, Count };

// 이번 단계에서 실제 사용하는 리소스만 한 번 로드하고 종료 시 해제합니다.
class GameResources
{
public:
    GameResources();
    bool Load(const char*& failedPath);
    bool PlayBackgroundSound();
    void Unload();
    int Texture(GameTexture texture) const;
    void Play(GameSound sound);

private:
    std::array<int, static_cast<std::size_t>(GameTexture::Count)> m_textures;
    int m_backgroundSound = -1;
    std::array<int, static_cast<std::size_t>(GameSound::Count)> m_sounds;
};
