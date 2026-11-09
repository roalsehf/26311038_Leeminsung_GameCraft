#pragma once
#include <array>
#include "GameConfig.h"

enum class PlayerPose { Idle, Run, Shoot, RunShoot, Jump, Crouch, CrouchShoot, Throw, Dead, AimUp, AimDown };
enum class AimDirection { Forward, Up, Down };

namespace PlayerArt
{
    // 공통 배율
    inline constexpr float SourceSize=1254.0F;
    inline constexpr float StandingHeight=266.0F;
    inline constexpr std::array<float,16> FeetY=
        {299,299,299,299,297,297,292,292,296,296,297,297,266,289,289,289};
    // 프레임별 총구 위치
    inline constexpr std::array<float,16> GunX=
        {258,259,260,289,265,264,264,263,264,264,265,265,262,261,260,260};
    inline constexpr std::array<float,16> GunY=
        {122,122,113,120,125,129,124,115,124,127,128,120,115,195,184,184};
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
    struct FrameSpec
    {
        int index, grid;
        float bodyHeight, feet, muzzleX, muzzleY;
        bool Vertical() const { return grid==2; }
        float Scale() const { return GameConfig::PlayerHeight/bodyHeight; }
        float WeaponOffsetX() const
        {
            const int column=index%grid;
            const int width=(column+1)*1254/grid-column*1254/grid;
            return (muzzleX-width*0.5F)*Scale();
        }
        float WeaponHeight() const { return (feet-muzzleY)*Scale(); }
    };
    inline FrameSpec Select(PlayerPose pose,int runFrame,bool moving,bool grounded,float time)
    {
        if(pose==PlayerPose::AimDown) return {2,2,466,479,301,515};
        if(pose==PlayerPose::AimUp)
        {
            if(!grounded) return {3,2,466,479,374,34};
            if(moving && (runFrame/2)%2) return {1,2,466,555,373,26};
            return {0,2,466,556,384,25};
        }
        const int i=Frame(pose,runFrame,time);
        return {i,4,StandingHeight,FeetY[i],GunX[i],GunY[i]};
    }
}
