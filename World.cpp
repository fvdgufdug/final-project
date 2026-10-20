#include "Globals.h"



glm::vec3 bodyWorldPos(const Body& b, float )
{
    return b.position;
}



void spawnCollisionEffect(const glm::vec3& pos, float scaleMul, float lifetimeMul)
{
    if (collisionEffects.size() > 40) collisionEffects.erase(collisionEffects.begin());
    CollisionEffect fx;
    fx.pos = pos;
    fx.spawnTime = g_simTime;
    fx.scaleMul = scaleMul;
    fx.lifetimeMul = lifetimeMul;
    for (int k = 0; k < 8; ++k)
        fx.sparkDir[k] = glm::normalize(glm::vec3(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f) + glm::vec3(0.0001f));
    collisionEffects.push_back(fx);
}



void spawnShockwave(const glm::vec3& pos, const glm::vec3& normal, float size, float lifetime, const glm::vec3& color)
{
    if (shockwaves.size() > 30) shockwaves.erase(shockwaves.begin());
    shockwaves.push_back({ pos, normal, g_simTime, size, lifetime, color });
}



glm::vec3 localDirToWorldPoint(const Body& b, const glm::vec3& localDir)
{
    float a = glm::radians(b.rotationSpeed * g_simTime);
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), a, glm::vec3(0, 1, 0));
    return b.position + glm::vec3(rot * glm::vec4(localDir, 0.0f)) * b.scaledSize;
}



glm::vec3 worldToLocalDir(const Body& b, const glm::vec3& worldPoint)
{
    glm::vec3 n = worldPoint - b.position;
    if (glm::length(n) < 0.0001f) return glm::vec3(0.0f, 1.0f, 0.0f);
    n = glm::normalize(n);
    float a = glm::radians(b.rotationSpeed * g_simTime);
    glm::mat4 inv = glm::rotate(glm::mat4(1.0f), -a, glm::vec3(0, 1, 0));
    return glm::normalize(glm::vec3(inv * glm::vec4(n, 0.0f)));
}



glm::mat4 alignYTo(const glm::vec3& n)
{
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    float d = glm::clamp(glm::dot(up, n), -1.0f, 1.0f);
    glm::vec3 axis = glm::cross(up, n);
    if (glm::length(axis) < 0.0001f)
        return (d > 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), 3.14159265f, glm::vec3(1, 0, 0));
    return glm::rotate(glm::mat4(1.0f), acosf(d), glm::normalize(axis));
}



glm::vec3 jitterDir(const glm::vec3& d, float maxAngle)
{
    glm::vec3 r(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f);
    glm::vec3 t = glm::cross(d, r);
    if (glm::length(t) < 0.0001f) return d;
    t = glm::normalize(t);
    float a = randf() * maxAngle;
    return glm::normalize(d * cosf(a) + t * sinf(a));
}



void uploadImpacts(const Body& b, int uCount, int uImp, int uAges)
{
    int n = (int)std::min(b.impacts.size(), (size_t)kMaxImpacts);
    float data[kMaxImpacts * 4];
    float ages[kMaxImpacts];
    for (int i = 0; i < n; ++i)
    {
        data[i * 4 + 0] = b.impacts[i].dir.x;
        data[i * 4 + 1] = b.impacts[i].dir.y;
        data[i * 4 + 2] = b.impacts[i].dir.z;
        data[i * 4 + 3] = b.impacts[i].radius;
        ages[i] = std::max(0.0f, g_simTime - b.impacts[i].spawnTime);
    }
    glUniform1i(uCount, n);
    if (n > 0)
    {
        glUniform4fv(uImp, n, data);
        glUniform1fv(uAges, n, ages);
    }
}



std::string formatPopulation(double p)
{
    char b[48];
    if (p < 1000.0) return "0 (extinct)";
    if (p >= 1e9) snprintf(b, sizeof(b), "%.2f billion", p / 1e9);
    else if (p >= 1e6) snprintf(b, sizeof(b), "%.1f million", p / 1e6);
    else snprintf(b, sizeof(b), "%.0f", p);
    return b;
}


