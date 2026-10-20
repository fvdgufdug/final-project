#include "Globals.h"



void applyImpact(int idx, const glm::vec3& localDir, Weapon w)
{
    if (idx < 0 || idx >= (int)bodies.size() || bodies[idx].destroyed) return;

    if (w == W_PLANET_KILLER) { killBody(idx); return; }

    const WeaponSpec& sp = kWeaponSpecs[w];
    Body& b = bodies[idx];
    glm::vec3 hitWorld = localDirToWorldPoint(b, localDir);
    glm::vec3 normal = glm::normalize(hitWorld - b.position);

    bool merged = false;
    if (w == W_LASER && !b.impacts.empty())
    {
        Impact& li = b.impacts.back();
        if (li.radius <= 0.08f && glm::dot(li.dir, localDir) > cosf(0.04f)) { li.spawnTime = g_simTime; merged = true; }
    }
    if (!merged)
    {
        b.impacts.push_back(Impact{ localDir, sp.radius, g_simTime });
        if ((int)b.impacts.size() > kMaxImpacts) b.impacts.erase(b.impacts.begin());
    }

    float fx = std::max(0.3f, b.scaledSize);
    if (w == W_LASER)
        spawnCollisionEffect(hitWorld, fx * 0.12f, 0.35f);
    else
    {
        spawnCollisionEffect(hitWorld, fx * (0.25f + sp.radius * 1.2f), 1.1f);
        if (w == W_NUKE || w == W_ASTEROID)
        {
            spawnShockwave(hitWorld, normal, fx * (0.4f + sp.radius * 1.5f), 1.6f, glm::vec3(1.0f, 0.7f, 0.35f));
            audioPlayExplosion();
        }
        else audioPlayCollision();
    }
    g_shake = std::max(g_shake, sp.shake);

    if (sp.kick > 0.0f)
    {
        float kick = (float)std::min((double)sp.kick / b.mass, 1.5);
        b.velocity -= normal * kick;
    }

    if (b.population > 0.0)
        b.population *= std::max(0.0, 1.0 - (double)sp.damage / 100.0 * 0.8);

    b.health -= sp.damage;
    if (b.health <= 0.0f) killBody(idx);
}



void fireWeapon(int idx, const glm::vec3& hitWorld)
{
    if (idx < 0 || idx >= (int)bodies.size() || bodies[idx].destroyed) return;
    if (pendingStrikes.size() > 80) return;

    const Body& b = bodies[idx];
    const glm::vec3 baseDir = worldToLocalDir(b, hitWorld);
    const glm::vec3 normal = glm::normalize(hitWorld - b.position);

    auto queue = [&](Weapon w, const glm::vec3& localDir, const glm::vec3& start, float delay, float duration)
    {
        PendingStrike s;
        s.body = idx; s.uid = b.uid; s.localDir = localDir; s.weapon = w;
        s.fireTime = g_simTime + delay; s.duration = duration; s.startWorld = start;
        pendingStrikes.push_back(s);
    };

    switch (currentWeapon)
    {
    case W_NUKE:

        queue(W_NUKE, baseDir, hitWorld + normal * (b.scaledSize * 2.0f + 8.0f), 0.0f, 0.9f);
        audioPlayLaunch();
        break;
    case W_MISSILES:

        for (int k = 0; k < 6; ++k)
            queue(W_MISSILES, jitterDir(baseDir, 0.28f), battleStationPos, 0.06f * (float)k, 1.1f);
        audioPlayLaunch();
        break;
    case W_ASTEROID:
    {

        glm::vec3 from = glm::normalize(normal + glm::vec3(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f) * 0.9f);
        queue(W_ASTEROID, baseDir, hitWorld + from * (b.scaledSize * 2.0f + 32.0f), 0.0f, 1.5f);
        audioPlayLaunch();
        break;
    }
    case W_LASER:
    {

        LaserBeam lb; lb.from = battleStationPos; lb.to = hitWorld; lb.spawnTime = g_simTime;
        laserBeams.push_back(lb);
        applyImpact(idx, baseDir, W_LASER);
        audioPlayLaser();
        g_laserTimer = 0.08f;
        break;
    }
    case W_PLANET_KILLER:
    {
        LaserBeam lb; lb.from = battleStationPos; lb.to = hitWorld; lb.spawnTime = g_simTime;
        laserBeams.push_back(lb);
        queue(W_PLANET_KILLER, baseDir, battleStationPos, 0.0f, 0.4f);
        audioPlayLaser();
        break;
    }
    default: break;
    }
}



void updatePendingStrikes()
{
    for (size_t i = 0; i < pendingStrikes.size(); )
    {
        PendingStrike s = pendingStrikes[i];
        bool valid = s.body >= 0 && s.body < (int)bodies.size() && bodies[s.body].uid == s.uid && !bodies[s.body].destroyed;
        if (!valid) { pendingStrikes.erase(pendingStrikes.begin() + i); continue; }
        if (g_simTime - s.fireTime >= s.duration)
        {
            pendingStrikes.erase(pendingStrikes.begin() + i);
            applyImpact(s.body, s.localDir, s.weapon);
            continue;
        }
        ++i;
    }
}

