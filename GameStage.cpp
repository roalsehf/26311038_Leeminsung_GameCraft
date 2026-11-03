#include "GameStage.h"
#include "GameConfig.h"
#include "GameResources.h"
#include "glc2d.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float TileSize = 64.0F;
    constexpr float FinishX = 4800.0F;
    constexpr float FirstSectionEnd = 1700.0F;
    constexpr float SecondSectionEnd = 3400.0F;

    struct Decoration
    {
        GameTexture texture;
        float x;
        float bottom;
        float width;
        float height;
        float scrollRatio = 1.0F;
        DWORD color = 0xFFFFFFFF;
    };

    // 원본 PNG의 투명 여백을 제외하여 물체의 밑면이 지면에 닿게 그립니다.
    RECT VisibleRect(GameTexture texture, int textureId)
    {
        switch (texture)
        {
        case GameTexture::Crate:      return { 0, 29, 128, 98 };
        case GameTexture::Sandbags:   return { 0, 36, 128, 92 };
        case GameTexture::Barrel:     return { 0, 10, 128, 117 };
        case GameTexture::Wire:       return { 0, 25, 128, 102 };
        case GameTexture::Radio:      return { 0, 0, 127, 128 };
        case GameTexture::RoadSign:   return { 0, 19, 128, 108 };
        case GameTexture::Tower:      return { 41, 0, 215, 176 };
        case GameTexture::DeadTree:   return { 36, 0, 219, 176 };
        case GameTexture::DryBush:    return { 0, 36, 256, 140 };
        case GameTexture::BigMesa:    return { 36, 0, 220, 176 };
        case GameTexture::FinishFlag: return { 7, 0, 153, 144 };
        default:
            return { 0, 0, g2_TextureWidth(textureId), g2_TextureHeight(textureId) };
        }
    }

    void DrawPart(const GameResources& resources, GameTexture texture,
        const RECT& source, float x, float y, float width, float height,
        DWORD color = 0xFFFFFFFF)
    {
        if (x + width <= 0.0F || x >= GameConfig::ScreenWidth)
            return;

        const VEC2 position(x, y);
        const VEC2 scale(width / static_cast<float>(source.right - source.left),
            height / static_cast<float>(source.bottom - source.top));
        g2_Draw2D(resources.Texture(texture), &source, &position, &scale,
            nullptr, 0.0F, color);
    }

    void DrawDecoration(const GameResources& resources, const Decoration& object,
        float cameraX)
    {
        const int texture = resources.Texture(object.texture);
        const RECT source = VisibleRect(object.texture, texture);
        DrawPart(resources, object.texture, source,
            object.x - cameraX * object.scrollRatio, object.bottom - object.height,
            object.width, object.height, object.color);
    }

    // 먼 바위와 감시탑은 지면보다 느리게 움직여 원근감을 만듭니다.
    const Decoration DistantScenery[] =
    {
        { GameTexture::BigMesa, 180, 474, 232, 222, 0.50F, 0xBBD8C2AA },
        { GameTexture::BigMesa, 1160, 478, 282, 270, 0.50F, 0xBBD8C2AA },
        { GameTexture::BigMesa, 2180, 472, 228, 218, 0.50F, 0xBBD8C2AA },
        { GameTexture::DeadTree, 430, 496, 146, 140, 0.80F, 0xDDDED0B8 },
        { GameTexture::DeadTree, 980, 498, 114, 110, 0.80F, 0xDDDED0B8 },
        { GameTexture::DeadTree, 1920, 496, 124, 120, 0.80F, 0xDDDED0B8 },
        { GameTexture::DeadTree, 3010, 498, 110, 106, 0.80F, 0xDDDED0B8 },
        { GameTexture::Tower, 1560, 506, 206, 208, 0.88F, 0xEEDDD5C1 },
        { GameTexture::Tower, 2580, 507, 222, 225, 0.88F, 0xEEDDD5C1 },
        { GameTexture::Tower, 3540, 505, 248, 251, 0.88F, 0xEEDDD5C1 },
        { GameTexture::Tower, 4280, 508, 214, 217, 0.88F, 0xEEDDD5C1 }
    };

    // 충돌하지 않는 보급품과 기지 소품입니다. 플레이어보다 먼저 그립니다.
    const Decoration StageObjects[] =
    {
        { GameTexture::DryBush, 50, 521, 86, 35 },
        { GameTexture::RoadSign, 356, 520, 94, 65 },
        { GameTexture::DryBush, 588, 521, 102, 41 },
        { GameTexture::Crate, 790, 436, 78, 42 },
        { GameTexture::Sandbags, 1110, 520, 132, 58 },
        { GameTexture::Barrel, 1364, 520, 61, 51 },
        { GameTexture::DryBush, 1490, 522, 96, 39 },
        { GameTexture::Crate, 1730, 436, 82, 44 },
        { GameTexture::Crate, 1816, 436, 72, 39 },
        { GameTexture::Wire, 1944, 520, 158, 95 },
        { GameTexture::Sandbags, 2150, 520, 150, 66 },
        { GameTexture::Crate, 2228, 384, 76, 41 },
        { GameTexture::Barrel, 2334, 384, 59, 49 },
        { GameTexture::Crate, 2450, 520, 92, 50 },
        { GameTexture::Crate, 2538, 520, 79, 43 },
        { GameTexture::Barrel, 2604, 520, 67, 56 },
        { GameTexture::RoadSign, 2720, 520, 88, 61 },
        { GameTexture::Sandbags, 2874, 436, 116, 51 },
        { GameTexture::Wire, 3034, 520, 146, 88 },
        { GameTexture::Crate, 3250, 352, 73, 39 },
        { GameTexture::DryBush, 3380, 521, 83, 34 },
        { GameTexture::Wire, 3510, 520, 154, 93 },
        { GameTexture::Sandbags, 3700, 520, 144, 63 },
        { GameTexture::Radio, 3980, 436, 69, 70 },
        { GameTexture::Crate, 4080, 436, 82, 44 },
        { GameTexture::Crate, 4310, 520, 88, 47 },
        { GameTexture::Radio, 4326, 473, 54, 55 },
        { GameTexture::Barrel, 4430, 520, 65, 54 },
        { GameTexture::Sandbags, 4580, 520, 124, 54 },
        { GameTexture::FinishFlag, FinishX, 520, 122, 120 },
        { GameTexture::DryBush, 5000, 521, 104, 42 }
    };
}