bool addBody(Body b)
{
    b.uid = g_nextUid++;
    for (int i = 9; i < (int)bodies.size(); ++i)
        if (bodies[i].destroyed && i != selectedBody) { bodies[i] = b; return true; }
    if ((int)bodies.size() >= kMaxBodies) return false;
    bodies.push_back(b);
    return true;
}



void spawnDebrisChunk(const glm::vec3& pos, const glm::vec3& vel, unsigned int rockTexture, double mass, float sizeMul)
{
    Body b;
    b.name = "Debris " + std::to_string((int)bodies.size());
    b.scaledSize = (0.12f + randf() * 0.34f) * sizeMul;
    b.shapeScale = glm::vec3(0.7f + randf() * 0.6f,
        0.7f + randf() * 0.6f,
        0.7f + randf() * 0.6f);
    b.fallbackColor = glm::vec3(0.42f + randf() * 0.15f,
        0.30f + randf() * 0.12f,
        0.22f + randf() * 0.10f);
    b.mass = mass;
    b.orbitRadius = 0.0f;
    b.rotationSpeed = (randf() - 0.5f) * 240.0f;
    b.revolutionSpeed = 0.0f;
    b.texture = rockTexture;
    b.plainColor = (randf() < 0.5f);
    b.position = pos;
    b.velocity = vel;
    addBody(b);
}



void destroyBody(int targetIndex)
{
    if (targetIndex < 0 || targetIndex >= (int)bodies.size()) return;
    if (bodies[targetIndex].destroyed) return;

    if (targetIndex == 0)
    {
        if (!bodies[0].supernovaed) supernova();
        else
        {
            spawnCollisionEffect(bodies[0].position, 6.0f, 2.5f);
            audioPlayExplosion();
            bodies[0].destroyed = true;
            bodies[0].velocity = glm::vec3(0.0f);
            statsDestroyed++;
            pushToast("The Sun's remnant is gone");
            if (selectedBody == 0) { selectedBody = -1; syncYawPitchFromFront(); }
        }
        return;
    }

    const glm::vec3    targetPos = bodies[targetIndex].position;
    const glm::vec3    targetVel = bodies[targetIndex].velocity;
    const float        targetSize = bodies[targetIndex].scaledSize;
    const double       targetMass = bodies[targetIndex].mass;
    const unsigned int rockTexture = bodies[targetIndex].texture;
    const std::string  targetName = bodies[targetIndex].name;
    const bool         wasSelected = (selectedBody == targetIndex);

    float fxScale = std::max(1.0f, targetSize * 0.7f);
    spawnCollisionEffect(targetPos, 3.2f * fxScale, 2.2f);
    spawnCollisionEffect(targetPos, 1.6f * fxScale, 1.4f);
    spawnShockwave(targetPos, glm::vec3(0.0f, 1.0f, 0.0f), targetSize * 3.0f, 2.0f, glm::vec3(1.0f, 0.6f, 0.25f));
    spawnShockwave(targetPos, glm::normalize(glm::vec3(0.6f, 0.8f, 0.2f)), targetSize * 2.2f, 1.6f, glm::vec3(1.0f, 0.85f, 0.5f));
    audioPlayExplosion();
    g_shake = std::max(g_shake, 1.6f);

    bodies[targetIndex].destroyed = true;
    bodies[targetIndex].velocity = glm::vec3(0.0f);
    bodies[targetIndex].health = 0.0f;
    bodies[targetIndex].population = 0.0;
    statsDestroyed++;
    pushToast(targetName + " destroyed!");

    int    debrisCount = 14 + (int)std::min(targetSize * 4.0f, 12.0f);
    float  spread = std::max(targetSize, 0.6f);
    double chunkMass = std::max(targetMass * 0.7 / (double)debrisCount, 0.01);
    float  sizeMul = glm::clamp(targetSize / 0.9f, 0.6f, 2.2f);
    for (int d = 0; d < debrisCount; ++d)
    {
        glm::vec3 dir = glm::normalize(glm::vec3(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f) + glm::vec3(0.0001f));
        float speed = 2.0f + randf() * 7.0f;
        spawnDebrisChunk(targetPos + dir * spread * (0.3f + randf() * 0.7f), targetVel + dir * speed, rockTexture, chunkMass, sizeMul);
    }

    if (wasSelected)
    {
        selectedBody = -1;
        if (cinematicEnabled) startCinematic(targetPos, targetSize);
        else syncYawPitchFromFront();
    }
}



