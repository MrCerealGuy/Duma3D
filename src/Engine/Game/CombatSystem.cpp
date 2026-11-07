#include "Engine/Game/CombatSystem.hpp"

#include <algorithm>
#include <cmath>

namespace Engine::Game
{
    namespace
    {
        constexpr float pi = 3.14159265359f;
        constexpr float enemyRadius = 0.34f;

        Graphics::Scene::Mesh glowingSphere(Math::Vec3 emission)
        {
            Graphics::Scene::Mesh mesh = Graphics::Scene::Mesh::sphere();
            for (Graphics::Scene::Vertex& vertex : mesh.vertices)
                vertex.emissiveColor = emission;
            return mesh;
        }

        float rayBoxDistance(Math::Vec3 origin, Math::Vec3 direction, const Collision::Box& box)
        {
            float nearDistance = 0.0f;
            float farDistance = 1000.0f;
            const float origins[]{origin.x, origin.y, origin.z};
            const float directions[]{direction.x, direction.y, direction.z};
            const float minimums[]{box.minimum.x, box.minimum.y, box.minimum.z};
            const float maximums[]{box.maximum.x, box.maximum.y, box.maximum.z};
            for (int axis = 0; axis < 3; ++axis)
            {
                if (std::abs(directions[axis]) < 1.0e-6f)
                {
                    if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis])
                        return -1.0f;
                    continue;
                }
                float first = (minimums[axis] - origins[axis]) / directions[axis];
                float second = (maximums[axis] - origins[axis]) / directions[axis];
                if (first > second) std::swap(first, second);
                nearDistance = std::max(nearDistance, first);
                farDistance = std::min(farDistance, second);
                if (nearDistance > farDistance) return -1.0f;
            }
            return nearDistance;
        }
    }

    void CombatSystem::initialize(std::vector<EnemySpawn> spawns)
    {
        enemies_.clear();
        enemies_.reserve(spawns.size());
        std::size_t index = 0;
        for (const EnemySpawn& spawn : spawns)
        {
            Enemy enemy;
            enemy.position = spawn.position;
            enemy.type = spawn.type;
            enemy.health = spawn.type == EnemyType::Demon ? 108.0f : 72.0f;
            enemy.attackCooldown = 0.65f + static_cast<float>(index) * 0.41f;
            enemies_.push_back(enemy);
            ++index;
        }
        magazine_ = magazineCapacity_;
        health_ = 100.0f;
        for (Projectile& projectile : projectiles_)
            projectile.active = false;
        muzzleFlashTime_ = 0.0f;
        visualsAttached_ = false;
    }

    void CombatSystem::attachScene(Graphics::Scene::Scene& scene)
    {
        const std::size_t cube = scene.addMesh(Graphics::Scene::Mesh::cube());
        const std::size_t sphere = scene.addMesh(Graphics::Scene::Mesh::sphere());
        const std::size_t flashSphere = scene.addMesh(glowingSphere({1.0f, 0.42f, 0.04f}));
        const std::size_t playerProjectileSphere = scene.addMesh(glowingSphere({1.0f, 0.72f, 0.08f}));
        const std::size_t enemyProjectileSphere = scene.addMesh(glowingSphere({1.0f, 0.035f, 0.008f}));
        const std::size_t gruntMaterial = scene.addMaterial({{0.43f, 0.47f, 0.27f}, {}});
        const std::size_t demonMaterial = scene.addMaterial({{0.58f, 0.16f, 0.08f}, {}});
        const std::size_t armorMaterial = scene.addMaterial({{0.15f, 0.18f, 0.2f}, {}});
        const std::size_t demonArmorMaterial = scene.addMaterial({{0.25f, 0.055f, 0.035f}, {}});
        const std::size_t eyeMaterial = scene.addMaterial({{1.0f, 0.045f, 0.012f}, {}});
        const std::size_t bloodMaterial = scene.addMaterial({{0.24f, 0.012f, 0.008f}, {}});
        const std::size_t weaponMaterial = scene.addMaterial({{0.11f, 0.13f, 0.15f}, {}});
        const std::size_t flashMaterial = scene.addMaterial({{1.0f, 0.58f, 0.08f}, {}});
        const std::size_t playerProjectileMaterial = scene.addMaterial({{1.0f, 0.78f, 0.18f}, {}});
        const std::size_t enemyProjectileMaterial = scene.addMaterial({{1.0f, 0.12f, 0.025f}, {}});
        for (Enemy& enemy : enemies_)
        {
            const std::size_t body = enemy.type == EnemyType::Demon ? demonMaterial : gruntMaterial;
            const std::size_t armor = enemy.type == EnemyType::Demon ? demonArmorMaterial : armorMaterial;
            const std::size_t meshes[]{cube, cube, sphere, cube, cube, cube, cube,
                cube, cube, cube, cube, cube, cube, cube, sphere};
            const std::size_t materials[]{body, armor, body, body, body, armor, armor,
                body, body, armor, armor, eyeMaterial, body, body, bloodMaterial};
            for (std::size_t part = 0; part < enemy.objects.size(); ++part)
            {
                scene.addObject(meshes[part], materials[part]);
                enemy.objects[part] = scene.objects().size() - 1;
            }
        }
        for (std::size_t index = 0; index < weaponObjects_.size(); ++index)
        {
            scene.addObject(cube, weaponMaterial);
            weaponObjects_[index] = scene.objects().size() - 1;
        }
        scene.addObject(flashSphere, flashMaterial);
        muzzleFlashObject_ = scene.objects().size() - 1;
        for (std::size_t index = 0; index < projectiles_.size(); ++index)
        {
            const std::size_t material = index < hostileProjectileCount_
                ? enemyProjectileMaterial : playerProjectileMaterial;
            const std::size_t mesh = index < hostileProjectileCount_
                ? enemyProjectileSphere : playerProjectileSphere;
            scene.addObject(mesh, material);
            projectiles_[index].objectIndex = scene.objects().size() - 1;
        }
        muzzleLightIndex_ = scene.pointLights().size();
        hasMuzzleLight_ = scene.addPointLight({{}, {1.0f, 0.58f, 0.12f}, 0.0f, 1.0f, 0.2f, 0.08f});
        visualsAttached_ = true;
        for (Enemy& enemy : enemies_)
            placeEnemy(scene, enemy);
        for (Projectile& projectile : projectiles_)
            updateProjectileVisual(scene, projectile);
    }

    bool CombatSystem::spawnProjectile(Math::Vec3 origin, Math::Vec3 direction,
        float distance, float damage, bool hostile)
    {
        const std::size_t first = hostile ? 0 : hostileProjectileCount_;
        const std::size_t end = hostile ? hostileProjectileCount_ : projectiles_.size();
        const float directionLength = std::sqrt(Math::dot(direction, direction));
        if (directionLength < 1.0e-5f || distance <= 0.0f)
            return false;
        direction = {direction.x / directionLength, direction.y / directionLength, direction.z / directionLength};
        for (std::size_t index = first; index < end; ++index)
        {
            Projectile& projectile = projectiles_[index];
            if (projectile.active) continue;
            projectile.position = origin;
            projectile.direction = direction;
            projectile.remainingDistance = distance;
            projectile.damage = damage;
            projectile.hostile = hostile;
            projectile.active = true;
            return true;
        }
        return false;
    }

    void CombatSystem::updateProjectileVisual(Graphics::Scene::Scene& scene, Projectile& projectile)
    {
        const Math::Vec3 scale = projectile.active
            ? (projectile.hostile ? Math::Vec3{0.11f, 0.11f, 0.18f} : Math::Vec3{0.07f, 0.07f, 0.13f})
            : Math::Vec3{0.001f, 0.001f, 0.001f};
        scene.setObjectTransform(projectile.objectIndex, {projectile.position, {}, scale});
    }

    bool CombatSystem::collides(const std::vector<Collision::Box>& colliders, Math::Vec3 position) const
    {
        for (const Collision::Box& box : colliders)
        {
            if (box.maximum.y <= position.y || box.minimum.y >= position.y + 1.8f)
                continue;
            const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
            const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
            const float dx = position.x - closestX;
            const float dz = position.z - closestZ;
            if (dx * dx + dz * dz < enemyRadius * enemyRadius)
                return true;
        }
        return false;
    }

    bool CombatSystem::hasLineOfSight(const std::vector<Collision::Box>& colliders,
        Math::Vec3 from, Math::Vec3 to) const
    {
        const Math::Vec3 delta{to.x - from.x, to.y - from.y, to.z - from.z};
        const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        const int steps = static_cast<int>(distance / 0.2f);
        for (int step = 1; step < steps; ++step)
        {
            const float amount = static_cast<float>(step) / static_cast<float>(steps);
            const Math::Vec3 point{from.x + delta.x * amount, from.y + delta.y * amount,
                from.z + delta.z * amount};
            if (collides(colliders, {point.x, point.y - 0.85f, point.z}))
                return false;
        }
        return true;
    }

    void CombatSystem::placeEnemy(Graphics::Scene::Scene& scene, Enemy& enemy)
    {
        const float yawRadians = enemy.yawDegrees * pi / 180.0f;
        const Math::Vec3 facing{std::sin(yawRadians), 0.0f, -std::cos(yawRadians)};
        const Math::Vec3 right{std::cos(yawRadians), 0.0f, std::sin(yawRadians)};
        const auto offset = [&enemy, &right, &facing](float side, float height, float forward)
        {
            return Math::Vec3{enemy.position.x + right.x * side + facing.x * forward,
                enemy.position.y + height,
                enemy.position.z + right.z * side + facing.z * forward};
        };
        const float death = enemy.alive ? 0.0f : std::clamp(enemy.deathProgress, 0.0f, 1.0f);
        const float fallAngle = death * 82.0f;
        const float walk = enemy.alive ? std::sin(enemy.walkPhase) : 0.0f;
        const float stride = enemy.alive ? walk * 0.16f : 0.0f;
        const Math::Vec3 positions[]{
            offset(0.0f, 1.0f - death * 0.55f, 0.0f),
            offset(0.0f, 1.03f - death * 0.48f, 0.08f),
            offset(0.0f, 1.72f - death * 1.05f, death * 0.12f),
            offset(-0.34f, 1.22f - death * 0.82f, 0.0f),
            offset(0.34f, 1.22f - death * 0.82f, 0.0f),
            offset(-0.38f, 0.83f - death * 0.62f, 0.0f),
            offset(0.38f, 0.83f - death * 0.62f, 0.0f),
            offset(-0.14f, 0.48f - death * 0.31f, stride),
            offset(0.14f, 0.48f - death * 0.31f, -stride),
            offset(-0.14f, 0.13f - death * 0.09f, stride + 0.04f),
            offset(0.14f, 0.13f - death * 0.09f, -stride + 0.04f),
            offset(0.0f, 1.74f - death * 1.65f, 0.24f),
            offset(-0.15f, 1.89f - death * 1.3f, 0.01f),
            offset(0.15f, 1.89f - death * 1.3f, 0.01f),
            offset(0.0f, -0.02f, 0.0f)
        };
        const Math::Vec3 scales[]{
            {0.48f, 0.68f, 0.3f}, {0.34f, 0.32f, 0.075f}, {0.34f, 0.4f, 0.32f},
            {0.14f, 0.34f, 0.15f}, {0.14f, 0.34f, 0.15f},
            {0.13f, 0.34f, 0.14f}, {0.13f, 0.34f, 0.14f},
            {0.17f, 0.39f, 0.18f}, {0.17f, 0.39f, 0.18f},
            {0.2f, 0.13f, 0.3f}, {0.2f, 0.13f, 0.3f},
            enemy.alive ? Math::Vec3{0.19f, 0.045f, 0.035f} : Math::Vec3{0.01f, 0.01f, 0.01f},
            enemy.type == EnemyType::Demon ? Math::Vec3{0.085f, 0.2f, 0.09f} : Math::Vec3{0.01f, 0.01f, 0.01f},
            enemy.type == EnemyType::Demon ? Math::Vec3{0.085f, 0.2f, 0.09f} : Math::Vec3{0.01f, 0.01f, 0.01f},
            enemy.alive ? Math::Vec3{0.01f, 0.01f, 0.01f} : Math::Vec3{0.68f, 0.025f, 0.52f}
        };
        const Math::Vec3 rotations[]{
            {fallAngle, enemy.yawDegrees, 0.0f}, {fallAngle, enemy.yawDegrees, 0.0f},
            {fallAngle, enemy.yawDegrees, 0.0f},
            {0.0f, enemy.yawDegrees, (death * -48.0f) + walk * 24.0f},
            {0.0f, enemy.yawDegrees, (death * 48.0f) - walk * 24.0f},
            {0.0f, enemy.yawDegrees, death * -70.0f}, {0.0f, enemy.yawDegrees, death * 70.0f},
            {stride * 100.0f, enemy.yawDegrees, death * 32.0f},
            {-stride * 100.0f, enemy.yawDegrees, death * -32.0f},
            {0.0f, enemy.yawDegrees, 0.0f}, {0.0f, enemy.yawDegrees, 0.0f},
            {0.0f, enemy.yawDegrees, 0.0f}, {0.0f, enemy.yawDegrees, 0.0f},
            {0.0f, enemy.yawDegrees, 0.0f}, {0.0f, 0.0f, 0.0f}
        };
        for (std::size_t part = 0; part < enemy.objects.size(); ++part)
            scene.setObjectTransform(enemy.objects[part], {positions[part], rotations[part], scales[part]});
    }

    float CombatSystem::update(float deltaTime, Math::Vec3 playerPosition,
        const std::vector<Collision::Box>& colliders, Graphics::Scene::Scene& scene)
    {
        if (!visualsAttached_ || !std::isfinite(deltaTime) || deltaTime <= 0.0f)
            return 0.0f;
        float damageToPlayer = 0.0f;
        muzzleFlashTime_ = std::max(0.0f, muzzleFlashTime_ - deltaTime);
        const Math::Vec3 playerTarget{playerPosition.x, playerPosition.y - 0.65f, playerPosition.z};
        for (Projectile& projectile : projectiles_)
        {
            if (!projectile.active) continue;
            const float speed = projectile.hostile ? 13.0f : 72.0f;
            const float distance = std::min(speed * deltaTime, projectile.remainingDistance);
            const Math::Vec3 next{projectile.position.x + projectile.direction.x * distance,
                projectile.position.y + projectile.direction.y * distance,
                projectile.position.z + projectile.direction.z * distance};
            bool impacted = false;
            for (const Collision::Box& box : colliders)
            {
                const float wallDistance = rayBoxDistance(projectile.position, projectile.direction, box);
                if (wallDistance >= 0.0f && wallDistance <= distance + 0.03f)
                {
                    impacted = true;
                    break;
                }
            }
            if (!impacted && projectile.hostile)
            {
                const Math::Vec3 segment{next.x - projectile.position.x, next.y - projectile.position.y,
                    next.z - projectile.position.z};
                const Math::Vec3 toPlayer{playerTarget.x - projectile.position.x,
                    playerTarget.y - projectile.position.y, playerTarget.z - projectile.position.z};
                const float segmentLengthSquared = Math::dot(segment, segment);
                const float amount = segmentLengthSquared > 1.0e-6f
                    ? std::clamp(Math::dot(toPlayer, segment) / segmentLengthSquared, 0.0f, 1.0f) : 0.0f;
                const Math::Vec3 closest{projectile.position.x + segment.x * amount,
                    projectile.position.y + segment.y * amount,
                    projectile.position.z + segment.z * amount};
                const Math::Vec3 offset{closest.x - playerTarget.x, closest.y - playerTarget.y,
                    closest.z - playerTarget.z};
                if (Math::dot(offset, offset) < 0.42f * 0.42f)
                {
                    damageToPlayer += projectile.damage;
                    impacted = true;
                }
            }
            if (impacted)
                projectile.active = false;
            else
            {
                projectile.position = next;
                projectile.remainingDistance -= distance;
                if (projectile.remainingDistance <= 1.0e-4f)
                    projectile.active = false;
            }
            updateProjectileVisual(scene, projectile);
        }
        for (Enemy& enemy : enemies_)
        {
            if (!enemy.alive)
            {
                enemy.deathProgress = std::min(1.0f, enemy.deathProgress + deltaTime / 0.58f);
                placeEnemy(scene, enemy);
                continue;
            }
            enemy.attackCooldown = std::max(0.0f, enemy.attackCooldown - deltaTime);
            const float dx = playerPosition.x - enemy.position.x;
            const float dz = playerPosition.z - enemy.position.z;
            const float distance = std::sqrt(dx * dx + dz * dz);
            if (distance > 1.0e-4f)
                enemy.yawDegrees = std::atan2(-dx, -dz) * 180.0f / pi;
            const float alertRange = enemy.type == EnemyType::Demon ? 13.0f : 10.0f;
            const float speed = enemy.type == EnemyType::Demon ? 1.55f : 1.05f;
            const Math::Vec3 previousPosition = enemy.position;
            if (distance > 1.7f && distance < alertRange)
            {
                const float step = std::min(speed * deltaTime, distance - 1.45f);
                const float moveX = dx / distance * step;
                const float moveZ = dz / distance * step;
                Math::Vec3 candidate = enemy.position;
                candidate.x += moveX;
                if (!collides(colliders, candidate)) enemy.position.x = candidate.x;
                candidate = enemy.position;
                candidate.z += moveZ;
                if (!collides(colliders, candidate)) enemy.position.z = candidate.z;
            }
            const float movedX = enemy.position.x - previousPosition.x;
            const float movedZ = enemy.position.z - previousPosition.z;
            if (movedX * movedX + movedZ * movedZ > 1.0e-6f)
                enemy.walkPhase += deltaTime * (enemy.type == EnemyType::Demon ? 10.0f : 7.5f);
            const Math::Vec3 eye{enemy.position.x, enemy.position.y + 1.35f, enemy.position.z};
            if (distance < 8.0f && enemy.attackCooldown <= 0.0f &&
                hasLineOfSight(colliders, eye, playerPosition))
            {
                const Math::Vec3 target{playerPosition.x, playerPosition.y - 0.65f, playerPosition.z};
                const Math::Vec3 direction{target.x - eye.x, target.y - eye.y, target.z - eye.z};
                const float projectileRange = std::sqrt(Math::dot(direction, direction));
                if (spawnProjectile(eye, direction, projectileRange,
                    enemy.type == EnemyType::Demon ? 14.0f : 9.0f, true))
                    enemy.attackCooldown = enemy.type == EnemyType::Demon ? 1.25f : 1.7f;
            }
            placeEnemy(scene, enemy);
        }
        health_ = std::max(0.0f, health_ - damageToPlayer);
        return damageToPlayer;
    }

    ShotResult CombatSystem::fire(Math::Vec3 origin, Math::Vec3 direction,
        const std::vector<Collision::Box>& colliders)
    {
        if (magazine_ <= 0) return {};
        --magazine_;
        direction = Math::normalize(direction);
        muzzleFlashTime_ = 0.075f;
        float nearestWall = 30.0f;
        for (const Collision::Box& box : colliders)
        {
            const float distance = rayBoxDistance(origin, direction, box);
            if (distance >= 0.0f) nearestWall = std::min(nearestWall, distance);
        }
        Enemy* target = nullptr;
        float nearestEnemy = nearestWall;
        for (Enemy& enemy : enemies_)
        {
            if (!enemy.alive) continue;
            const Math::Vec3 center{enemy.position.x, enemy.position.y + 1.2f, enemy.position.z};
            const Math::Vec3 offset{center.x - origin.x, center.y - origin.y, center.z - origin.z};
            const float projection = Math::dot(offset, direction);
            if (projection <= 0.0f || projection >= nearestEnemy) continue;
            const float offsetLength = Math::dot(offset, offset);
            const float perpendicularSquared = offsetLength - projection * projection;
            const float hitRadius = enemy.type == EnemyType::Demon ? 0.72f : 0.62f;
            if (perpendicularSquared <= hitRadius * hitRadius)
            {
                target = &enemy;
                nearestEnemy = projection;
            }
        }
        const float projectileDistance = target ? nearestEnemy : nearestWall;
        spawnProjectile({origin.x + direction.x * 0.75f, origin.y + direction.y * 0.75f,
            origin.z + direction.z * 0.75f}, direction, std::max(0.05f, projectileDistance - 0.75f), 0.0f, false);
        if (!target)
            return {true, false, false};
        target->health -= shotDamage_;
        if (target->health <= 0.0f)
        {
            target->alive = false;
            return {true, true, true};
        }
        return {true, true, false};
    }

    void CombatSystem::updateWeapon(const Graphics::Camera& camera, Graphics::Scene::Scene& scene)
    {
        if (!visualsAttached_) return;
        const Math::Vec3 front = camera.front();
        const Math::Vec3 worldUp{0.0f, 1.0f, 0.0f};
        const Math::Vec3 right = Math::normalize(Math::cross(front, worldUp));
        const float yaw = std::atan2(front.x, front.z) * 180.0f / pi;
        const Math::Vec3 base{
            camera.position().x + front.x * 0.68f + right.x * 0.32f,
            camera.position().y - 0.3f,
            camera.position().z + front.z * 0.68f + right.z * 0.32f
        };
        const Math::Vec3 barrel{base.x + front.x * 0.19f, base.y + 0.04f, base.z + front.z * 0.19f};
        const Math::Vec3 receiver{base.x, base.y - 0.03f, base.z};
        const Math::Vec3 grip{base.x - front.x * 0.1f, base.y - 0.19f, base.z - front.z * 0.1f};
        scene.setObjectTransform(weaponObjects_[0], {barrel, {0.0f, yaw, 0.0f}, {0.14f, 0.15f, 0.5f}});
        scene.setObjectTransform(weaponObjects_[1], {receiver, {0.0f, yaw, 0.0f}, {0.22f, 0.2f, 0.34f}});
        scene.setObjectTransform(weaponObjects_[2], {grip, {0.0f, yaw, -16.0f}, {0.15f, 0.28f, 0.18f}});
        const float flash = std::clamp(muzzleFlashTime_ / 0.075f, 0.0f, 1.0f);
        const Math::Vec3 muzzle{base.x + front.x * 0.45f, base.y + 0.04f,
            base.z + front.z * 0.45f};
        const float flashScale = flash > 0.0f ? flash : 0.001f;
        scene.setObjectTransform(muzzleFlashObject_, {muzzle, {0.0f, yaw, 0.0f},
            {0.13f * flashScale, 0.13f * flashScale, 0.3f * flashScale}});
        if (hasMuzzleLight_)
            scene.setPointLight(muzzleLightIndex_, {muzzle, {1.0f, 0.53f, 0.12f},
                12.0f * flash, 1.0f, 0.4f, 0.15f});
    }

    std::size_t CombatSystem::enemiesAlive() const
    {
        return static_cast<std::size_t>(std::count_if(enemies_.begin(), enemies_.end(),
            [](const Enemy& enemy) { return enemy.alive; }));
    }
}
