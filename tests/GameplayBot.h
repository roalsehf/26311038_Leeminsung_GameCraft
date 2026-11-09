#pragma once
#include "GameSession.h"
#include <cmath>

// 자동 플레이 입력
inline GameInput VerificationInput(const GameSession& game,float& engagedHome)
{
    GameInput input;
    const Enemy* target=nullptr;
    for(const Enemy& e:game.Enemies())
        if(e.hp>0&&e.x>game.Player().X()-80&&(!target||e.x<target->x)) target=&e;
    if(game.StageNumber()>1 && target)
    {
        const auto& player=game.Player();
        const float offset=target->x-player.X();
        const float height=target->kind==EnemyKind::Drone?58.0F:target->kind==EnemyKind::Boss?145.0F:108.0F;
        const float upY=player.Y()-(player.IsGrounded()?146.0F:122.0F);
        if(player.Y()-88>target->y+3 && upY>=target->y-height)
        {
            const float aimX=player.X()+(player.IsFacingLeft()?-18.0F:18.0F);
            const float difference=target->x-aimX;
            const float tolerance=target->kind==EnemyKind::Boss?57.0F:26.0F;
            input.aimUp=true;
            input.fire=std::abs(difference)<tolerance;
            if(!input.fire) { input.right=difference>0; input.left=difference<0; }
        }
        else if(std::abs(offset)<(target->kind==EnemyKind::Boss?430:580) &&
            player.Y()-88>=target->y-height && player.Y()-88<=target->y+3)
        {
            const bool left=offset<0;
            const bool turn=left!=player.IsFacingLeft();
            input.left=turn&&left; input.right=turn&&!left;
            input.crouch=player.IsGrounded()&&!turn && player.Y()-51<=target->y+3;
            input.fire=true;
            input.grenade=target->kind==EnemyKind::Boss && game.StageNumber()==3 && game.Grenades()>0;
        }
        else
        {
            input.right=true;
            if(target->kind==EnemyKind::Boss && std::abs(offset)<75 && !player.IsGrounded())
            { input.right=false; input.aimUp=true; input.fire=true; }
        }
    }
    else if(target&&(target->home==engagedHome || target->x-game.Player().X()<(target->kind==EnemyKind::Boss?400:600)))
    {
        engagedHome=target->home;
        input.crouch=true; input.fire=true;
        input.grenade=target->kind==EnemyKind::Boss&&game.Grenades()>0;
        if(game.Player().Y()<target->y-54) { input={}; input.right=true; }
        else if(game.Player().IsFacingLeft()) { input.crouch=false; input.right=true; }
    }
    else input.right=true;
    if(!input.crouch&&!input.aimUp&&game.Player().IsGrounded())
        for(const Projectile& b:game.Projectiles())
            if(b.Hostile()&&std::abs(b.x-game.Player().X())<160&&std::abs(b.y-game.Player().Y()+65)<90)
                input.jump=true;
    return input;
}
