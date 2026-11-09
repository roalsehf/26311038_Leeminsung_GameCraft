#include "GameResources.h"
#include "glc2d.h"

namespace
{
    constexpr std::array<const char*, static_cast<std::size_t>(GameTexture::Count)> TexturePaths =
    {
        "resource/RunAndGun_Desert_Assets/backgrounds/desert_full_wide.png",
        "resource/campaign/depot_fortress.png",
        "resource/campaign/machines.png",
        "resource/campaign/vertical_aim.png",
        "resource/RunAndGun_Desert_Assets/tiles/reinforced_concrete.png",
        "resource/RunAndGun_Desert_Assets/tiles/rock_wall.png",
        "resource/RunAndGun_Desert_Assets/props/warning_light.png",
        "resource/RunAndGun_Desert_Assets/props/metal_door.png",
        "resource/RunAndGun_Desert_Assets/items/heavy_machinegun.png",
        "resource/player/soldier_actions_v2.png",
        "resource/RunAndGun_Desert_Assets/tiles/sand_top.png",
        "resource/RunAndGun_Desert_Assets/tiles/sand_fill.png",
        "resource/RunAndGun_Desert_Assets/tiles/metal_platform_left.png",
        "resource/RunAndGun_Desert_Assets/tiles/metal_platform_middle.png",
        "resource/RunAndGun_Desert_Assets/tiles/metal_platform_right.png",
        "resource/RunAndGun_Desert_Assets/props/wooden_crate.png",
        "resource/RunAndGun_Desert_Assets/props/sandbags.png",
        "resource/RunAndGun_Desert_Assets/props/explosive_barrel.png",
        "resource/RunAndGun_Desert_Assets/props/barbed_wire.png",
        "resource/RunAndGun_Desert_Assets/props/radio.png",
        "resource/RunAndGun_Desert_Assets/props/road_sign.png",
        "resource/RunAndGun_Desert_Assets/scenery/guard_tower.png",
        "resource/RunAndGun_Desert_Assets/scenery/dead_tree.png",
        "resource/RunAndGun_Desert_Assets/scenery/dry_bush.png",
        "resource/RunAndGun_Desert_Assets/scenery/big_mesa.png",
        "resource/RunAndGun_Desert_Assets/interactive/finish_flag.png",
        "resource/RunAndGun_Desert_Assets/enemies/soldier_idle.png",
        "resource/RunAndGun_Desert_Assets/enemies/soldier_walk_01.png",
        "resource/RunAndGun_Desert_Assets/enemies/soldier_walk_02.png",
        "resource/RunAndGun_Desert_Assets/enemies/soldier_shoot.png",
        "resource/RunAndGun_Desert_Assets/enemies/soldier_death.png",
        "resource/RunAndGun_Desert_Assets/enemies/turret_idle.png",
        "resource/RunAndGun_Desert_Assets/enemies/turret_shoot.png",
        "resource/RunAndGun_Desert_Assets/enemies/turret_destroyed.png",
        "resource/RunAndGun_Desert_Assets/boss/tank_idle.png",
        "resource/RunAndGun_Desert_Assets/boss/tank_shoot.png",
        "resource/RunAndGun_Desert_Assets/boss/tank_damaged.png",
        "resource/RunAndGun_Desert_Assets/boss/tank_wreck.png",
        "resource/RunAndGun_Desert_Assets/effects/bullet.png",
        "resource/RunAndGun_Desert_Assets/effects/tank_shell.png",
        "resource/RunAndGun_Desert_Assets/effects/grenade_projectile.png",
        "resource/RunAndGun_Desert_Assets/effects/muzzle_flash.png",
        "resource/RunAndGun_Desert_Assets/effects/bullet_impact.png",
        "resource/RunAndGun_Desert_Assets/effects/grenade_explosion.png",
        "resource/RunAndGun_Desert_Assets/effects/sand_dust.png",
        "resource/RunAndGun_Desert_Assets/items/health_kit.png",
        "resource/RunAndGun_Desert_Assets/items/grenade.png",
        "resource/RunAndGun_Desert_Assets/hud/health_icon.png",
        "resource/RunAndGun_Desert_Assets/hud/grenade_icon.png",
        "resource/RunAndGun_Desert_Assets/hud/hud_panel.png",
        "resource/RunAndGun_Desert_Assets/hud/boss_hp_full.png",
        "resource/RunAndGun_Desert_Assets/hud/boss_hp_empty.png"
    };
    constexpr std::array<const char*, static_cast<std::size_t>(GameMusic::Count)> MusicPaths =
    { nullptr, "resource/sound/negev_desert_loop.wav", "resource/sound/negev_fight_loop.wav" };
    constexpr std::array<const char*, static_cast<std::size_t>(GameSound::Count)> SoundPaths =
    { "resource/sound/shot.wav", "resource/sound/explosion.wav", "resource/sound/hurt.wav",
      "resource/sound/pickup.wav", "resource/sound/clear.wav", "resource/sound/jump.wav" };
}

GameResources::GameResources()
{
    m_textures.fill(-1);
    m_music.fill(-1);
    m_sounds.fill(-1);
}

bool GameResources::Load(const char*& failedPath)
{
    Unload();
    failedPath = nullptr;
    for (std::size_t index = 0; index < TexturePaths.size(); ++index)
    {
        m_textures[index] = g2_TextureLoad(TexturePaths[index], 0);
        if (m_textures[index] < 0)
        {
            failedPath = TexturePaths[index];
            Unload();
            return false;
        }
    }

    for (std::size_t index=1; index<MusicPaths.size(); ++index)
    {
        m_music[index]=g2_SoundLoad(MusicPaths[index]);
        if (m_music[index]<0) { failedPath=MusicPaths[index]; Unload(); return false; }
    }
    for (std::size_t index=0; index<SoundPaths.size(); ++index)
    {
        m_sounds[index]=g2_SoundLoad(SoundPaths[index]);
        if(m_sounds[index]<0) { failedPath=SoundPaths[index]; Unload(); return false; }
    }
    return true;
}

bool GameResources::SetMusic(GameMusic track)
{
    if (m_currentMusic==track) return true;

    const int previous=m_music[static_cast<std::size_t>(m_currentMusic)];
    if (previous>=0)
    {
        g2_SoundStop(previous);
        if (g2_SoundIsPlaying(previous)) return false;
    }
    m_currentMusic=track;

    if (track==GameMusic::Silence) return true;
    const int next=m_music[static_cast<std::size_t>(track)];
    if (next<0) return false;
    g2_SoundReset(next);
    g2_SoundPlay(next,true);
    return g2_SoundIsPlaying(next);
}

void GameResources::Unload()
{
    SetMusic(GameMusic::Silence);
    for(int& sound:m_sounds)
    {
        if(sound>=0) { g2_SoundStop(sound); g2_SoundRelease(sound); }
        sound=-1;
    }
    for (int& music:m_music)
    {
        if (music>=0) g2_SoundRelease(music);
        music=-1;
    }
    m_currentMusic=GameMusic::Silence;
    for (int& texture : m_textures)
    {
        if (texture >= 0)
            g2_TextureRelease(texture);
        texture = -1;
    }
}

int GameResources::Texture(GameTexture texture) const
{
    return m_textures[static_cast<std::size_t>(texture)];
}

void GameResources::Play(GameSound sound)
{
    const int id=m_sounds[static_cast<std::size_t>(sound)];
    if(id>=0) { g2_SoundStop(id); g2_SoundReset(id); g2_SoundPlay(id,false); }
}
