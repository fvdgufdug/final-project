#include "Globals.h"



std::string saveSlotPath(int slot)
{
    return "save_slot_" + std::to_string(slot) + ".txt";
}



bool saveSimulation(const char* path)
{
    std::ofstream f(path);
    if (!f.is_open()) { std::cout << "Could not open " << path << " for writing\n"; return false; }
    int count = std::min((int)bodies.size(), 9);
    f << count << "\n";
    for (int i = 0; i < count; ++i)
    {
        const Body& b = bodies[i];
        f << b.mass << " "
            << b.position.x << " " << b.position.y << " " << b.position.z << " "
            << b.velocity.x << " " << b.velocity.y << " " << b.velocity.z << " "
            << (b.destroyed ? 1 : 0) << " " << b.scaledSize << "\n";
    }
    f << "EXTRA\n";
    for (int i = 0; i < count; ++i)
    {
        const Body& b = bodies[i];
        f << b.health << " " << b.population << " " << (b.supernovaed ? 1 : 0) << " " << (b.isStar ? 1 : 0)
            << " " << b.impacts.size();
        for (const Impact& im : b.impacts)
            f << " " << im.dir.x << " " << im.dir.y << " " << im.dir.z << " " << im.radius << " " << (g_simTime - im.spawnTime);
        f << "\n";
    }
    std::cout << "Saved " << count << " bodies to " << path << "\n";
    return true;
}



bool loadSimulation(const char* path)
{
    std::ifstream f(path);
    if (!f.is_open()) { std::cout << "No save file found at " << path << "\n"; return false; }
    int count = 0;
    f >> count;
    count = std::min(count, (int)bodies.size());
    for (int i = 0; i < count; ++i)
    {
        Body& b = bodies[i];
        int destroyedFlag = 0;
        f >> b.mass >> b.position.x >> b.position.y >> b.position.z
            >> b.velocity.x >> b.velocity.y >> b.velocity.z >> destroyedFlag >> b.scaledSize;
        b.destroyed = (destroyedFlag != 0);
        b.trail.clear();

        b.health = 100.0f;
        b.impacts.clear();
        b.supernovaed = false;
        b.isStar = (i == 0);
        b.population = (i == 3 && !b.destroyed) ? 8.0e9 : 0.0;
    }

    std::string tag;
    if (f >> tag && tag == "EXTRA")
    {
        for (int i = 0; i < count; ++i)
        {
            Body& b = bodies[i];
            int sn = 0, st = 0;
            size_t ic = 0;
            if (!(f >> b.health >> b.population >> sn >> st >> ic)) break;
            b.supernovaed = (sn != 0);
            b.isStar = (st != 0) || (i == 0);
            b.impacts.clear();
            for (size_t k = 0; k < ic; ++k)
            {
                Impact im; float age = 0.0f;
                f >> im.dir.x >> im.dir.y >> im.dir.z >> im.radius >> age;
                im.spawnTime = g_simTime - age;
                if ((int)b.impacts.size() < kMaxImpacts) b.impacts.push_back(im);
            }
        }
    }

    if (count > 0)
    {
        Body& s = bodies[0];
        s.plainColor = s.supernovaed;
        s.fallbackColor = s.supernovaed ? glm::vec3(0.75f, 0.85f, 1.0f) : glm::vec3(1.0f, 0.85f, 0.2f);
    }
    pendingStrikes.clear();
    shockwaves.clear();
    std::cout << "Loaded " << count << " bodies from " << path << "\n";
    return true;
}

