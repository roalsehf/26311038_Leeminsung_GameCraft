#pragma once
#include "GameResources.h"

struct ProjectileAppearance
{
    GameTexture texture;
    int left,top,right,bottom;
    float width,height;
};

// 투명 여백/잔상을 제외한 실제 탄체 크기입니다. 플레이어 자세와 무관합니다.
inline ProjectileAppearance ProjectileArt(bool explosive,bool hostile)
{
    if(explosive && !hostile) return {GameTexture::GrenadeProjectile,87,38,128,83,20,22};
    if(explosive) return {GameTexture::Shell,0,45,128,83,36,12};
    return {GameTexture::Bullet,0,28,78,49,hostile?28.0F:34.0F,10};
}
