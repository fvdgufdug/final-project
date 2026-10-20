#include "Globals.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#define _chdir chdir
#endif



int main()
{
    _chdir("./helloWorldGLFW");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "CSE3200 - Group 1 - Solar System", primaryMonitor, nullptr);
    if (!window) { std::cout << "Failed to create window\n";glfwTerminate();return -1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "GLAD fail\n";return -1; }

    showLoading(window, "Loading Solar System...");

    audioInit();
    audioSetAmbient(true);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    unsigned int prog = buildShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(prog);

    int uModel = glGetUniformLocation(prog, "model");
    int uView = glGetUniformLocation(prog, "view");
    int uProjection = glGetUniformLocation(prog, "projection");
    int uColor = glGetUniformLocation(prog, "objectColor");
    int uSunPos = glGetUniformLocation(prog, "sunWorldPos");
    int uUseTex = glGetUniformLocation(prog, "useTexture");
    int uUseLit = glGetUniformLocation(prog, "useLighting");
    int uTex = glGetUniformLocation(prog, "objectTexture");
    int uUVOffset = glGetUniformLocation(prog, "uvOffset");
    int uAlpha = glGetUniformLocation(prog, "alpha");

    int uImpactCount = glGetUniformLocation(prog, "impactCount");
    int uImpacts = glGetUniformLocation(prog, "impacts");
    int uImpactAges = glGetUniformLocation(prog, "impactAges");
    int uCloudMode = glGetUniformLocation(prog, "cloudMode");
    int uCloudSeed = glGetUniformLocation(prog, "cloudSeed");

    glUniform1i(uTex, 0);
    glUniform1f(uAlpha, 1.0f);
    glUniform1i(uImpactCount, 0);
    glUniform1i(uCloudMode, 0);

    std::vector<float> sv; std::vector<unsigned int> si;
    buildSphere(64, 64, sv, si);
    GpuMesh sphereMesh = uploadIndexed(sv, si, 8, true);

    std::vector<float> rv; std::vector<unsigned int> ri;
    buildRing(1.8f, 2.8f, 100, rv, ri);
    GpuMesh ringMesh = uploadIndexed(rv, ri, 8, true);

    std::vector<float> ov;
    buildOrbitCircle(100, ov);
    GpuMesh orbitMesh = uploadVertexOnly(ov, 6);

    std::vector<float> ssv;
    buildShootingStarLine(ssv);
    GpuMesh ssMesh = uploadVertexOnly(ssv, 3);

    const char* vs2D = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uOrtho;
