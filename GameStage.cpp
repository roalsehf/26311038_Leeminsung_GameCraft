#include "GameStage.h"
#include "GameConfig.h"
#include "GameResources.h"
#include "glc2d.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
    constexpr float TileSize = 64.0F;
    constexpr float Ground=GameConfig::GroundY;
    using E=EnemyKind;
    using S=SupplyKind;
    const std::array<StageDefinition,StageCount> Stages={{
        {"사막 진입로","장갑 지휘차",GameConfig::WorldWidth,4800,4880,0xFFFFC47A,
         {{700,436,256},{1040,352,256},{1600,436,320},{2120,384,384},{2800,436,256},{3150,352,256},{3850,436,384}},
         {{E::Soldier,640,Ground,2},{E::Soldier,1160,Ground,2},{E::Turret,1510,Ground,5},
          {E::Soldier,1880,Ground,2},{E::Soldier,2440,Ground,2},{E::Turret,2780,Ground,5},
          {E::Soldier,3180,Ground,2},{E::Soldier,3640,Ground,2},{E::Turret,3980,Ground,5},{E::Boss,4630,Ground,36}},
         {{S::Health,1010,Ground},{S::Grenades,2050,Ground},{S::Health,2670,Ground},
          {S::Grenades,3500,Ground},{S::Health,4200,Ground}}},
        {"철제 보급기지","기지 공성포",4608,4300,4380,0xFF74DCD7,
         {{400,424,256},{800,356,256},{1170,410,256},{1650,432,320},{2020,354,256},
          {2450,426,320},{2980,372,320},{3460,435,320},{3940,420,384}},
         {{E::Soldier,520,Ground,2},{E::Drone,925,275,3},{E::Turret,1290,410,4},
          {E::Soldier,1460,Ground,2},{E::Drone,1900,280,3},{E::Soldier,2580,Ground,3},
          {E::Drone,2750,245,3},{E::Turret,3130,372,5},{E::Soldier,3390,Ground,3},{E::Boss,4090,420,28}},
         {{S::MachineGun,730,Ground},{S::Health,1540,Ground},{S::Grenades,2180,Ground},
          {S::Health,2880,Ground},{S::MachineGun,3650,Ground},{S::Health,3800,Ground}}},
        {"야간 요새","로켓 장갑차",5184,4900,4980,0xFFFFA080,
         {{450,414,256},{850,356,320},{1260,423,320},{1830,422,320},{2300,355,320},
          {2760,425,320},{3160,384,256},{3750,423,320},{4250,426,256}},
         {{E::Soldier,550,Ground,3},{E::Turret,1000,356,5},{E::Drone,1230,280,4},
          {E::Soldier,1580,Ground,3},{E::Turret,1990,422,5},{E::Soldier,2300,Ground,3},
          {E::Drone,2830,260,4},{E::Drone,3460,246,4},{E::Soldier,3670,Ground,3},
          {E::Turret,3910,423,6},{E::Boss,4660,Ground,44}},
         {{S::MachineGun,690,Ground},{S::Health,1500,Ground},{S::Grenades,2190,Ground},
          {S::MachineGun,3150,Ground},{S::Health,3530,Ground},{S::Grenades,4150,Ground},{S::Health,4310,Ground}}}
    }};

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

    // 스프라이트 투명 여백 제거
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

    // 원경 스크롤
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

    // 배경 소품
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
        { GameTexture::DryBush, 5000, 521, 104, 42 }
    };

    const Decoration DepotObjects[]=
    {
        {GameTexture::Wire,75,520,156,74}, {GameTexture::WarningLight,303,520,32,46},
        {GameTexture::Crate,535,424,96,51}, {GameTexture::Barrel,642,520,52,60},
        {GameTexture::Crate,884,356,92,48}, {GameTexture::Barrel,1030,520,54,60},
        {GameTexture::Radio,1190,410,52,52}, {GameTexture::Crate,1350,520,92,52},
        {GameTexture::Wire,1550,520,170,95}, {GameTexture::Crate,1770,432,94,47},
        {GameTexture::WarningLight,2002,520,34,43}, {GameTexture::Barrel,2200,354,56,61},
        {GameTexture::Sandbags,2320,520,125,50}, {GameTexture::Crate,2510,426,94,51},
        {GameTexture::Barrel,2640,520,52,60}, {GameTexture::Radio,3020,372,59,56},
        {GameTexture::Crate,3190,520,114,60}, {GameTexture::Wire,3370,520,134,89},
        {GameTexture::Barrel,3510,435,55,60}, {GameTexture::WarningLight,3870,520,40,53},
        {GameTexture::Crate,3990,520,90,48}, {GameTexture::Sandbags,4205,520,104,45}
    };
    const Decoration FortressObjects[]=
    {
        {GameTexture::RockWall,0,520,150,160,1,0xFF8790AA},{GameTexture::WarningLight,340,520,33,44},
        {GameTexture::Sandbags,510,414,115,52},{GameTexture::Wire,730,520,152,100},
        {GameTexture::Crate,955,356,78,47},{GameTexture::Tower,1050,490,150,200,0.87F,0xAA8C9CBC},
        {GameTexture::Barrel,1170,520,50,63},{GameTexture::Radio,1360,423,59,55},
        {GameTexture::Wire,1630,520,164,82},{GameTexture::Crate,1840,520,91,49},
        {GameTexture::Sandbags,1970,422,133,52},{GameTexture::WarningLight,2180,520,37,44},
        {GameTexture::Tower,2300,490,175,212,0.87F,0xAA8C9CBC},{GameTexture::Barrel,2470,355,50,55},
        {GameTexture::Crate,2715,520,94,46},{GameTexture::Wire,2920,520,165,91},
        {GameTexture::WarningLight,3250,384,36,45},{GameTexture::Sandbags,3340,520,117,45},
        {GameTexture::Tower,3620,490,172,208,0.88F,0xAA8C9CBC},{GameTexture::Radio,3800,423,60,62},
        {GameTexture::Crate,3955,520,96,51},{GameTexture::Barrel,4100,520,56,65},
        {GameTexture::Wire,4290,520,153,92},{GameTexture::WarningLight,4510,520,37,46},
        {GameTexture::Sandbags,4760,520,106,49}
    };
}

