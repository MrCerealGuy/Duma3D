#pragma once

#include "Engine/Game/Collision/CollisionBox.hpp"
#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/Scene/Scene.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace Engine::Game
{
    enum class EnemyType { Grunt, Demon };

    struct EnemySpawn
    {
        Math::Vec3 position{};
        EnemyType type = EnemyType::Grunt;
    };

    struct ShotResult
    {
        bool fired = false;
        bool hit = false;
        bool killed = false;
    };

    // Small reusable hitscan combat loop with simple pursuit and ranged attacks.
    class CombatSystem
    {
    public:
        void initialize(std::vector<EnemySpawn> spawns);
        void attachScene(Graphics::Scene::Scene& scene);
        float update(float deltaTime, Math::Vec3 playerPosition,
            const std::vector<Collision::Box>& colliders, Graphics::Scene::Scene& scene);
        ShotResult fire(Math::Vec3 origin, Math::Vec3 direction,
            const std::vector<Collision::Box>& colliders);
        void updateWeapon(const Graphics::Camera& camera, Graphics::Scene::Scene& scene);

        std::size_t enemiesAlive() const;
        int magazine() const { return magazine_; }
        void reload() { magazine_ = magazineCapacity_; }
        float health() const { return health_; }

    private:
        struct Enemy
        {
            Math::Vec3 position{};
            EnemyType type = EnemyType::Grunt;
            float health = 0.0f;
            float attackCooldown = 0.0f;
            float yawDegrees = 0.0f;
            float walkPhase = 0.0f;
            float deathProgress = 0.0f;
            std::array<std::size_t, 15> objects{};
            bool alive = true;
        };

        struct Projectile
        {
            Math::Vec3 position{};
            Math::Vec3 direction{};
            float remainingDistance = 0.0f;
            float damage = 0.0f;
            bool hostile = false;
            bool active = false;
            std::size_t objectIndex = 0;
        };

        bool collides(const std::vector<Collision::Box>& colliders, Math::Vec3 position) const;
        bool hasLineOfSight(const std::vector<Collision::Box>& colliders,
            Math::Vec3 from, Math::Vec3 to) const;
        void placeEnemy(Graphics::Scene::Scene& scene, Enemy& enemy);
        bool spawnProjectile(Math::Vec3 origin, Math::Vec3 direction, float distance,
            float damage, bool hostile);
        void updateProjectileVisual(Graphics::Scene::Scene& scene, Projectile& projectile);

        static constexpr int magazineCapacity_ = 12;
        static constexpr float shotDamage_ = 36.0f;
        std::vector<Enemy> enemies_;
        std::array<std::size_t, 3> weaponObjects_{};
        static constexpr std::size_t hostileProjectileCount_ = 16;
        std::array<Projectile, 24> projectiles_{};
        std::size_t muzzleFlashObject_ = 0;
        std::size_t muzzleLightIndex_ = 0;
        bool hasMuzzleLight_ = false;
        float muzzleFlashTime_ = 0.0f;
        int magazine_ = magazineCapacity_;
        float health_ = 100.0f;
        bool visualsAttached_ = false;
    };
}