void killBody(int idx)
{
    if (idx < 0 || idx >= (int)bodies.size() || bodies[idx].destroyed) return;
    if (idx < 9) { destroyBody(idx); return; }
    spawnCollisionEffect(bodies[idx].position, 0.7f);
    audioPlayCollision();
    bodies[idx].destroyed = true;
    bodies[idx].velocity = glm::vec3(0.0f);
    bodies[idx].health = 0.0f;
}



void supernova()
{
    if (bodies.empty() || bodies[0].destroyed || bodies[0].supernovaed) return;
    const glm::vec3 c = bodies[0].position;

    spawnCollisionEffect(c, 14.0f, 3.5f);
    spawnCollisionEffect(c, 8.0f, 2.2f);
    for (int k = 0; k < 4; ++k)
    {
        glm::vec3 n = glm::normalize(glm::vec3(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f) + glm::vec3(0.0001f));
        spawnShockwave(c, n, 8.0f + k * 3.0f, 2.6f + k * 0.5f, glm::vec3(0.9f, 0.8f, 1.0f));
    }
    audioPlayExplosion(); audioPlayExplosion();
    g_shake = 3.0f;
    pushToast("SUPERNOVA!");

    bodies[0].supernovaed = true;
    bodies[0].mass *= 0.15;
    bodies[0].scaledSize *= 0.35f;
    bodies[0].plainColor = true;
    bodies[0].fallbackColor = glm::vec3(0.75f, 0.85f, 1.0f);
    bodies[0].health = 100.0f;
    bodies[0].impacts.clear();

    std::vector<int> doomed;
    for (int i = 1; i < (int)bodies.size(); ++i)
    {
        Body& b = bodies[i];
        if (b.destroyed) continue;
        glm::vec3 d = b.position - c;
        float dist = glm::length(d);
        if (dist < 0.001f) continue;

        b.velocity += (d / dist) * (7.0f * expf(-dist / 40.0f));
        float dmg = 220.0f * expf(-dist / 30.0f);
        b.impacts.push_back(Impact{ worldToLocalDir(b, c), 1.2f, g_simTime });
        if ((int)b.impacts.size() > kMaxImpacts) b.impacts.erase(b.impacts.begin());
        if (b.population > 0.0) b.population *= std::max(0.0, 1.0 - dmg / 100.0);
        b.health -= dmg;
        if (b.health <= 0.0f) doomed.push_back(i);
    }
    for (int idx : doomed) killBody(idx);

    if (selectedBody == 0) selectedBody = -1;
    if (cinematicEnabled) startCinematic(c, 5.0f);
    else syncYawPitchFromFront();
}



void fireBattleStationAt(int targetIndex)
{
    if (battleStationCooldownRemaining > 0.0f) return;
    if (targetIndex <= 0 || targetIndex >= (int)bodies.size()) return;
    if (bodies[targetIndex].destroyed) return;

    LaserBeam lb; lb.from = battleStationPos; lb.to = bodies[targetIndex].position; lb.spawnTime = g_simTime;
    laserBeams.push_back(lb);
    audioPlayLaser();
    battleStationCooldownRemaining = kBattleStationCooldown;
    destroyBody(targetIndex);
}



void damageFromCollision(int idx, int otherIdx, float dmg)
{
    if (idx < 0 || idx >= (int)bodies.size() || bodies[idx].destroyed) return;
    Body& b = bodies[idx];
    if (b.scaledSize >= 0.4f)
    {
        b.impacts.push_back(Impact{ worldToLocalDir(b, bodies[otherIdx].position), 0.25f, g_simTime });
        if ((int)b.impacts.size() > kMaxImpacts) b.impacts.erase(b.impacts.begin());
    }
    if (b.population > 0.0) b.population *= std::max(0.0, 1.0 - (double)dmg / 100.0 * 0.9);
    b.health -= dmg;
    if (b.health <= 0.0f) killBody(idx);
}



