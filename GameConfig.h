#pragma once

namespace GameConfig
{
    // 이전 프로젝트 세팅의 논리 해상도를 유지합니다.
    inline constexpr int ScreenWidth = 960;
    inline constexpr int ScreenHeight = 600;
    inline constexpr float WorldWidth = 5120.0F;
    inline constexpr float PlayerWidth = 128.0F;
    inline constexpr float PlayerHeight = 128.0F;
    inline constexpr float GroundY = 520.0F;
    inline constexpr float StartX = 160.0F;
    inline constexpr float MoveSpeed = 260.0F; // 초당 픽셀
    inline constexpr float RunFrameSeconds = 0.075F; // 연속 달리기 8프레임
}
