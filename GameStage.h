#pragma once

#include <vector>
#include <cstdint>

class GameResources;

enum class StageId { Desert, Depot, Fortress };
enum class EnemyKind { Soldier, Turret, Drone, Boss };
enum class SupplyKind { Health, Grenades, MachineGun };
inline constexpr int StageCount=3;

struct StagePlatform
{
    float left;
    float top;
    float width;
};

struct EnemySpawn { EnemyKind kind; float x, feet; int hp; };
struct SupplySpawn { SupplyKind kind; float x, feet; };
struct StageDefinition
{
    const char* name;
    const char* bossName;
    float width, gate, goal;
    std::uint32_t accent;
    std::vector<StagePlatform> platforms;
    std::vector<EnemySpawn> enemies;
    std::vector<SupplySpawn> supplies;
};

// 스테이지와 카메라
class GameStage
{
public:
    explicit GameStage(StageId id=StageId::Desert) : m_id(id) {}
    void Update(float deltaSeconds, float playerX);
    void RenderBackdrop(const GameResources& resources) const;
    void RenderForeground(const GameResources& resources) const;

    const std::vector<StagePlatform>& Platforms() const;
    float CameraX() const;
    bool AtEnd(float playerX) const;
    const StageDefinition& Data() const;
    StageId Id() const { return m_id; }

private:
    StageId m_id;
    float m_cameraX = 0.0F;
};