const StageDefinition& GameStage::Data() const { return Stages[static_cast<std::size_t>(m_id)]; }

void GameStage::Update(float deltaSeconds, float playerX)
{
    const float maxCamera = Data().width - GameConfig::ScreenWidth;
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
    const bool desert=m_id==StageId::Desert;
    const bool night=m_id==StageId::Fortress;
    const int panorama = resources.Texture(desert?GameTexture::Background:GameTexture::CampaignBackgrounds);
    const float panoramaWidth = static_cast<float>(g2_TextureWidth(panorama));
    const int fullHeight=g2_TextureHeight(panorama);
    const int half=fullHeight/2;
    const RECT panoramaSource{0,night?half:0,static_cast<LONG>(panoramaWidth),desert||night?fullHeight:half};
    const float panoramaHeight=static_cast<float>(panoramaSource.bottom-panoramaSource.top);
    const float scale = (std::max)(GameConfig::ScreenHeight / panoramaHeight,
        GameConfig::ScreenWidth / panoramaWidth);
    const float cameraRange = Data().width - GameConfig::ScreenWidth;
    const float panoramaRange = panoramaWidth * scale - GameConfig::ScreenWidth;
    const float panoramaX = -panoramaRange * m_cameraX / cameraRange;
    const VEC2 backgroundPosition(panoramaX, 0.0F);
    const VEC2 backgroundScale(scale, scale);
    g2_Draw2D(panorama, &panoramaSource, &backgroundPosition, &backgroundScale);

    if(desert) for (const Decoration& object : DistantScenery)
        DrawDecoration(resources, object, m_cameraX);

    // 지면 틈 채우기
    const RECT groundSource = { 0, 0, 64, 63 };
    const int firstTile = static_cast<int>(std::floor(m_cameraX / TileSize));
    const int lastTile = static_cast<int>(std::ceil(
        (m_cameraX + GameConfig::ScreenWidth) / TileSize));
    for (int tile = firstTile; tile < lastTile; ++tile)
    {
        const float x = tile * TileSize - m_cameraX;
        if(!desert)
        {
            const DWORD tint=night?0xFF9DAAC5:0xFFC0C7C8;
            DrawPart(resources, GameTexture::Concrete,groundSource,x,Ground,TileSize,80,tint);
            DrawPart(resources, GameTexture::PlatformMiddle,RECT{0,0,64,22},x,Ground,TileSize,17,tint);
            continue;
        }
        DrawPart(resources, GameTexture::SandFill, groundSource, x,
            GameConfig::GroundY, TileSize, GameConfig::ScreenHeight - GameConfig::GroundY);
        DrawPart(resources, GameTexture::SandTop, groundSource, x,
            GameConfig::GroundY, TileSize, TileSize);
    }

    const RECT platformSource = { 0, 0, 64, 64 };
    for (const StagePlatform& platform : Platforms())
    {
        const int tileCount = static_cast<int>(platform.width / TileSize);
        for (int tile = 0; tile < tileCount; ++tile)
        {
            const GameTexture texture = tile == 0 ? GameTexture::PlatformLeft
                : (tile == tileCount - 1 ? GameTexture::PlatformRight : GameTexture::PlatformMiddle);
            DrawPart(resources, texture, platformSource,
                platform.left + tile * TileSize - m_cameraX, platform.top,
                TileSize, 32.0F,night?0xFFA4B1D1:0xFFFFFFFF);
        }
    }

    if(desert) for(const auto& object:StageObjects) DrawDecoration(resources,object,m_cameraX);
    else if(night) for(const auto& object:FortressObjects) DrawDecoration(resources,object,m_cameraX);
    else for(const auto& object:DepotObjects) DrawDecoration(resources,object,m_cameraX);
    DrawDecoration(resources,{GameTexture::FinishFlag,Data().goal-52,Ground,112,114},m_cameraX);
}

void GameStage::RenderForeground(const GameResources& resources) const
{
    if(m_id!=StageId::Desert) return;
    // 지면 앞쪽 풀
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
    return Data().platforms;
}

float GameStage::CameraX() const
{
    return m_cameraX;
}

bool GameStage::AtEnd(float playerX) const
{
    return playerX >= Data().goal;
}