void GameStage::Update(float deltaSeconds, float playerX)
{
    const float maxCamera = GameConfig::WorldWidth - GameConfig::ScreenWidth;
    const float target = std::clamp(playerX - GameConfig::ScreenWidth * 0.40F,
        0.0F, maxCamera);
    const float blend = 1.0F - std::exp(-9.0F * (std::max)(0.0F, deltaSeconds));
    m_cameraX += (target - m_cameraX) * blend;
    if (std::abs(target - m_cameraX) < 0.05F)
        m_cameraX = target;
    m_cameraX = std::clamp(m_cameraX, 0.0F, maxCamera);
}

void GameStage::RenderBackdrop(const GameResources& resources) const
{
    const int panorama = resources.Texture(GameTexture::Background);
    const float panoramaWidth = static_cast<float>(g2_TextureWidth(panorama));
    const float panoramaHeight = static_cast<float>(g2_TextureHeight(panorama));
    const float scale = (std::max)(GameConfig::ScreenHeight / panoramaHeight,
        GameConfig::ScreenWidth / panoramaWidth);
    const float cameraRange = GameConfig::WorldWidth - GameConfig::ScreenWidth;
    const float panoramaRange = panoramaWidth * scale - GameConfig::ScreenWidth;
    const float panoramaX = -panoramaRange * m_cameraX / cameraRange;
    const VEC2 backgroundPosition(panoramaX, 0.0F);
    const VEC2 backgroundScale(scale, scale);
    g2_Draw2D(panorama, nullptr, &backgroundPosition, &backgroundScale);

    for (const Decoration& object : DistantScenery)
        DrawDecoration(resources, object, m_cameraX);

    // 밑흙을 먼저 채워 PNG의 투명한 가장자리에 배경 틈이 생기지 않게 합니다.
    const RECT groundSource = { 0, 0, 64, 63 };
    const int firstTile = static_cast<int>(std::floor(m_cameraX / TileSize));
    const int lastTile = static_cast<int>(std::ceil(
        (m_cameraX + GameConfig::ScreenWidth) / TileSize));
    for (int tile = firstTile; tile < lastTile; ++tile)
    {
        const float x = tile * TileSize - m_cameraX;
        DrawPart(resources, GameTexture::SandFill, groundSource, x,
            GameConfig::GroundY, TileSize, GameConfig::ScreenHeight - GameConfig::GroundY);
        DrawPart(resources, GameTexture::SandTop, groundSource, x,
            GameConfig::GroundY, TileSize, TileSize);
    }

    const RECT platformSource = { 0, 0, 64, 64 };
    for (const StagePlatform& platform : m_platforms)
    {
        const int tileCount = static_cast<int>(platform.width / TileSize);
        for (int tile = 0; tile < tileCount; ++tile)
        {
            const GameTexture texture = tile == 0 ? GameTexture::PlatformLeft
                : (tile == tileCount - 1 ? GameTexture::PlatformRight : GameTexture::PlatformMiddle);
            DrawPart(resources, texture, platformSource,
                platform.left + tile * TileSize - m_cameraX, platform.top,
                TileSize, 32.0F);
        }
    }

    for (const Decoration& object : StageObjects)
        DrawDecoration(resources, object, m_cameraX);
}

void GameStage::RenderForeground(const GameResources& resources) const
{
    // 작은 풀만 지면 가장자리에 덧그려 캐릭터의 상체와 진행 방향을 가리지 않습니다.
    const Decoration edgeBushes[] =
    {
        { GameTexture::DryBush, 12, 546, 91, 25, 1.0F, 0xEEC9BA98 },
        { GameTexture::DryBush, 1240, 545, 75, 22, 1.0F, 0xEEC9BA98 },
        { GameTexture::DryBush, 2020, 546, 81, 24, 1.0F, 0xEEC9BA98 },
        { GameTexture::DryBush, 3440, 547, 82, 24, 1.0F, 0xEEC9BA98 },
        { GameTexture::DryBush, 4770, 546, 74, 22, 1.0F, 0xEEC9BA98 }
    };
    for (const Decoration& object : edgeBushes)
        DrawDecoration(resources, object, m_cameraX);
}

const std::vector<StagePlatform>& GameStage::Platforms() const
{
    return m_platforms;
}

float GameStage::CameraX() const
{
    return m_cameraX;
}

bool GameStage::AtEnd(float playerX) const
{
    return playerX >= FinishX + 80.0F;
}

const char* GameStage::SectionName(float playerX) const
{
    if (playerX < FirstSectionEnd)
        return "DESERT APPROACH";
    if (playerX < SecondSectionEnd)
        return "SUPPLY OUTPOST";
    return "COMMUNICATION BASE";
}