void spawnProjectile(const glm::vec3& pos, const glm::vec3& vel, bool isRocket, bool isBlackHole)
{
    Body b;
    if (isBlackHole)
    {
        b.name = "Black Hole " + std::to_string((int)bodies.size());
        b.scaledSize = 0.35f;
        b.fallbackColor = glm::vec3(0.02f, 0.02f, 0.03f);
        b.mass = 20000.0;
        b.plainColor = true;
        b.isBlackHole = true;
    }
    else if (isRocket)
    {
        b.name = "Rocket " + std::to_string((int)bodies.size());
        b.scaledSize = 0.16f;
        b.shapeScale = glm::vec3(0.55f, 0.55f, 2.0f);
        b.fallbackColor = glm::vec3(0.85f, 0.15f, 0.1f);
        b.mass = 0.03;
        b.plainColor = true;
    }
    else
    {
        b.name = "Comet " + std::to_string((int)bodies.size());
        b.scaledSize = 0.18f;
        b.fallbackColor = glm::vec3(0.6f, 0.55f, 0.5f);
        b.mass = 0.02;
    }
    b.orbitRadius = 0.0f;
    b.rotationSpeed = 40.0f;
    b.revolutionSpeed = 0.0f;
    b.texture = g_asteroidTexture;
    b.position = pos;
    b.velocity = vel;
    addBody(b);
}



void computeAccelerations(const std::vector<Body>& srcBodies, std::vector<glm::vec3>& outAccel)
{
    int n = (int)srcBodies.size();
    outAccel.assign(n, glm::vec3(0.0f));
    const float softening = 0.35f;

    for (int i = 0; i < n; ++i)
    {
        if (srcBodies[i].destroyed) continue;
        for (int j = 0; j < n; ++j)
        {
            if (i == j || srcBodies[j].destroyed) continue;
            glm::vec3 diff = srcBodies[j].position - srcBodies[i].position;
            float distSq = glm::dot(diff, diff) + softening * softening;
            float dist = sqrtf(distSq);
            float forceMag = (float)(kG * srcBodies[j].mass) / distSq;
            outAccel[i] += (diff / dist) * forceMag;
        }
    }
}