uniform float uScale;
out vec4 vColor;
void main()
{
    vColor = aColor;
    gl_Position = uOrtho * vec4(aPos.xy * uScale, 0.0, 1.0);
}
)";
    const char* fs2D = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)";
    unsigned int uiProg = buildShaderProgram(vs2D, fs2D);
    int uOrtho = glGetUniformLocation(uiProg, "uOrtho");
    int uUiScale = glGetUniformLocation(uiProg, "uScale");

    const int MAX_UI_QUADS = 500;
    std::vector<unsigned int> uiIdx;
    uiIdx.reserve(MAX_UI_QUADS * 6);
    for (int q = 0; q < MAX_UI_QUADS; ++q)
    {
        unsigned int base = q * 4;
        uiIdx.push_back(base + 0); uiIdx.push_back(base + 1); uiIdx.push_back(base + 2);
        uiIdx.push_back(base + 2); uiIdx.push_back(base + 3); uiIdx.push_back(base + 0);
    }

    unsigned int textVAO, textVBO, textEBO;
    glGenVertexArrays(1, &textVAO); glGenBuffers(1, &textVBO); glGenBuffers(1, &textEBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_UI_QUADS * 4 * 16, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiIdx.size() * sizeof(unsigned int), uiIdx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void*)12); glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int panelVAO, panelVBO, panelEBO;
    glGenVertexArrays(1, &panelVAO); glGenBuffers(1, &panelVBO); glGenBuffers(1, &panelEBO);
    glBindVertexArray(panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER, panelVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 16, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, panelEBO);
    unsigned int panelIdx[6] = { 0,1,2, 2,3,0 };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(panelIdx), panelIdx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void*)12); glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    showLoading(window, "Loading textures: stars...");
    unsigned int texStars = loadTexture("textures/stars.jpg");

    showLoading(window, "Loading textures: sun...");
    unsigned int texSun = loadTexture("textures/sun.jpg");

    showLoading(window, "Loading textures: mercury...");
    unsigned int texMercury = loadTexture("textures/mercury.jpg");

    showLoading(window, "Loading textures: venus...");
    unsigned int texVenus = loadTexture("textures/venus.jpg");

    showLoading(window, "Loading textures: earth...");
    unsigned int texEarth = loadTexture("textures/earth.jpg");

    showLoading(window, "Loading textures: mars...");
    unsigned int texMars = loadTexture("textures/mars.jpg");

    showLoading(window, "Loading textures: jupiter...");
    unsigned int texJupiter = loadTexture("textures/jupiter.jpg");

    showLoading(window, "Loading textures: saturn...");
    unsigned int texSaturn = loadTexture("textures/saturn.jpg");
    unsigned int texSaturnRing = loadTexture("textures/saturn_ring.png");

    showLoading(window, "Loading textures: uranus...");
    unsigned int texUranus = loadTexture("textures/uranus.jpg");

    showLoading(window, "Loading textures: neptune...");
    unsigned int texNeptune = loadTexture("textures/neptune.jpg");

    showLoading(window, "Loading textures: moon...");
    unsigned int texMoon = loadTexture("textures/moon.jpg");

    showLoading(window, "Loading textures: asteroid...");
    unsigned int texAsteroid = loadTexture("textures/asteroid.jpg");
    g_asteroidTexture = texAsteroid;

    glfwSetWindowTitle(window, "CSE3200 - Group 1 - Solar System");

    bodies = {
        {"Sun",    4.50f, 0.0f, 5.0f, 0.0f,  glm::vec3(1.0f,0.85f,0.2f),texSun    },
        {"Mercury",0.45f, 16.0f, 0.4f, 7.6f,  glm::vec3(0.7f,0.7f, 0.7f),texMercury},
        {"Venus",  0.85f, 19.0f,0.15f, 7.2f,  glm::vec3(0.9f,0.7f, 0.3f),texVenus  },
        {"Earth",  0.90f, 22.5f, 0.5f, 7.0f,  glm::vec3(0.2f,0.4f, 1.0f),texEarth  },
        {"Mars",   0.65f, 26.0f,0.45f, 6.8f,  glm::vec3(0.8f,0.3f, 0.2f),texMars   },

        {"Jupiter",2.20f, 36.5f, 0.9f, 6.4f,  glm::vec3(0.8f,0.6f, 0.4f),texJupiter},
        {"Saturn", 1.90f, 45.0f, 0.8f, 6.3f,  glm::vec3(0.9f,0.8f, 0.5f),texSaturn },
        {"Uranus", 1.30f, 51.5f, 0.6f, 6.2f,  glm::vec3(0.5f,0.8f, 0.9f),texUranus },
        {"Neptune",1.20f, 58.0f,0.55f,6.15f,  glm::vec3(0.3f,0.5f, 0.9f),texNeptune},
    };

    {

        double startMasses[] = { 300000.0, 0.055, 0.815, 1.0, 0.107, 317.8, 95.2, 14.5, 17.1 };
        for (int i = 0; i < (int)bodies.size() && i < 9; ++i)
            bodies[i].mass = startMasses[i];

        double desiredEarthOmega = glm::radians(7.0);
        double earthR = bodies[3].orbitRadius;
        kG = (desiredEarthOmega * desiredEarthOmega) * (earthR * earthR * earthR) / bodies[0].mass;

        for (Body& b : bodies)
        {
            b.position = glm::vec3(b.orbitRadius, 0.0f, 0.0f);
            if (b.orbitRadius > 0.0f)
            {
                double v = std::sqrt(kG * bodies[0].mass / (double)b.orbitRadius);
                b.velocity = glm::vec3(0.0f, 0.0f, (float)v);
            }
        }
    }

    {
        bodies[0].isStar = true;
        bodies[3].population = 8.0e9;
        bodies[3].hasClouds = true;
        auto setAtmo = [&](int i, const glm::vec3& c, float s) { bodies[i].atmoColor = c; bodies[i].atmoStrength = s; };
        setAtmo(2, glm::vec3(1.0f, 0.80f, 0.40f), 1.0f);
        setAtmo(3, glm::vec3(0.35f, 0.60f, 1.0f), 1.0f);
        setAtmo(4, glm::vec3(0.90f, 0.50f, 0.35f), 0.35f);
        setAtmo(5, glm::vec3(0.90f, 0.75f, 0.55f), 0.5f);
        setAtmo(6, glm::vec3(0.95f, 0.85f, 0.60f), 0.5f);
        setAtmo(7, glm::vec3(0.50f, 0.85f, 0.95f), 0.7f);
        setAtmo(8, glm::vec3(0.30f, 0.50f, 1.0f), 0.7f);
    }

    std::vector<Body> initialBodies = bodies;

    const float     moonSize = 0.45f, moonOrbitRadius = 1.8f;
    const float     moonSpinSpeed = 1.0f, moonRevSpeed = 10.0f;
    const glm::vec3 moonFallback = glm::vec3(0.8f, 0.8f, 0.85f);

    struct Satellite
    {
        std::string parentName;
        float orbitRadius, orbitSpeed, spinSpeed, size;
        glm::vec3 fallbackColor;
    };
    std::vector<Satellite> satellites = {

        { "Jupiter", 3.2f, 90.0f, 20.0f, 0.12f, glm::vec3(0.85f, 0.75f, 0.45f) },
        { "Jupiter", 4.0f, 70.0f, 15.0f, 0.11f, glm::vec3(0.75f, 0.80f, 0.85f) },
        { "Jupiter", 5.0f, 50.0f, 12.0f, 0.16f, glm::vec3(0.55f, 0.50f, 0.45f) },
        { "Jupiter", 6.2f, 35.0f, 10.0f, 0.15f, glm::vec3(0.35f, 0.32f, 0.30f) },

        { "Saturn",  6.4f, 55.0f, 14.0f, 0.10f, glm::vec3(0.80f, 0.78f, 0.72f) },
        { "Saturn",  7.6f, 40.0f, 8.0f,  0.18f, glm::vec3(0.85f, 0.65f, 0.35f) },
        { "Saturn",  8.6f, 30.0f, 18.0f, 0.08f, glm::vec3(0.95f, 0.95f, 0.95f) },
    };

    std::vector<Asteroid> asteroids;
    srand(123);
    for (int i = 0;i < 600;++i)
    {
        Asteroid a;
        a.orbitRadius = 29.0f + randf() * 4.0f;
        a.orbitAngle = randf() * 360.0f;
        a.orbitSpeed = 1.5f + randf() * 1.5f;
        a.size = 0.04f + randf() * 0.14f;
        a.yOffset = (randf() - 0.5f) * 2.0f;
        asteroids.push_back(a);
    }

    const int MAX_SS = 20;
    std::vector<ShootingStar> shootingStars(MAX_SS);
    for (auto& ss : shootingStars) ss.active = false;
    float nextSpawnTime = 2.0f;
    srand(999);

    auto spawnShootingStar = [&](float currentTime)
        {

            for (auto& ss : shootingStars)
            {
                if (!ss.active)
                {

                    float theta = randf() * 6.2832f;
                    float phi = randf() * 3.1416f;
                    float r = 80.0f + randf() * 30.0f;
                    ss.origin = glm::vec3(r * sinf(phi) * cosf(theta), r * cosf(phi) * 0.4f, r * sinf(phi) * sinf(theta));

                    glm::vec3 inward = glm::normalize(-ss.origin);
                    glm::vec3 perp = glm::normalize(glm::vec3(inward.z, 0, -inward.x));
                    ss.direction = glm::normalize(inward + perp * (randf() - 0.5f) * 0.5f);
                    ss.speed = 60.0f + randf() * 60.0f;
                    ss.length = 4.0f + randf() * 6.0f;
                    ss.spawnTime = currentTime;
                    ss.lifetime = 0.8f + randf() * 0.8f;
                    ss.active = true;
                    break;
                }
            }
        };

    lastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        g_wallTime += deltaTime;

        fpsAccumTime += deltaTime;
        fpsFrameCount++;
        if (fpsAccumTime >= 0.5f)
        {
            displayFps = (float)fpsFrameCount / fpsAccumTime;
            fpsAccumTime = 0.0f;
            fpsFrameCount = 0;
        }

        processInput(window);

        if (selectedBody >= 0 && selectedBody < (int)bodies.size() && bodies[selectedBody].destroyed && !cinematicActive)
        {
            syncYawPitchFromFront();
            selectedBody = -1;
        }

        if (selectedBody >= 0 && selectedBody < (int)bodies.size())
        {
            Body& b = bodies[selectedBody];
            if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) b.mass *= (1.0 + 1.5 * (double)deltaTime);
            if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)  b.mass *= (1.0 - 1.5 * (double)deltaTime);
            if (b.mass < 0.01) b.mass = 0.01;

            if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) b.scaledSize *= (1.0f + 1.0f * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS)  b.scaledSize *= (1.0f - 1.0f * deltaTime);
            if (b.scaledSize < 0.05f) b.scaledSize = 0.05f;
            focusMinDist = b.scaledSize * 1.6f + 0.5f;
        }

        if (battleStationCooldownRemaining > 0.0f)
        {
            battleStationCooldownRemaining -= deltaTime;
            if (battleStationCooldownRemaining < 0.0f) battleStationCooldownRemaining = 0.0f;
        }

        bool kNow = (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS);
        if (kNow && !kPressedLastFrame && selectedBody > 0 && selectedBody < (int)bodies.size() && !bodies[selectedBody].destroyed)
            fireBattleStationAt(selectedBody);
        kPressedLastFrame = kNow;

        g_laserTimer -= deltaTime;
        if (currentWeapon == W_LASER && !paused && g_laserTimer <= 0.0f &&
            selectedBody >= 0 && selectedBody < (int)bodies.size() && !bodies[selectedBody].destroyed &&
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            glm::vec3 ro, rd; float t;
            if (screenToWorldRay(window, ro, rd) &&
                raySphereIntersect(ro, rd, bodies[selectedBody].position, bodies[selectedBody].scaledSize, t))
            {
                glm::vec3 hit = ro + rd * t;
                LaserBeam lb; lb.from = battleStationPos; lb.to = hit; lb.spawnTime = g_simTime;
                laserBeams.push_back(lb);
                applyImpact(selectedBody, worldToLocalDir(bodies[selectedBody], hit), W_LASER);
                g_laserTimer = 0.08f;
            }
        }

        if (saveRequested)
        {
            bool ok = saveSimulation(saveSlotPath(activeSaveSlot).c_str());
            pushToast(ok ? ("Saved to slot " + std::to_string(activeSaveSlot))
                : ("Could not save slot " + std::to_string(activeSaveSlot)));
            saveRequested = false;
        }
        if (loadRequested)
        {
            bool ok = loadSimulation(saveSlotPath(activeSaveSlot).c_str());
            pushToast(ok ? ("Loaded slot " + std::to_string(activeSaveSlot))
                : ("No save found in slot " + std::to_string(activeSaveSlot)));
            loadRequested = false;
        }

        if (resetRequested)
        {
            bodies = initialBodies;
            selectedBody = -1;
            tourActive = false;
            collisionEffects.clear();

            pendingStrikes.clear();
            shockwaves.clear();
            laserBeams.clear();
            cinematicActive = false;
            g_slowMo = 1.0f;
            g_shake = 0.0f;
            slingCharging = false;
            resetRequested = false;
            pushToast("Simulation reset");
        }

        if (scenarioRequested != 0)
        {
            runScenario(scenarioRequested);
            scenarioRequested = 0;
        }

        {
            float slowTarget = cinematicActive ? 0.2f : 1.0f;
            g_slowMo += (slowTarget - g_slowMo) * std::min(1.0f, 5.0f * deltaTime);
            g_shake *= expf(-5.0f * deltaTime);
            if (g_shake < 0.001f) g_shake = 0.0f;
        }
        const float effScale = timeScale * g_slowMo;

        if (!paused) g_simTime += deltaTime * effScale;
        if (!paused)
        {

            float physicsDt = deltaTime * effScale;
            const float kMaxSubstep = 1.0f / 240.0f;
            int substeps = (int)ceilf(physicsDt / kMaxSubstep);
            substeps = glm::clamp(substeps, 1, 32);
            float stepDt = physicsDt / (float)substeps;
            for (int s = 0; s < substeps; ++s) stepPhysics(stepDt);
        }

        updatePendingStrikes();

        if (g_simTime >= nextSpawnTime)
        {
            spawnShootingStar(g_simTime);
            nextSpawnTime = g_simTime + 0.5f + randf() * 2.5f;
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH == 0) fbH = 1;

        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        float uiScale = (winW > 0) ? (float)fbW / (float)winW : 1.0f;

        if (tourActive && !bodies.empty())
        {
            if (!paused) tourTimer += deltaTime;
            if (tourIndex < 0 || tourTimer >= kTourDwellSeconds)
            {
                int guard = 0;
                do {
                    tourIndex = (tourIndex + 1) % (int)bodies.size();
                    ++guard;
                } while (bodies[tourIndex].destroyed && guard <= (int)bodies.size());
                tourTimer = 0.0f;
                selectedBody = tourIndex;
                glm::vec3 tp = bodyWorldPos(bodies[tourIndex], g_simTime);
                float dist = bodies[tourIndex].scaledSize * 8.0f;
                if (dist < 6.0f) dist = 6.0f;
                glm::vec3 dir = glm::normalize(glm::vec3(0.6f, 0.35f, 0.6f));
                focusOffset = dir * dist;
            }
        }

        if (cinematicActive)
        {
            cinematicTimer -= deltaTime;
            glm::mat4 spin = glm::rotate(glm::mat4(1.0f), glm::radians(22.0f * deltaTime), glm::vec3(0, 1, 0));
            cinematicOffset = glm::vec3(spin * glm::vec4(cinematicOffset, 0.0f));
            cameraPos = cinematicCenter + cinematicOffset;
            cameraFront = glm::normalize(cinematicCenter - cameraPos);
            if (cinematicTimer <= 0.0f) cancelCinematic();
        }
        else if (selectedBody >= 0 && selectedBody < (int)bodies.size())
        {
            glm::vec3 focusPos = bodyWorldPos(bodies[selectedBody], g_simTime);
            cameraPos = focusPos + focusOffset;
            cameraFront = glm::normalize(focusPos - cameraPos);
        }

        glm::vec3 shakeOff(0.0f);
        if (g_shake > 0.0f)
            shakeOff = glm::vec3(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f) * g_shake * 0.6f;
        glm::mat4 view = glm::lookAt(cameraPos + shakeOff, cameraPos + shakeOff + cameraFront, cameraUp);
        glm::mat4 proj = glm::perspective(glm::radians(fov), (float)fbW / fbH, 0.1f, 200.0f);
        g_lastView = view; g_lastProj = proj; g_fbW = fbW; g_fbH = fbH;

        hoveredBody = -1;
        if (!cursorDisabled)
        {
            hoveredBody = sidebarHitTest(window);
            if (hoveredBody < 0)
            {
                glm::vec3 ro, rd;
                if (screenToWorldRay(window, ro, rd))
                {
                    float bestT = 1e9f;
                    for (int i = 0; i < (int)bodies.size(); ++i)
                    {
                        if (bodies[i].destroyed) continue;
                        glm::vec3 pos = bodyWorldPos(bodies[i], g_simTime);
                        float pickRadius = bodies[i].scaledSize * 1.3f;
                        float t;
                        if (raySphereIntersect(ro, rd, pos, pickRadius, t) && t < bestT)
                        {
                            bestT = t;
                            hoveredBody = i;
                        }
                    }
                }
            }
        }

        glUseProgram(prog);
        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjection, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(uSunPos, 0, 0, 0);

        {
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_FALSE);

            glm::mat4 sky = glm::scale(glm::mat4(1.0f), glm::vec3(150.0f));

            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(sky));
            glUniform1i(uUseTex, GL_TRUE);
            glUniform1i(uUseLit, GL_FALSE);
            glUniform1f(uUVOffset, 0.0f);
            glUniform1f(uAlpha, 1.0f);
            glUniform3f(uColor, 1, 1, 1);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texStars);

            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

            glDepthMask(GL_TRUE);
        }

        glm::vec3 earthPos(0.0f);
        bool earthAlive = false;

        for (int bi = 0; bi < (int)bodies.size(); ++bi)
        {
            Body& body = bodies[bi];
            if (body.destroyed) continue;
            glm::vec3 pos = body.position;

            {
                const float kTrailSampleInterval = 0.15f;
                const size_t kTrailMaxPoints = 50;
                if (!paused)
                {
                    body.trailTimer += deltaTime * effScale;
                    if (body.trailTimer >= kTrailSampleInterval)
                    {
                        body.trailTimer = 0.0f;
                        body.trail.push_back(pos);
                        if (body.trail.size() > kTrailMaxPoints) body.trail.erase(body.trail.begin());
                    }
                }

                if (body.trail.size() >= 2)
                {
                    glm::mat4 identity = glm::mat4(1.0f);
                    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
                    glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                    glUniform1f(uUVOffset, 0.0f);
                    glUniform3f(uColor, body.fallbackColor.r, body.fallbackColor.g, body.fallbackColor.b);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                    size_t n = body.trail.size();
                    glBindVertexArray(ssMesh.VAO);
                    glBindBuffer(GL_ARRAY_BUFFER, ssMesh.VBO);
                    glBufferData(GL_ARRAY_BUFFER, n * sizeof(glm::vec3), body.trail.data(), GL_DYNAMIC_DRAW);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                    glEnableVertexAttribArray(0);
                    const size_t kGroups = 5;
                    for (size_t g = 0; g < kGroups; ++g)
                    {
                        size_t first = (n - 1) * g / kGroups;
                        size_t last = (n - 1) * (g + 1) / kGroups;
                        if (last <= first) continue;
                        glUniform1f(uAlpha, ((float)(g + 1) / (float)kGroups) * 0.6f);
                        glDrawArrays(GL_LINE_STRIP, (GLint)first, (GLsizei)(last - first + 1));
                    }
                    glBindVertexArray(0);
                    glDisable(GL_BLEND);
                }
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(body.rotationSpeed * g_simTime), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(body.scaledSize) * body.shapeScale);

            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(uColor, body.fallbackColor.r, body.fallbackColor.g, body.fallbackColor.b);
            bool isSun = body.isStar || body.name == "Sun";
            glUniform1i(uUseLit, isSun ? GL_FALSE : GL_TRUE);
            glUniform1i(uUseTex, body.plainColor ? GL_FALSE : GL_TRUE);
            glUniform1f(uUVOffset, isSun ? g_simTime * 0.04f : 0.0f);
            glUniform1f(uAlpha, 1.0f);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, body.texture);
            uploadImpacts(body, uImpactCount, uImpacts, uImpactAges);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
            glUniform1i(uImpactCount, 0);

            if (body.atmoStrength > 0.0f && !isSun)
            {
                float hp = glm::clamp(body.health / 100.0f, 0.0f, 1.0f);
                glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.3f, 0.1f), body.atmoColor, hp);
                float strength = body.atmoStrength * (0.25f + 0.75f * hp);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                for (int layer = 0; layer < 3; ++layer)
                {
                    float s = 1.06f + layer * 0.07f;
                    glm::mat4 am = glm::translate(glm::mat4(1.0f), pos);
                    am = glm::scale(am, glm::vec3(body.scaledSize * s));
                    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(am));
                    glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                    glUniform1f(uUVOffset, 0.0f);
                    glUniform1f(uAlpha, 0.22f * strength / (float)(layer + 1));
                    glUniform3f(uColor, col.r, col.g, col.b);
                    glBindVertexArray(sphereMesh.VAO);
                    glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
                }
                glCullFace(GL_BACK);
                glDisable(GL_CULL_FACE);
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            if (body.hasClouds)
            {
                float hp = glm::clamp(body.health / 100.0f, 0.0f, 1.0f);
                if (hp > 0.02f)
                {
                    glm::mat4 cm = glm::translate(glm::mat4(1.0f), pos);
                    cm = glm::rotate(cm, glm::radians(body.rotationSpeed * g_simTime * 1.25f + 40.0f), glm::vec3(0, 1, 0));
                    cm = glm::scale(cm, glm::vec3(body.scaledSize * 1.025f));
                    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(cm));
                    glUniform1i(uCloudMode, 1);
                    glUniform1f(uCloudSeed, 3.7f);
                    glUniform1f(uAlpha, hp * hp);
                    glDepthMask(GL_FALSE);
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glBindVertexArray(sphereMesh.VAO);
                    glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
                    glDisable(GL_BLEND);
                    glDepthMask(GL_TRUE);
                    glUniform1i(uCloudMode, 0);
                }
            }

            if (body.isBlackHole)
            {
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                for (int layer = 0; layer < 2; ++layer)
                {
                    float glowScale = 1.15f + layer * 0.35f;
                    glm::mat4 glowModel = glm::translate(glm::mat4(1.0f), pos);
                    glowModel = glm::scale(glowModel, glm::vec3(body.scaledSize * glowScale));
                    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(glowModel));
                    glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                    glUniform1f(uUVOffset, 0.0f);
                    glUniform1f(uAlpha, 0.35f / (float)(layer + 1));
                    glUniform3f(uColor, 1.0f, 0.75f, 0.4f);
                    glBindVertexArray(sphereMesh.VAO);
                    glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
                }
                glCullFace(GL_BACK);
                glDisable(GL_CULL_FACE);
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            if (isSun)
            {
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);

                for (int layer = 0; layer < 3; ++layer)
                {
                    float glowScale = 1.25f + layer * 0.6f;
                    float glowAlpha = 0.30f / (float)(layer + 1);
                    glm::mat4 glowModel = glm::translate(glm::mat4(1.0f), pos);
                    glowModel = glm::scale(glowModel, glm::vec3(body.scaledSize * glowScale));
                    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(glowModel));
                    glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                    glUniform1f(uUVOffset, 0.0f);
                    glUniform1f(uAlpha, glowAlpha);
                    if (body.supernovaed) glUniform3f(uColor, 0.6f, 0.8f, 1.0f);
                    else                  glUniform3f(uColor, 1.0f, 0.65f, 0.25f);
                    glBindVertexArray(sphereMesh.VAO);
                    glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
                }

                glCullFace(GL_BACK);
                glDisable(GL_CULL_FACE);
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            if (body.orbitRadius > 0.0f)
            {
                glm::mat4 om = glm::scale(glm::mat4(1.0f), glm::vec3(body.orbitRadius));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(om));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);   glUniform1f(uAlpha, 1.0f);
                glUniform3f(uColor, 0.25f, 0.25f, 0.25f);
                glBindVertexArray(orbitMesh.VAO);
                glDrawArrays(GL_LINE_LOOP, 0, orbitMesh.vertexCount);
            }

            if (body.name == "Saturn")
            {
                glDisable(GL_CULL_FACE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glm::mat4 rm = glm::translate(glm::mat4(1.0f), pos);
                rm = glm::rotate(rm, glm::radians(25.0f), glm::vec3(1, 0, 0));
                rm = glm::scale(rm, glm::vec3(body.scaledSize));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(rm));
                glUniform1i(uUseTex, GL_TRUE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);   glUniform1f(uAlpha, 1.0f);
                glUniform3f(uColor, 1.0f, 1.0f, 1.0f);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texSaturnRing);
                glBindVertexArray(ringMesh.VAO);
                glDrawElements(GL_TRIANGLES, ringMesh.indexCount, GL_UNSIGNED_INT, 0);
                glDisable(GL_BLEND);
                glEnable(GL_CULL_FACE);
            }

            if (body.name == "Earth") { earthPos = pos; earthAlive = true; }

            if (bi == hoveredBody && bi != selectedBody)
            {
                glm::mat4 outline = glm::mat4(1.0f);
                outline = glm::translate(outline, pos);
                outline = glm::scale(outline, glm::vec3(body.scaledSize * 1.08f));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(outline));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);   glUniform1f(uAlpha, 1.0f);
                glUniform3f(uColor, 1.0f, 0.85f, 0.35f);
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                glBindVertexArray(sphereMesh.VAO);
                glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
                glCullFace(GL_BACK);
                glDisable(GL_CULL_FACE);
            }
        }

        if (earthAlive)
        {
            float ma = glm::radians(moonRevSpeed * g_simTime);
            glm::vec3 mp = earthPos + glm::vec3(cosf(ma) * moonOrbitRadius, 0, sinf(ma) * moonOrbitRadius);
            glm::mat4 mm = glm::translate(glm::mat4(1.0f), mp);
            mm = glm::rotate(mm, glm::radians(moonSpinSpeed * g_simTime), glm::vec3(0, 1, 0));
            mm = glm::scale(mm, glm::vec3(moonSize));
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(mm));
            glUniform1i(uUseTex, GL_TRUE); glUniform1i(uUseLit, GL_TRUE);
            glUniform1f(uUVOffset, 0.0f);  glUniform1f(uAlpha, 1.0f);
            glUniform3f(uColor, moonFallback.r, moonFallback.g, moonFallback.b);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texMoon);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        for (size_t sIdx = 0; sIdx < satellites.size(); ++sIdx)
        {
            const Satellite& sat = satellites[sIdx];
            glm::vec3 parentPos(0.0f);
            bool parentAlive = false;
            for (const Body& pb : bodies)
                if (pb.name == sat.parentName) { parentPos = pb.position; parentAlive = !pb.destroyed; break; }
            if (!parentAlive) continue;

            float phase = (float)sIdx * 47.0f;
            float sa = glm::radians(sat.orbitSpeed * g_simTime + phase);
            glm::vec3 sp = parentPos + glm::vec3(cosf(sa) * sat.orbitRadius, 0, sinf(sa) * sat.orbitRadius);
            glm::mat4 smat = glm::translate(glm::mat4(1.0f), sp);
            smat = glm::rotate(smat, glm::radians(sat.spinSpeed * g_simTime), glm::vec3(0, 1, 0));
            smat = glm::scale(smat, glm::vec3(sat.size));
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(smat));
            glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_TRUE);
            glUniform1f(uUVOffset, 0.0f);  glUniform1f(uAlpha, 1.0f);
            glUniform3f(uColor, sat.fallbackColor.r, sat.fallbackColor.g, sat.fallbackColor.b);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        {
            glm::mat4 dsRot = glm::rotate(glm::mat4(1.0f), glm::radians(6.0f * g_simTime), glm::vec3(0, 1, 0));
            glm::mat4 dsModel = glm::translate(glm::mat4(1.0f), battleStationPos) * dsRot;

            glm::mat4 bodyModel = glm::scale(dsModel, glm::vec3(1.6f));
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(bodyModel));
            glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_TRUE);
            glUniform1f(uUVOffset, 0.0f); glUniform1f(uAlpha, 1.0f);
            glUniform3f(uColor, 0.55f, 0.56f, 0.60f);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

            glm::mat4 dishModel = glm::translate(dsModel, glm::vec3(1.5f, 0.3f, 0.0f));
            dishModel = glm::scale(dishModel, glm::vec3(0.35f));
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(dishModel));
            glUniform1i(uUseLit, GL_FALSE);
            glUniform3f(uColor, 1.0f, 0.25f, 0.2f);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        for (const Asteroid& a : asteroids)
        {
            float angle = glm::radians(a.orbitAngle + a.orbitSpeed * g_simTime);
            glm::vec3 ap = glm::vec3(cosf(angle) * a.orbitRadius, a.yOffset, sinf(angle) * a.orbitRadius);
            glm::mat4 am = glm::translate(glm::mat4(1.0f), ap);
            am = glm::rotate(am, angle * 3.0f, glm::vec3(0.3f, 1.0f, 0.5f));
            am = glm::scale(am, glm::vec3(a.size));
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(am));
            glUniform1i(uUseTex, GL_TRUE); glUniform1i(uUseLit, GL_TRUE);
            glUniform1f(uUVOffset, 0.0f);  glUniform1f(uAlpha, 1.0f);
            glUniform3f(uColor, 0.5f, 0.45f, 0.4f);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texAsteroid);
            glBindVertexArray(sphereMesh.VAO);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(2.0f);

        for (auto& ss : shootingStars)
        {
            if (!ss.active) continue;
            float age = g_simTime - ss.spawnTime;
            if (age > ss.lifetime) { ss.active = false; continue; }

            float t = age / ss.lifetime;
            float fade = (t < 0.2f) ? (t / 0.2f) : (1.0f - (t - 0.2f) / 0.8f);
            fade = glm::clamp(fade, 0.0f, 1.0f);

            glm::vec3 head = ss.origin + ss.direction * (ss.speed * age);
            glm::vec3 tail = head - ss.direction * ss.length;

            glm::mat4 identity = glm::mat4(1.0f);
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
            glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
            glUniform1f(uUVOffset, 0.0f);
            glUniform1f(uAlpha, fade);
            glUniform3f(uColor, 1.0f, 1.0f, 0.85f);

            drawLine3D(ssMesh, tail, head);
        }

        glLineWidth(1.0f);
        glDisable(GL_BLEND);

        {
            const float kFxLifetime = 0.6f;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glLineWidth(2.0f);

            for (size_t fi = 0; fi < collisionEffects.size(); )
            {
                CollisionEffect& fx = collisionEffects[fi];
                float age = g_simTime - fx.spawnTime;
                float effLifetime = kFxLifetime * fx.lifetimeMul;
                if (age > effLifetime) { collisionEffects.erase(collisionEffects.begin() + fi); continue; }

                float t = age / effLifetime;
                float fade = 1.0f - t;

                glm::mat4 flashModel = glm::translate(glm::mat4(1.0f), fx.pos);
                flashModel = glm::scale(flashModel, glm::vec3((0.6f + t * 2.5f) * fx.scaleMul));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(flashModel));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);
                glUniform1f(uAlpha, fade * 0.8f);
                glUniform3f(uColor, 1.0f, 0.8f, 0.4f);
                glBindVertexArray(sphereMesh.VAO);
                glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

                glm::mat4 identity = glm::mat4(1.0f);
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
                glUniform3f(uColor, 1.0f, 0.9f, 0.6f);
                glUniform1f(uAlpha, fade);
                for (int k = 0; k < 8; ++k)
                {
                    glm::vec3 head = fx.pos + fx.sparkDir[k] * (age * 12.0f * fx.scaleMul);
                    glm::vec3 tail = fx.pos + fx.sparkDir[k] * (age * 12.0f * fx.scaleMul - 0.8f * fx.scaleMul);
                    drawLine3D(ssMesh, tail, head);
                }

                ++fi;
            }

            glLineWidth(1.0f);
            glDisable(GL_BLEND);
        }

        {
            const float kBeamLifetime = 0.4f;
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glLineWidth(4.0f);

            for (size_t bi2 = 0; bi2 < laserBeams.size(); )
            {
                LaserBeam& lb = laserBeams[bi2];
                float age = g_simTime - lb.spawnTime;
                if (age > kBeamLifetime) { laserBeams.erase(laserBeams.begin() + bi2); continue; }

                glm::mat4 identity = glm::mat4(1.0f);
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);
                glUniform1f(uAlpha, 1.0f - age / kBeamLifetime);
                glUniform3f(uColor, 0.3f, 1.0f, 0.4f);

                drawLine3D(ssMesh, lb.from, lb.to);

                ++bi2;
            }

            glLineWidth(1.0f);
            glDisable(GL_BLEND);
        }

        if (slingCharging)
        {
            glm::vec3 previewVel = computeSlingVelocity(window);
            const int   kPreviewSteps = 260;
            const float kPreviewDt = 0.05f;
            std::vector<glm::vec3> path = predictTrajectory(slingOrigin, previewVel, kPreviewSteps, kPreviewDt);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(2.0f);

            glm::mat4 identity = glm::mat4(1.0f);
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
            glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
            glUniform1f(uUVOffset, 0.0f);
            glUniform3f(uColor, 0.4f, 0.9f, 1.0f);

            glm::vec3 prev = slingOrigin;
            for (size_t p = 0; p < path.size(); ++p)
            {

                if ((p / 3) % 2 == 0)
                {
                    glUniform1f(uAlpha, 0.85f * (1.0f - (float)p / (float)path.size()));
                    drawLine3D(ssMesh, prev, path[p]);
                }
                prev = path[p];
            }

            glLineWidth(1.0f);
            glDisable(GL_BLEND);
        }

        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glLineWidth(3.0f);
            for (const PendingStrike& ps : pendingStrikes)
            {
                if (ps.weapon == W_PLANET_KILLER || ps.weapon == W_LASER) continue;
                if (ps.body < 0 || ps.body >= (int)bodies.size()) continue;
                const Body& tb = bodies[ps.body];
                if (tb.uid != ps.uid || tb.destroyed) continue;
                float t = (g_simTime - ps.fireTime) / ps.duration;
                if (t < 0.0f || t > 1.0f) continue;

                glm::vec3 target = localDirToWorldPoint(tb, ps.localDir);
                glm::vec3 head = glm::mix(ps.startWorld, target, t * t);
                glm::vec3 travel = target - ps.startWorld;
                float tl = glm::length(travel);
                glm::vec3 dirv = (tl > 0.001f) ? (travel / tl) : glm::vec3(0.0f, 1.0f, 0.0f);

                float tailLen = 2.0f, headSize = 0.14f;
                glm::vec3 col(1.0f, 0.5f, 0.2f);
                if (ps.weapon == W_NUKE)     { tailLen = 3.5f; headSize = 0.24f; col = glm::vec3(1.0f, 0.9f, 0.5f); }
                if (ps.weapon == W_ASTEROID) { tailLen = 6.0f; headSize = 0.55f; col = glm::vec3(1.0f, 0.55f, 0.2f); }

                glm::mat4 identity = glm::mat4(1.0f);
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(identity));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);
                glUniform1f(uAlpha, 0.9f);
                glUniform3f(uColor, col.r, col.g, col.b);
                drawLine3D(ssMesh, head - dirv * tailLen, head);

                glm::mat4 hm = glm::translate(glm::mat4(1.0f), head);
                hm = glm::scale(hm, glm::vec3(headSize));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(hm));
                glBindVertexArray(sphereMesh.VAO);
                glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
            }
            glLineWidth(1.0f);
            glDisable(GL_BLEND);
        }

        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            for (size_t wi = 0; wi < shockwaves.size(); )
            {
                Shockwave sw = shockwaves[wi];
                float age = g_simTime - sw.spawnTime;
                if (age > sw.lifetime) { shockwaves.erase(shockwaves.begin() + wi); continue; }
                float t = age / sw.lifetime;

                glm::mat4 m = glm::translate(glm::mat4(1.0f), sw.pos) * alignYTo(sw.normal);
                m = glm::scale(m, glm::vec3(sw.size * (0.15f + t)));
                glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(m));
                glUniform1i(uUseTex, GL_FALSE); glUniform1i(uUseLit, GL_FALSE);
                glUniform1f(uUVOffset, 0.0f);
                glUniform1f(uAlpha, (1.0f - t) * 0.75f);
                glUniform3f(uColor, sw.color.r, sw.color.g, sw.color.b);
                glBindVertexArray(ringMesh.VAO);
                glDrawElements(GL_TRIANGLES, ringMesh.indexCount, GL_UNSIGNED_INT, 0);
                ++wi;
            }
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        if (cursorDisabled)
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoCross = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoCross));
            glUniform1f(uUiScale, uiScale);

            float cx = (winW > 0) ? (float)winW * 0.5f : 0.0f;
            float cy = (winH > 0) ? (float)winH * 0.5f : 0.0f;
            drawUiLine(panelVAO, panelVBO, cx - 8.0f, cy, cx - 2.0f, cy, 2.0f, 255, 255, 255, 200);
            drawUiLine(panelVAO, panelVBO, cx + 2.0f, cy, cx + 8.0f, cy, 2.0f, 255, 255, 255, 200);
            drawUiLine(panelVAO, panelVBO, cx, cy - 8.0f, cx, cy - 2.0f, 2.0f, 255, 255, 255, 200);
            drawUiLine(panelVAO, panelVBO, cx, cy + 2.0f, cx, cy + 8.0f, 2.0f, 255, 255, 255, 200);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoList = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoList));
            glUniform1f(uUiScale, uiScale);

            float listX = (float)winW - kSidebarWidth;
            float listPanelH = 30.0f + (float)bodies.size() * kSidebarItemH + 10.0f;

            unsigned char listVerts[4 * 16];
            auto setLV = [&](int i, float x, float y) {
                float* fp = (float*)(listVerts + i * 16);
                fp[0] = x; fp[1] = y; fp[2] = 0.0f;
                unsigned char* cp = listVerts + i * 16 + 12;
                cp[0] = 10; cp[1] = 15; cp[2] = 30; cp[3] = 170;
                };
            setLV(0, listX, kSidebarTop);
            setLV(1, (float)winW, kSidebarTop);
            setLV(2, (float)winW, kSidebarTop + listPanelH);
            setLV(3, listX, kSidebarTop + listPanelH);

            glBindVertexArray(panelVAO);
            glBindBuffer(GL_ARRAY_BUFFER, panelVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(listVerts), listVerts);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            drawUiText(textVAO, textVBO, listX + 12, kSidebarTop + 10, "PLANETS", 140, 150, 170, 255);
            for (int i = 0; i < (int)bodies.size(); ++i)
            {
                float ty = kSidebarTop + 30.0f + i * kSidebarItemH + 6.0f;
                if (bodies[i].destroyed)
                {
                    std::string label = bodies[i].name + " (destroyed)";
                    drawUiText(textVAO, textVBO, listX + 12, ty, label.c_str(), 90, 70, 70, 255);
                    continue;
                }
                unsigned char r = 170, g = 180, b = 200;
                if (i == selectedBody) { r = 255; g = 220; b = 120; }
                else if (i == hoveredBody) { r = 255; g = 255; b = 255; }
                drawUiText(textVAO, textVBO, listX + 12, ty, bodies[i].name.c_str(), r, g, b, 255);
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        if (selectedBody >= 0 && selectedBody < (int)bodies.size())
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 ortho = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(ortho));
            glUniform1f(uUiScale, uiScale);

            const Body& fb = bodies[selectedBody];
            float px = 24.0f, py = 24.0f;
            float pw = (winW > 0) ? std::min(430.0f, (float)winW - px * 2.0f) : 430.0f;
            pw = std::max(pw, 220.0f);
            char buf[128];

            std::vector<std::string> lines;

            if (selectedBody < (int)planetInfo.size())
            {
                const PlanetInfo& info = planetInfo[selectedBody];
                lines.push_back("Type: " + info.type);
            }

            snprintf(buf, sizeof(buf), "Mass: %.3g Earths   ( [ / ] to change )", fb.mass);
            lines.push_back(buf);
            snprintf(buf, sizeof(buf), "Size: %.2f   ( , / . to change )", fb.scaledSize);
            lines.push_back(buf);
            snprintf(buf, sizeof(buf), "Speed: %.2f units/s", glm::length(fb.velocity));
            lines.push_back(buf);

            snprintf(buf, sizeof(buf), "Health: %.0f%%", std::max(0.0f, fb.health));
            lines.push_back(buf);
            if (fb.population > 0.0)
                lines.push_back("Population: " + formatPopulation(fb.population));
            if (fb.supernovaed)
                lines.push_back("Status: white-dwarf remnant (supernova)");

            if (selectedBody != 0 && !bodies.empty())
            {
                const Body& sun = bodies[0];
                glm::vec3 r = fb.position - sun.position;
                glm::vec3 v = fb.velocity - sun.velocity;
                double mu = kG * sun.mass;
                if (mu > 0.0 && glm::length(r) > 0.001f)
                {
                    glm::vec3 h = glm::cross(r, v);
                    glm::vec3 eVec = glm::cross(v, h) / (float)mu - glm::normalize(r);
                    float ecc = glm::length(eVec);
                    const char* desc = (ecc < 0.05f) ? "circular" : (ecc < 0.3f) ? "slightly elliptical"
                        : (ecc < 1.0f) ? "elliptical" : "ESCAPING (not a closed orbit)";
                    snprintf(buf, sizeof(buf), "Eccentricity: %.2f (%s)", ecc, desc);
                    lines.push_back(buf);
                }
            }

            if (selectedBody < (int)planetInfo.size())
            {
                const PlanetInfo& info = planetInfo[selectedBody];
                lines.push_back("Distance from Sun: " + info.distance);
                lines.push_back("Day length: " + info.dayLength);
                lines.push_back("Year length: " + info.yearLength);
                lines.push_back("Moons: " + info.moons);
            }

            if (fb.orbitRadius > 0.0f && !bodies.empty())
            {
                float distNow = glm::length(fb.position - bodies[0].position);
                float deviation = fabsf(distNow - fb.orbitRadius) / fb.orbitRadius;
                const char* status = (deviation < 0.15f) ? "Stable" : (deviation < 0.5f ? "Drifting" : "UNSTABLE");
                snprintf(buf, sizeof(buf), "Orbit: %s", status);
                lines.push_back(buf);
            }

            if (selectedBody < (int)planetInfo.size())
                lines.push_back(planetInfo[selectedBody].funFact);

            if (selectedBody != 0)
            {
                if (battleStationCooldownRemaining > 0.0f)
                {
                    snprintf(buf, sizeof(buf), "Battle station recharging: %.1fs", battleStationCooldownRemaining);
                    lines.push_back(buf);
                }
                else
                {
                    lines.push_back("Press K to destroy it (battle station)");
                }
            }
            float ph = 56.0f + (float)lines.size() * 18.0f + 24.0f;

            unsigned char panelVerts[4 * 16];
            auto setV = [&](int i, float x, float y) {
                float* fp = (float*)(panelVerts + i * 16);
                fp[0] = x; fp[1] = y; fp[2] = 0.0f;
                unsigned char* cp = panelVerts + i * 16 + 12;
                cp[0] = 10; cp[1] = 15; cp[2] = 30; cp[3] = 190;
                };
            setV(0, px, py);
            setV(1, px + pw, py);
            setV(2, px + pw, py + ph);
            setV(3, px, py + ph);

            glBindVertexArray(panelVAO);
            glBindBuffer(GL_ARRAY_BUFFER, panelVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(panelVerts), panelVerts);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            drawUiText(textVAO, textVBO, px + 16, py + 14, fb.name.c_str(), 255, 255, 255, 255);

            {
                float bw = 110.0f, bh = 8.0f;
                float bx = px + pw - bw - 16.0f, by = py + 16.0f;
                float hpf = glm::clamp(fb.health / 100.0f, 0.0f, 1.0f);
                drawUiRect(panelVAO, panelVBO, bx, by, bw, bh, 30, 34, 48, 220);
                if (hpf > 0.001f)
                {
                    unsigned char hr = (unsigned char)(230.0f * (1.0f - hpf) + 70.0f * hpf);
                    unsigned char hg = (unsigned char)(60.0f * (1.0f - hpf) + 220.0f * hpf);
                    drawUiRect(panelVAO, panelVBO, bx, by, bw * hpf, bh, hr, hg, 70, 240);
                }
            }

            float lineY = py + 38;
            for (size_t li = 0; li < lines.size(); ++li)
            {

                bool isFunFact = (li == lines.size() - 1);
                if (isFunFact)
                    drawUiText(textVAO, textVBO, px + 16, lineY, lines[li].c_str(), 230, 220, 160, 255);
                else
                    drawUiText(textVAO, textVBO, px + 16, lineY, lines[li].c_str(), 190, 205, 230, 255);
                lineY += 18;
            }
            drawUiText(textVAO, textVBO, px + 16, py + ph - 20, "Right-click to go back", 140, 150, 170, 255);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoMap = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoMap));
            glUniform1f(uUiScale, uiScale);

            float mapSize = 170.0f;
            float mapX = 20.0f;
            float mapY = (winH > 0 ? (float)winH - 210.0f : 20.0f);
            mapY = glm::clamp(mapY, kSidebarTop, std::max(kSidebarTop, (winH > 0 ? (float)winH - mapSize - 20.0f : 20.0f)));
            float mapCenterX = mapX + mapSize * 0.5f;
            float mapCenterY = mapY + mapSize * 0.5f;

            const float kMapWorldRadius = 62.0f;
            float mapScale = (mapSize * 0.5f - 8.0f) / kMapWorldRadius;

            drawUiRect(panelVAO, panelVBO, mapX, mapY, mapSize, mapSize, 8, 10, 22, 170);
            drawUiText(textVAO, textVBO, mapX + 8, mapY + 6, "MAP", 140, 150, 170, 255);

            float camMx = mapCenterX + cameraPos.x * mapScale;
            float camMy = mapCenterY + cameraPos.z * mapScale;
            camMx = glm::clamp(camMx, mapX + 4.0f, mapX + mapSize - 4.0f);
            camMy = glm::clamp(camMy, mapY + 4.0f, mapY + mapSize - 4.0f);
            glm::vec2 camDir2D = glm::normalize(glm::vec2(cameraFront.x, cameraFront.z) + glm::vec2(0.0001f, 0.0f));
            drawUiLine(panelVAO, panelVBO, camMx, camMy, camMx + camDir2D.x * 14.0f, camMy + camDir2D.y * 14.0f,
                2.0f, 120, 220, 255, 255);
            drawUiRect(panelVAO, panelVBO, camMx - 2.0f, camMy - 2.0f, 4.0f, 4.0f, 120, 220, 255, 255);

            for (int i = 0; i < (int)bodies.size(); ++i)
            {
                if (bodies[i].destroyed) continue;
                glm::vec3 pos = bodyWorldPos(bodies[i], g_simTime);
                float dotX = mapCenterX + pos.x * mapScale;
                float dotY = mapCenterY + pos.z * mapScale;
                float dotSize = (bodies[i].name == "Sun") ? 6.0f : 3.0f;

                unsigned char r, g, b;
                if (i == selectedBody) { r = 255; g = 220; b = 120; }
                else if (i == hoveredBody) { r = 255; g = 255; b = 255; }
                else
                {
                    r = (unsigned char)(bodies[i].fallbackColor.r * 255.0f);
                    g = (unsigned char)(bodies[i].fallbackColor.g * 255.0f);
                    b = (unsigned char)(bodies[i].fallbackColor.b * 255.0f);
                }
                drawUiRect(panelVAO, panelVBO, dotX - dotSize * 0.5f, dotY - dotSize * 0.5f, dotSize, dotSize, r, g, b, 255);
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        if (paused || fabsf(timeScale - 1.0f) > 0.01f || tourActive)
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoStatus = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoStatus));
            glUniform1f(uUiScale, uiScale);

            char buf[96];
            if (tourActive && selectedBody >= 0 && selectedBody < (int)bodies.size())
                snprintf(buf, sizeof(buf), "TOUR: %s  (T to stop)", bodies[selectedBody].name.c_str());
            else if (paused)
                snprintf(buf, sizeof(buf), "PAUSED  (P to resume)");
            else
                snprintf(buf, sizeof(buf), "Speed: %.1fx  (+/- to change)", timeScale);

            float statusY = (winH > 0 ? (float)winH - 30.0f : 30.0f);
            drawUiText(textVAO, textVBO, 20.0f, statusY, buf, 255, 210, 90, 255);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoStats = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoStats));
            glUniform1f(uUiScale, uiScale);

            char statBuf[64];
            float sx = (winW > 0 ? (float)winW - 190.0f : 20.0f);
            snprintf(statBuf, sizeof(statBuf), "FPS: %.0f", displayFps);
            drawUiText(textVAO, textVBO, sx, (float)winH - 66.0f, statBuf, 150, 220, 150, 255);
            snprintf(statBuf, sizeof(statBuf), "Collisions: %d", statsCollisions);
            drawUiText(textVAO, textVBO, sx, (float)winH - 48.0f, statBuf, 190, 190, 210, 255);
            snprintf(statBuf, sizeof(statBuf), "Destroyed: %d", statsDestroyed);
            drawUiText(textVAO, textVBO, sx, (float)winH - 30.0f, statBuf, 210, 150, 150, 255);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoBar = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoBar));
            glUniform1f(uUiScale, uiScale);

            float barW = 140.0f, barH = 10.0f;
            float barX = (winW > 0) ? (float)winW - kSidebarWidth - barW - 16.0f : 16.0f;
            float barY = kSidebarTop;
            float ready = 1.0f - glm::clamp(battleStationCooldownRemaining / kBattleStationCooldown, 0.0f, 1.0f);

            drawUiText(textVAO, textVBO, barX, barY - 14.0f, "STATION", 140, 150, 170, 255);
            drawUiRect(panelVAO, panelVBO, barX, barY, barW, barH, 20, 24, 34, 200);
            unsigned char br = ready >= 1.0f ? 100 : 220, bg = ready >= 1.0f ? 230 : 180, bb = 110;
            if (ready > 0.001f)
                drawUiRect(panelVAO, panelVBO, barX, barY, barW * ready, barH, br, bg, bb, 230);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        if (helpVisible)
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoHelp = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoHelp));
            glUniform1f(uUiScale, uiScale);

            static const char* helpLines[] = {
                "CONTROLS",
                "WASD - move       Mouse - look around      Tab - toggle cursor",
                "Left click - fire comet / select / drag to slingshot",
                "Right click - fire rocket / deselect        Middle click - fire black hole",
                "P - pause          + / - - simulation speed        T - guided tour",
                "R - reset          [ / ] - selected mass          , / . - selected size",
                "K - battle station fires at selected planet",
                "O - save          L - load           1 / 2 / 3 - choose save slot",
                "H - toggle this help          M - mute/unmute sound",
                "",
                "SOLAR SMASH",
                "Select a planet, then click its surface to fire (hold for Laser)",
                "Q / E or click the bar - change weapon      C - cinematic camera",
                "Arrows / middle-drag - orbit planet         Wheel - zoom",
                "7 Earth-Mars crash   8 Jupiter star   9 Sun supernova   0 black hole",
            };
            int lineCount = (int)(sizeof(helpLines) / sizeof(helpLines[0]));

            float hw = 620.0f, hh = 30.0f + (float)lineCount * 20.0f + 16.0f;
            float hx = (winW > 0) ? ((float)winW - hw) * 0.5f : 20.0f;
            float hy = (winH > 0) ? ((float)winH - hh) * 0.5f : 20.0f;
            hx = glm::clamp(hx, 10.0f, std::max(10.0f, (winW > 0 ? (float)winW - hw - 10.0f : 10.0f)));
            hy = glm::clamp(hy, 10.0f, std::max(10.0f, (winH > 0 ? (float)winH - hh - 10.0f : 10.0f)));

            drawUiRect(panelVAO, panelVBO, hx, hy, hw, hh, 8, 10, 22, 225);
            for (int li = 0; li < lineCount; ++li)
            {
                bool isTitle = (li == 0 || li == 10);
                drawUiText(textVAO, textVBO, hx + 16.0f, hy + 14.0f + li * 20.0f, helpLines[li],
                    isTitle ? 255 : 195, isTitle ? 230 : 205, isTitle ? 140 : 225, 255);
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        if (!toasts.empty())
        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoToast = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoToast));
            glUniform1f(uUiScale, uiScale);

            float ty = 16.0f;
            for (size_t ti = 0; ti < toasts.size(); )
            {
                float age = g_wallTime - toasts[ti].spawnTime;
                if (age > kToastLifetime) { toasts.erase(toasts.begin() + ti); continue; }

                float fade = (age < 0.2f) ? (age / 0.2f) : glm::clamp(1.0f - (age - (kToastLifetime - 0.5f)) / 0.5f, 0.0f, 1.0f);
                unsigned char a = (unsigned char)(fade * 235.0f);

                float tw = 24.0f + (float)toasts[ti].text.size() * 6.0f;
                float tx = (winW > 0) ? ((float)winW - tw) * 0.5f : 20.0f;
                drawUiRect(panelVAO, panelVBO, tx, ty, tw, 26.0f, 15, 20, 35, a);
                drawUiText(textVAO, textVBO, tx + 12.0f, ty + 8.0f, toasts[ti].text.c_str(), 230, 235, 245, a);

                ty += 32.0f;
                ++ti;
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(uiProg);
            glm::mat4 orthoWeap = glm::ortho(0.0f, (float)fbW, (float)fbH, 0.0f, -1.0f, 1.0f);
            glUniformMatrix4fv(uOrtho, 1, GL_FALSE, glm::value_ptr(orthoWeap));
            glUniform1f(uUiScale, uiScale);

            float total = W_COUNT * kWeaponBtnW + (W_COUNT - 1) * kWeaponBtnGap;
            float x0 = ((float)winW - total) * 0.5f;
            float y0 = (float)winH - kWeaponBtnH - 14.0f;

            const char* hint = (selectedBody >= 0)
                ? "Click the planet's surface to fire   (Q / E: change weapon)"
                : "Select a planet first, then click its surface   (Q / E: change weapon)";
            drawUiText(textVAO, textVBO, x0, y0 - 18.0f, hint, 150, 160, 185, 255);

            for (int w = 0; w < W_COUNT; ++w)
            {
                float x = x0 + w * (kWeaponBtnW + kWeaponBtnGap);
                bool sel = (w == (int)currentWeapon);
                drawUiRect(panelVAO, panelVBO, x, y0, kWeaponBtnW, kWeaponBtnH,
                    sel ? 70 : 12, sel ? 50 : 16, sel ? 18 : 30, sel ? 235 : 190);
                if (sel)
                    drawUiRect(panelVAO, panelVBO, x, y0 + kWeaponBtnH - 3.0f, kWeaponBtnW, 3.0f, 255, 190, 70, 255);
                drawUiText(textVAO, textVBO, x + 10.0f, y0 + 9.0f, weaponNames[w],
                    sel ? 255 : 170, sel ? 225 : 180, sel ? 140 : 200, 255);
            }

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    audioShutdown();
    glDeleteVertexArrays(1, &sphereMesh.VAO); glDeleteBuffers(1, &sphereMesh.VBO); glDeleteBuffers(1, &sphereMesh.EBO);
    glDeleteVertexArrays(1, &ringMesh.VAO);   glDeleteBuffers(1, &ringMesh.VBO);   glDeleteBuffers(1, &ringMesh.EBO);
    glDeleteVertexArrays(1, &orbitMesh.VAO);  glDeleteBuffers(1, &orbitMesh.VBO);
    glDeleteVertexArrays(1, &ssMesh.VAO);     glDeleteBuffers(1, &ssMesh.VBO);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}

