#include "Globals.h"



void runScenario(int id)
{
    if (bodies.size() < 9) return;
    switch (id)
    {
    case 1:
    {
        Body& e = bodies[3];
        Body& m = bodies[4];
        if (e.destroyed || m.destroyed) { pushToast("Earth or Mars is already gone"); return; }
        glm::vec3 radial = e.position - bodies[0].position;
        radial.y = 0.0f;
        radial = (glm::length(radial) > 0.001f) ? glm::normalize(radial) : glm::vec3(1.0f, 0.0f, 0.0f);
        m.position = e.position + radial * 8.0f;
        e.velocity = radial * 10.0f;
        m.velocity = -radial * 10.0f;
        e.trail.clear(); m.trail.clear();
        tourActive = false;
        selectedBody = 3;
        focusOffset = glm::normalize(glm::vec3(0.5f, 0.6f, 0.8f)) * 14.0f;
        pushToast("Earth and Mars: collision course!");
        break;
    }
    case 2:
    {
        Body& j = bodies[5];
        if (j.destroyed) { pushToast("Jupiter is already gone"); return; }
        j.mass *= 60.0;
        j.isStar = true;
        j.atmoStrength = 0.0f;
        j.health = 100.0f;
        j.impacts.clear();
        tourActive = false;
        selectedBody = 5;
        focusOffset = glm::normalize(glm::vec3(0.5f, 0.35f, 0.8f)) * 16.0f;
        pushToast("Jupiter ignites!");
        break;
    }
    case 3:
        supernova();
        break;
    case 4:
    {
        glm::vec3 start(-75.0f, 2.0f, 25.0f);
        glm::vec3 aim(0.0f, 0.0f, -15.0f);
        spawnProjectile(start, glm::normalize(aim - start) * 9.0f, false, true);
        pushToast("Rogue black hole incoming!");
        break;
    }
    default: break;
    }
}

