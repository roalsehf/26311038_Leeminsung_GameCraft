#pragma once
#include <array>
#include "GameConfig.h"

enum class PlayerPose { Idle, Run, Shoot, RunShoot, Jump, Crouch, CrouchShoot, Throw, Dead };

namespace PlayerArt
{
    // 모든 자세에 같은 픽셀 배율을 사용합니다. 앉기/사망 이미지를 키우지 않습니다.
    inline constexpr float SourceSize=1254.0F;
    inline constexpr float StandingHeight=266.0F;
    inline constexpr std::array<float,16> FeetY=
        {299,299,299,299,297,297,292,292,296,296,297,297,266,289,289,289};
    // 각 프레임에서 실제 총구가 있는 픽셀 좌표. 달리기 상체의 높이 변화도 반영합니다.
    inline constexpr std::array<float,16> GunX=
        {258,259,260,289,265,264,264,263,264,264,265,265,262,261,260,260};
    inline constexpr std::array<float,16> GunY=
        {122,122,113,120,125,129,124,115,124,127,128,120,115,195,184,184};
    inline float WeaponOffsetX(int frame)
    {
        const int column=frame%4;
        const int cellWidth=(column+1)*1254/4-column*1254/4;
        return (GunX[frame]-cellWidth*0.5F)*GameConfig::PlayerHeight/StandingHeight;
    }
    inline float WeaponHeight(int frame)
    { return (FeetY[frame]-GunY[frame])*GameConfig::PlayerHeight/StandingHeight; }
    inline int Frame(PlayerPose pose,int runFrame,float time)
    {
        switch(pose)
        {
        case PlayerPose::Run: case PlayerPose::RunShoot: return 4+runFrame%8;
        case PlayerPose::Shoot: return 2;
        case PlayerPose::Throw: return 3;
        case PlayerPose::Jump: return 12;
        case PlayerPose::Crouch: return 13;
        case PlayerPose::CrouchShoot: return 14;
        case PlayerPose::Dead: return 15;
        default: return static_cast<int>(time*2)%2;
        }
    }
}
