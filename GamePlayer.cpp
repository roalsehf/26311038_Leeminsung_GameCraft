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

void GamePlayer::Update(float deltaSeconds, bool moveLeft, bool moveRight,
    bool jumpPressed, const std::vector<StagePlatform>& platforms)
{
    if (deltaSeconds <= 0.0F)
        return;

    m_jumpBuffer = jumpPressed ? JumpGraceSeconds
        : (std::max)(0.0F, m_jumpBuffer - deltaSeconds);
    m_coyoteTime = m_grounded ? JumpGraceSeconds
        : (std::max)(0.0F, m_coyoteTime - deltaSeconds);

    if (m_jumpBuffer > 0.0F && m_coyoteTime > 0.0F)
    {
        m_verticalSpeed = -JumpSpeed;
        m_grounded = false;
        m_coyoteTime = 0.0F;
        m_jumpBuffer = 0.0F;
    }

    // A와 D를 함께 누르면 이동하지 않습니다.
    const int direction = static_cast<int>(moveRight) - static_cast<int>(moveLeft);
    if (direction != 0)
        m_facingLeft = direction < 0;

    const float previousX = m_x;
    const float halfWidth = GameConfig::PlayerWidth * 0.5F;
    m_x = std::clamp(m_x + direction * GameConfig::MoveSpeed * deltaSeconds,
        halfWidth, GameConfig::WorldWidth - halfWidth);
    m_moving = m_x != previousX;

    const float previousFeet = m_y;
    float nextFeet = m_y + m_verticalSpeed * deltaSeconds
        + 0.5F * Gravity * deltaSeconds * deltaSeconds;
    m_verticalSpeed += Gravity * deltaSeconds;
    m_grounded = false;

    // 발판은 아래에서 통과하며, 내려올 때 발이 윗면을 지나는 경우에만 착지합니다.
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

            // 착지 직전에 눌렀던 점프도 한 번 받아 주되, 길게 눌러 연속 점프하지 않습니다.
            if (m_jumpBuffer > 0.0F)
            {
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
}

int GamePlayer::RunFrame() const
{
    return static_cast<int>(m_runTime/GameConfig::RunFrameSeconds)%8;
}

void GamePlayer::Render(const GameResources& resources,float cameraX,int atlasFrame) const
{
    atlasFrame=std::clamp(atlasFrame,0,15);
    const int texture=resources.Texture(GameTexture::PlayerAtlas);
    const int width=g2_TextureWidth(texture),height=g2_TextureHeight(texture);
    const int column=atlasFrame%4,row=atlasFrame/4;
    RECT source{column*width/4,row*height/4,(column+1)*width/4,(row+1)*height/4};
    const float canvasWidth=static_cast<float>(source.right-source.left);
    const float sourceRatio=height/PlayerArt::SourceSize;
    const float ratio=GameConfig::PlayerHeight/(PlayerArt::StandingHeight*sourceRatio);
    // 투척 그림에 그려진 공중 수류탄은 제외하고 실제 월드 투사체만 표시합니다.
    if(atlasFrame==3) source.right=source.left+static_cast<LONG>(249*sourceRatio);
    const float sourceWidth=static_cast<float>(source.right-source.left);
    const float left=m_facingLeft?canvasWidth*0.5F-sourceWidth:-canvasWidth*0.5F;
    const VEC2 position(m_x-cameraX+left*ratio,
        m_y-PlayerArt::FeetY[atlasFrame]*sourceRatio*ratio);
    const VEC2 scale(m_facingLeft?-ratio:ratio,ratio);
    g2_Draw2D(texture,&source,&position,&scale);
}

void GamePlayer::RestrictX(float left,float right) { m_x=std::clamp(m_x,left,right); }
