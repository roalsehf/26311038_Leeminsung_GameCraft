#include "GamePlayer.h"
#include "GameResources.h"
#include "GameStage.h"
#include "PlayerAnimation.h"
#include "glc2d.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float JumpSpeed = 600.0F;
    constexpr float Gravity = 1600.0F;
    constexpr float JumpGraceSeconds = 0.08F;
    constexpr float BodyHalfWidth = 22.0F;
    constexpr float RunFrameCount = 8.0F;
}

bool GamePlayer::Update(float deltaSeconds, bool moveLeft, bool moveRight,
    bool jumpPressed, const std::vector<StagePlatform>& platforms,float worldWidth,bool dropThrough,float rightLimit)
{
    if (deltaSeconds <= 0.0F)
        return false;
    bool jumped=false;
    if(dropThrough && m_grounded && m_y<GameConfig::GroundY-1)
    {
        // 발판 내려가기
        m_y+=2;
        m_verticalSpeed=35;
        m_grounded=false;
        m_coyoteTime=m_jumpBuffer=0;
        jumpPressed=false;
    }

    m_jumpBuffer = jumpPressed ? JumpGraceSeconds
        : (std::max)(0.0F, m_jumpBuffer - deltaSeconds);
    m_coyoteTime = m_grounded ? JumpGraceSeconds
        : (std::max)(0.0F, m_coyoteTime - deltaSeconds);

    if (m_jumpBuffer > 0.0F && m_coyoteTime > 0.0F)
    {
        jumped=true;
        m_verticalSpeed = -JumpSpeed;
        m_grounded = false;
        m_coyoteTime = 0.0F;
        m_jumpBuffer = 0.0F;
    }

    // 플레이어 이동
    const int direction = static_cast<int>(moveRight) - static_cast<int>(moveLeft);
    if (direction != 0)
        m_facingLeft = direction < 0;

    const float previousX = m_x;
    const float halfWidth = GameConfig::PlayerWidth * 0.5F;
    const float worldEdge=worldWidth-halfWidth;
    const float edge=rightLimit>0?(std::min)(worldEdge,rightLimit):worldEdge;
    m_x = std::clamp(m_x + direction * GameConfig::MoveSpeed * deltaSeconds,halfWidth,edge);
    m_moving = m_x != previousX;

    const float previousFeet = m_y;
    float nextFeet = m_y + m_verticalSpeed * deltaSeconds
        + 0.5F * Gravity * deltaSeconds * deltaSeconds;
    m_verticalSpeed += Gravity * deltaSeconds;
    m_grounded = false;

    // 발판 착지
    if (m_verticalSpeed >= 0.0F)
    {
        float landingY = GameConfig::GroundY;
        for (const StagePlatform& platform : platforms)
        {
            const bool overlaps = m_x + BodyHalfWidth > platform.left
                && m_x - BodyHalfWidth < platform.left + platform.width;
            const bool crossesTop = previousFeet <= platform.top + 0.01F
                && nextFeet >= platform.top;
            if (overlaps && crossesTop)
                landingY = (std::min)(landingY, platform.top);
        }

        if (nextFeet >= landingY)
        {
            nextFeet = landingY;
            m_verticalSpeed = 0.0F;
            m_grounded = true;

            // 착지 직전 점프 입력
            if (m_jumpBuffer > 0.0F)
            {
                jumped=true;
                m_verticalSpeed = -JumpSpeed;
                m_grounded = false;
                m_jumpBuffer = 0.0F;
                m_coyoteTime = 0.0F;
            }
        }
    }
    m_y = nextFeet;

    if (m_moving && m_grounded)
        m_runTime = std::fmod(m_runTime + deltaSeconds,
            GameConfig::RunFrameSeconds * RunFrameCount);
    else
        m_runTime = 0.0F;
    return jumped;
}

int GamePlayer::RunFrame() const
{
    return static_cast<int>(m_runTime/GameConfig::RunFrameSeconds)%8;
}

void GamePlayer::Render(const GameResources& resources,float cameraX,const PlayerArt::FrameSpec& frame,std::uint32_t color) const
{
    const int texture=resources.Texture(frame.Vertical()?GameTexture::VerticalAim:GameTexture::PlayerAtlas);
    const int width=g2_TextureWidth(texture),height=g2_TextureHeight(texture);
    const int column=frame.index%frame.grid,row=frame.index/frame.grid;
    RECT source{column*width/frame.grid,row*height/frame.grid,(column+1)*width/frame.grid,(row+1)*height/frame.grid};
    const float canvasWidth=static_cast<float>(source.right-source.left);
    const float sourceRatio=height/PlayerArt::SourceSize;
    const float ratio=frame.Scale()/sourceRatio;
    // 그림 속 수류탄 제외
    if(!frame.Vertical() && frame.index==3) source.right=source.left+static_cast<LONG>(249*sourceRatio);
    const float sourceWidth=static_cast<float>(source.right-source.left);
    const float left=m_facingLeft?canvasWidth*0.5F-sourceWidth:-canvasWidth*0.5F;
    const VEC2 position(m_x-cameraX+left*ratio,
        m_y-frame.feet*sourceRatio*ratio);
    const VEC2 scale(m_facingLeft?-ratio:ratio,ratio);
    g2_Draw2D(texture,&source,&position,&scale,nullptr,0,color);
}
