#pragma once
#include "GameSession.h"
#include <cmath>

// 일반 입력만 사용합니다. 무적, 순간이동, 적 체력 변경은 하지 않습니다.
inline GameInput VerificationInput(const GameSession& game,float& engagedHome)
{
    GameInput input;
    const Enemy* target=nullptr;
    for(const Enemy& e:game.Enemies())
        if(e.hp>0&&e.x>game.Player().X()-80&&(!target||e.x<target->x)) target=&e;
    if(target&&(target->home==engagedHome || target->x-game.Player().X()<(target->kind==EnemyKind::Boss?400:600)))
    {
        engagedHome=target->home;
        input.crouch=true; input.fire=true;
        input.grenade=target->kind==EnemyKind::Boss&&game.Grenades()>0;
        if(game.Player().Y()<target->y-54) { input={}; input.right=true; }
        else if(game.Player().IsFacingLeft()) { input.crouch=false; input.right=true; }
    }
    else input.right=true;
    if(!input.crouch&&game.Player().IsGrounded())
        for(const Projectile& b:game.Projectiles())
            if(b.hostile&&std::abs(b.x-game.Player().X())<160&&std::abs(b.y-game.Player().Y()+65)<90)
                input.jump=true;
    return input;
}
