#pragma once

namespace GameConfig
{
    // 화면 해상도
    inline constexpr int ScreenWidth = 960;
    inline constexpr int ScreenHeight = 600;
    inline constexpr float WorldWidth = 5120.0F;
    inline constexpr float PlayerWidth = 128.0F;
    inline constexpr float PlayerHeight = 128.0F;
    inline constexpr float GroundY = 520.0F;
    inline constexpr float StartX = 160.0F;
    inline constexpr float MoveSpeed = 260.0F; // 초당 픽셀
    inline constexpr float RunFrameSeconds = 0.075F; // 달리기 프레임 간격
}
