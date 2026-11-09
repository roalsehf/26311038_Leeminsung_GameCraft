#pragma once

#include "GameStage.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace Combat
{
    struct Point { float x=0, y=0; };
    struct Box { float left, top, right, bottom; };

    inline constexpr float Miss=2.0F;
    inline constexpr float Gravity=680.0F;
    inline constexpr float MortarFlight=1.20F;
    inline constexpr float MortarRadius=86.0F;
    inline constexpr float GrenadeRadius=145.0F;

    inline Box Body(Point feet,float halfWidth,float height)
    {
        return {feet.x-halfWidth,feet.y-height,feet.x+halfWidth,feet.y+3.0F};
    }

    // 첫 충돌 지점
    inline float Sweep(Point from,Point to,Box box)
    {
        float first=0.0F,last=1.0F;
        const auto clip=[&](float start,float movement,float lo,float hi)
        {
            if(std::abs(movement)<0.0001F) return start>=lo && start<=hi;
            float entry=(lo-start)/movement,exit=(hi-start)/movement;
            if(entry>exit) std::swap(entry,exit);
            first=(std::max)(first,entry);
            last=(std::min)(last,exit);
            return first<=last;
        };
        return clip(from.x,to.x-from.x,box.left,box.right) &&
            clip(from.y,to.y-from.y,box.top,box.bottom) ? first : Miss;
    }

    inline Point Interpolate(Point a,Point b,float t)
    { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t}; }

    inline bool InsideBlast(Point center,float radius,Box box)
    {
        const float dx=center.x-std::clamp(center.x,box.left,box.right);
        const float dy=center.y-std::clamp(center.y,box.top,box.bottom);
        return dx*dx+dy*dy<=radius*radius;
    }

    inline bool Overlaps(Box a,Box b)
    { return a.left<b.right && b.left<a.right && a.top<b.bottom && b.top<a.bottom; }
}

enum class ProjectileType { Rifle, MachineGun, EnemyRound, Grenade, Mortar };
struct Projectile
{
    ProjectileType type;
    float x,y,vx,vy,life;
    float landingX=0;

    bool Hostile() const { return type==ProjectileType::EnemyRound||type==ProjectileType::Mortar; }
    bool Explosive() const { return type==ProjectileType::Grenade||type==ProjectileType::Mortar; }
    bool Heavy() const { return type==ProjectileType::MachineGun; }
    int Damage() const { return Heavy()?2:Explosive()&&!Hostile()?8:1; }
};

enum class AttackKind { None, Rifle, Cannon, Aimed, Siege, Mortar, Sweep, Fan };
struct EnemyAttack
{
    AttackKind kind=AttackKind::None;
    Combat::Point origin,target;
    float remaining=0,duration=0,recovery=0;
    int facing=-1;
    bool phaseTwo=false;
    std::array<Projectile,3> shots{};
    std::size_t shotCount=0;

    bool Active() const { return kind!=AttackKind::None; }
    void Queue(Projectile shot)
    {
        assert(shotCount<shots.size());
        if(shotCount<shots.size()) shots[shotCount++]=shot;
    }
};

struct Enemy
{
    EnemyKind kind;
    float x,y,home,homeFeet;
    int hp,maxHp;
    float fireTimer;
    float hurt=0,phase=0,flash=0,deathRemaining=0;
    int facing=-1,volley=0;
    EnemyAttack attack;
};

enum class EffectKind { Impact, Explosion, Dust, Health, Grenades, MachineGun };
struct GameEffect
{
    EffectKind kind;
    float x,y,remaining,duration;
    int amount=0;
    float Progress() const { return 1.0F-std::clamp(remaining/duration,0.0F,1.0F); }
};
