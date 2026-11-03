#pragma once

#include <vector>

class GameResources;

struct StagePlatform
{
    float left;
    float top;
    float width;
};

// 한 스테이지의 발판, 장식과 횡스크롤 카메라를 관리합니다.
class GameStage
{
public:
    void Update(float deltaSeconds, float playerX);
    void RenderBackdrop(const GameResources& resources) const;
    void RenderForeground(const GameResources& resources) const;

    const std::vector<StagePlatform>& Platforms() const;
    float CameraX() const;
    bool AtEnd(float playerX) const;
    const char* SectionName(float playerX) const;

private:
    float m_cameraX = 0.0F;
    std::vector<StagePlatform> m_platforms =
    {
        { 700.0F, 436.0F, 256.0F },
        { 1040.0F, 352.0F, 256.0F },
        { 1600.0F, 436.0F, 320.0F },
        { 2120.0F, 384.0F, 384.0F },
        { 2800.0F, 436.0F, 256.0F },
        { 3150.0F, 352.0F, 256.0F },
        { 3850.0F, 436.0F, 384.0F }
    };
};
