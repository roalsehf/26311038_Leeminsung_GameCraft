#pragma once

#include "GameConfig.h"
#include "PlayerAnimation.h"
#include <cstdint>
#include <vector>

class GameResources;
struct StagePlatform;

class GamePlayer
{
public:
    bool Update(float deltaSeconds, bool moveLeft, bool moveRight,
        bool jumpPressed, const std::vector<StagePlatform>& platforms,float worldWidth=GameConfig::WorldWidth,
        bool dropThrough=false,float rightLimit=0);
    void Render(const GameResources& resources, float cameraX,const PlayerArt::FrameSpec& frame,
        std::uint32_t color=0xFFFFFFFF) const;
    int RunFrame() const;

    float X() const { return m_x; }
    float Y() const { return m_y; } // 발 위치
    bool IsMoving() const { return m_moving; }
    bool IsFacingLeft() const { return m_facingLeft; }
    bool IsGrounded() const { return m_grounded; }

private:
    float m_x = GameConfig::StartX;
    float m_y = GameConfig::GroundY;
    float m_verticalSpeed = 0.0F;
    float m_runTime = 0.0F;
    float m_coyoteTime = 0.08F;
    float m_jumpBuffer = 0.0F;
    bool m_facingLeft = false;
    bool m_moving = false;
    bool m_grounded = true;
};
