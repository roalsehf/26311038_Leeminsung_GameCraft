#pragma once
#include "GameResources.h"

struct ProjectileAppearance
{
    GameTexture texture;
    int left,top,right,bottom;
    float width,height;
};

// 투사체 표시 크기
inline ProjectileAppearance ProjectileArt(bool explosive,bool hostile)
{
    if(explosive && !hostile) return {GameTexture::GrenadeProjectile,87,38,128,83,20,22};
    if(explosive) return {GameTexture::Shell,0,45,128,83,36,12};
    return {GameTexture::Bullet,0,28,78,49,hostile?28.0F:34.0F,10};
}
