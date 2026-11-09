#pragma once

#include <array>
#include <cstddef>

enum class GameTexture : std::size_t
{
    Background,
    CampaignBackgrounds, MachineAtlas, VerticalAim, Concrete, RockWall, WarningLight, MetalDoor, MachineGunPickup,
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
    Bullet, Shell, GrenadeProjectile, Muzzle, Impact, Explosion, Dust,
    HealthKit, GrenadePickup, HealthIcon, GrenadeIcon, HudPanel,
    BossBar, BossBarEmpty,
    Count
};

enum class GameSound : std::size_t { Shot, Explosion, Hurt, Pickup, Clear, Jump, Count };
enum class GameMusic : std::size_t { Silence, Stage, Boss, Count };

// 리소스 관리
class GameResources
{
public:
    GameResources();
    bool Load(const char*& failedPath);
    bool SetMusic(GameMusic track);
    void Unload();
    int Texture(GameTexture texture) const;
    void Play(GameSound sound);

private:
    std::array<int, static_cast<std::size_t>(GameTexture::Count)> m_textures;
    std::array<int, static_cast<std::size_t>(GameMusic::Count)> m_music;
    GameMusic m_currentMusic = GameMusic::Silence;
    std::array<int, static_cast<std::size_t>(GameSound::Count)> m_sounds;
};