void stepPhysics(float dt)
{
    int n = (int)bodies.size();
    if (n == 0 || dt <= 0.0f) return;

    std::vector<glm::vec3> accelOld;
    computeAccelerations(bodies, accelOld);

    for (int i = 0; i < n; ++i)
    {
        if (bodies[i].destroyed) continue;
        bodies[i].position += bodies[i].velocity * dt + 0.5f * accelOld[i] * dt * dt;
    }

    std::vector<glm::vec3> accelNew;
    computeAccelerations(bodies, accelNew);

    for (int i = 0; i < n; ++i)
    {
        if (bodies[i].destroyed) continue;
        bodies[i].velocity += 0.5f * (accelOld[i] + accelNew[i]) * dt;
    }

    for (int i = 0; i < n; ++i)
    {
        if (bodies[i].destroyed) continue;
        for (int j = i + 1; j < n; ++j)
        {
            if (bodies[i].destroyed) break;
            if (bodies[j].destroyed) continue;
            float rSum = bodies[i].scaledSize + bodies[j].scaledSize;
            glm::vec3 diff = bodies[j].position - bodies[i].position;
            float dist = glm::length(diff);
            if (dist < rSum && dist > 0.0001f)
            {
                glm::vec3 normal = diff / dist;

                if (bodies[i].isBlackHole || bodies[j].isBlackHole)
                {
                    int holeIdx = bodies[i].isBlackHole ? i : j;
                    int preyIdx = bodies[i].isBlackHole ? j : i;
                    if (!bodies[preyIdx].isBlackHole)
                    {
                        spawnCollisionEffect(bodies[preyIdx].position, 0.6f);
                        audioPlayCollision();
                        bodies[preyIdx].destroyed = true;
                        bodies[preyIdx].velocity = glm::vec3(0.0f);
                        bodies[holeIdx].mass += bodies[preyIdx].mass * 0.05;
                        statsCollisions++;
                        continue;
                    }
                }

                if (i == 0 || j == 0)
                {
                    int preyIdx = (i == 0) ? j : i;
                    spawnCollisionEffect(bodies[preyIdx].position, 0.8f);
                    audioPlayCollision();
                    bodies[preyIdx].destroyed = true;
                    bodies[preyIdx].velocity = glm::vec3(0.0f);
                    bodies[0].mass += bodies[preyIdx].mass;
                    statsCollisions++;
                    continue;
                }

                float relSpeed = glm::length(bodies[j].velocity - bodies[i].velocity);
                if (relSpeed < kMergeSpeed && !bodies[i].isBlackHole && !bodies[j].isBlackHole)
                {
                    int big = (bodies[i].mass >= bodies[j].mass) ? i : j;
                    int small = (big == i) ? j : i;
                    float mB = (float)bodies[big].mass, mS = (float)bodies[small].mass, mT = mB + mS;
                    bodies[big].velocity = (bodies[big].velocity * mB + bodies[small].velocity * mS) / mT;
                    bodies[big].position = (bodies[big].position * mB + bodies[small].position * mS) / mT;
                    float rb = bodies[big].scaledSize, rs = bodies[small].scaledSize;
                    bodies[big].scaledSize = cbrtf(rb * rb * rb + rs * rs * rs);
                    bodies[big].mass = (double)mT;
                    bodies[small].destroyed = true;
                    bodies[small].velocity = glm::vec3(0.0f);
                    spawnCollisionEffect(bodies[small].position, 0.5f);
                    audioPlayCollision();
                    statsCollisions++;
                    continue;
                }

                float overlap = rSum - dist;
                float invMassI = 1.0f / (float)bodies[i].mass;
                float invMassJ = 1.0f / (float)bodies[j].mass;
                float totalInv = invMassI + invMassJ;

                bodies[i].position -= normal * overlap * (invMassI / totalInv);
                bodies[j].position += normal * overlap * (invMassJ / totalInv);

                glm::vec3 relVel = bodies[j].velocity - bodies[i].velocity;
                float velAlongNormal = glm::dot(relVel, normal);
                if (velAlongNormal < 0.0f)
                {
                    float restitution = 0.5f;
                    float impulseMag = -(1.0f + restitution) * velAlongNormal / totalInv;
                    glm::vec3 impulse = impulseMag * normal;
                    bodies[i].velocity -= impulse * invMassI;
                    bodies[j].velocity += impulse * invMassJ;

                    spawnCollisionEffect(bodies[i].position + normal * (bodies[i].scaledSize));

                    float crashSpeed = -velAlongNormal;

                    if (crashSpeed > kCrashSpeed && (i < 9 || j < 9))
                    {
                        float mi = (float)bodies[i].mass, mj = (float)bodies[j].mass;
                        float dmgI = (crashSpeed - kCrashSpeed) * 15.0f * (mj / (mi + mj));
                        float dmgJ = (crashSpeed - kCrashSpeed) * 15.0f * (mi / (mi + mj));
                        damageFromCollision(i, j, dmgI);
                        damageFromCollision(j, i, dmgJ);
                    }

                    audioPlayCollision();
                    statsCollisions++;
                }
            }
        }
    }
}



std::vector<glm::vec3> predictTrajectory(const glm::vec3& startPos, const glm::vec3& startVel, int steps, float dt)
{
    std::vector<glm::vec3> path;
    path.reserve(steps);

    std::vector<Body> sim = bodies;
    Body ghost;
    ghost.position = startPos;
    ghost.velocity = startVel;
    ghost.mass = 0.0;
    sim.push_back(ghost);
    size_t ghostIdx = sim.size() - 1;

    std::vector<glm::vec3> accelOld;
    computeAccelerations(sim, accelOld);

    for (int s = 0; s < steps; ++s)
    {
        for (size_t i = 0; i < sim.size(); ++i)
        {
            if (sim[i].destroyed) continue;
            sim[i].position += sim[i].velocity * dt + 0.5f * accelOld[i] * dt * dt;
        }
        std::vector<glm::vec3> accelNew;
        computeAccelerations(sim, accelNew);
        for (size_t i = 0; i < sim.size(); ++i)
        {
            if (sim[i].destroyed) continue;
            sim[i].velocity += 0.5f * (accelOld[i] + accelNew[i]) * dt;
        }
        accelOld.swap(accelNew);
        path.push_back(sim[ghostIdx].position);
    }
    return path;
}

