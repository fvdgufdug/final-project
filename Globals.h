#pragma once

#include <iostream>
#include <cstdio>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <mutex>

// ---------------------------------------------------------------------------
// Shaders (used by Render.cpp)
// ---------------------------------------------------------------------------
inline const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos     = vec3(model * vec4(aPos, 1.0));
    Normal      = mat3(transpose(inverse(model))) * aNormal;
    TexCoord    = aTexCoord;
    LocalPos    = aPos;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

inline const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LocalPos;
out vec4 FragColor;

uniform sampler2D objectTexture;
uniform vec3      objectColor;
uniform vec3      sunWorldPos;
uniform bool      useTexture;
uniform bool      useLighting;
uniform float     uvOffset;
uniform float     alpha;

// Impact scars: xyz = direction in the planet's LOCAL space, w = angular radius (radians)
const int MAX_IMPACTS = 32;
uniform int   impactCount;
uniform vec4  impacts[MAX_IMPACTS];
uniform float impactAges[MAX_IMPACTS];

// cloudMode == 1 -> draw a procedural cloud layer instead of a normal surface
uniform int   cloudMode;
uniform float cloudSeed;

float hash31(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float valueNoise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash31(i + vec3(0.0, 0.0, 0.0)), hash31(i + vec3(1.0, 0.0, 0.0)), f.x),
                   mix(hash31(i + vec3(0.0, 1.0, 0.0)), hash31(i + vec3(1.0, 1.0, 0.0)), f.x), f.y),
               mix(mix(hash31(i + vec3(0.0, 0.0, 1.0)), hash31(i + vec3(1.0, 0.0, 1.0)), f.x),
                   mix(hash31(i + vec3(0.0, 1.0, 1.0)), hash31(i + vec3(1.0, 1.0, 1.0)), f.x), f.y), f.z);
}

float fbm(vec3 p)
{
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; ++i) { v += a * valueNoise(p); p *= 2.03; a *= 0.5; }
    return v;
}

void main()
{
    vec3  norm     = normalize(Normal);
    vec3  lightDir = normalize(sunWorldPos - FragPos);
    float diff     = max(dot(norm, lightDir), 0.0);

    if (cloudMode == 1)
    {
        float n     = fbm(normalize(LocalPos) * 3.2 + vec3(cloudSeed));
        float cover = smoothstep(0.50, 0.72, n);
        vec3  c     = vec3(1.0) * (0.12 + 1.15 * diff);
        FragColor   = vec4(c, cover * alpha * 0.9);
        return;
    }

    vec4  texSample = texture(objectTexture, vec2(TexCoord.x + uvOffset, TexCoord.y));
    vec3  color     = useTexture ? texSample.rgb : objectColor;
    float texA      = useTexture ? texSample.a : 1.0;

    // ── Impact scars: burnt crater, dark rim, glowing lava that slowly cools ──
    vec3 glow = vec3(0.0);
    if (impactCount > 0)
    {
        vec3  p   = normalize(LocalPos);
        float wob = valueNoise(p * 14.0) - 0.5;   // ragged, non-circular edges
        for (int i = 0; i < impactCount; ++i)
        {
            float r     = impacts[i].w;
            float ang   = acos(clamp(dot(p, impacts[i].xyz), -1.0, 1.0)) + wob * r * 0.6;
            float age   = impactAges[i];
            float burn  = 1.0 - smoothstep(r * 0.55, r, ang);
            float rim   = smoothstep(r * 0.75, r, ang) * (1.0 - smoothstep(r, r * 1.25, ang));
            float lava  = (1.0 - smoothstep(0.0, r * 0.7, ang)) * exp(-age * 0.15);
            float flash = (1.0 - smoothstep(0.0, r * 1.3, ang)) * exp(-age * 5.0);
            color = mix(color, color * 0.10 + vec3(0.03, 0.015, 0.01), burn);
            color = mix(color, color * 0.55, rim * 0.6);
            glow += vec3(1.0, 0.35, 0.05) * lava * 1.4 + vec3(1.0, 0.9, 0.6) * flash * 1.5;
        }
    }

    if (!useLighting)
    {
        FragColor = vec4(color + glow, alpha * texA);
        return;
    }

    vec3 ambient  = 0.25 * color;
    vec3 diffuse  = 1.4 * diff * color;
    FragColor = vec4(ambient + diffuse + glow, alpha * texA);
}
)";

// ---------------------------------------------------------------------------
// Shared types, global state and data tables
// ---------------------------------------------------------------------------
inline glm::vec3 cameraPos = glm::vec3(0.0f, 20.0f, 60.0f);
inline glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);

inline glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

inline float deltaTime = 0.0f, lastFrame = 0.0f;

inline float lastX = 960.0f, lastY = 540.0f;

inline bool  firstMouse = true;

inline float yaw = -90.0f, pitch = 0.0f;

inline bool  cursorDisabled = true, tabPressedLastFrame = false;

inline float fov = 70.0f;

inline glm::mat4 g_lastView = glm::mat4(1.0f);

inline glm::mat4 g_lastProj = glm::mat4(1.0f);

inline int       g_fbW = 1, g_fbH = 1;

inline int       selectedBody = -1;

inline glm::vec3 focusOffset(0.0f);

inline bool      rightMousePressedLastFrame = false;

inline float g_simTime = 0.0f;

inline bool  paused = false;

inline float timeScale = 1.0f;

inline bool  pPressedLastFrame = false;

inline bool  tourActive = false;

inline int   tourIndex = -1;

inline float tourTimer = 0.0f;

inline bool  tPressedLastFrame = false;

inline bool  resetRequested = false;

inline bool  rPressedLastFrame = false;

inline bool  oPressedLastFrame = false;

inline bool  lPressedLastFrame = false;

inline bool  saveRequested = false;

inline bool  loadRequested = false;

inline const float kTourDwellSeconds = 5.0f;

inline float g_wallTime = 0.0f;

inline int  activeSaveSlot = 1;

inline bool digit1PressedLastFrame = false, digit2PressedLastFrame = false, digit3PressedLastFrame = false;



struct Toast { std::string text; float spawnTime; };

inline std::vector<Toast> toasts;

inline const float kToastLifetime = 2.4f;

inline bool helpVisible = false;

inline bool hPressedLastFrame = false;

inline bool mPressedLastFrame = false;



enum Weapon { W_NUKE, W_MISSILES, W_ASTEROID, W_LASER, W_PLANET_KILLER, W_COUNT };

inline Weapon      currentWeapon = W_NUKE;

inline const char* weaponNames[W_COUNT] = { "Nuke", "Missiles", "Asteroid", "Laser", "Planet Killer" };

inline float       g_laserTimer = 0.0f;

inline float g_shake = 0.0f;

inline float g_slowMo = 1.0f;

inline float focusMinDist = 2.0f;

inline bool      cinematicEnabled = true;

inline bool      cinematicActive = false;

inline float     cinematicTimer = 0.0f;

inline glm::vec3 cinematicCenter(0.0f);

inline glm::vec3 cinematicOffset(0.0f, 0.0f, 10.0f);

inline int scenarioRequested = 0;

inline bool      slingCharging = false;

inline double    slingStartMouseX = 0.0, slingStartMouseY = 0.0;

inline glm::vec3 slingOrigin(0.0f);

inline glm::vec3 slingBaseDir(0.0f, 0.0f, -1.0f);

inline const float kSlingMinSpeed = 6.0f;

inline const float kSlingMaxSpeed = 55.0f;

inline const float kSlingDragToSpeed = 0.12f;

inline const float kSlingDragToAim = 0.012f;



struct GpuMesh { unsigned int VAO = 0, VBO = 0, EBO = 0, indexCount = 0, vertexCount = 0; };

inline const int kMaxImpacts = 32;


struct Impact { glm::vec3 dir; float radius; float spawnTime; };



struct Body {
    std::string  name;
    float        scaledSize, orbitRadius, rotationSpeed, revolutionSpeed;
    glm::vec3    fallbackColor;
    unsigned int texture;

    double     mass = 1.0;
    glm::vec3  position = glm::vec3(0.0f);
    glm::vec3  velocity = glm::vec3(0.0f);

    std::vector<glm::vec3> trail;
    float trailTimer = 0.0f;

    bool      destroyed = false;
    glm::vec3 shapeScale = glm::vec3(1.0f);
    bool      plainColor = false;
    bool      isBlackHole = false;

    float               health = 100.0f;
    std::vector<Impact> impacts;
    double              population = 0.0;
    glm::vec3           atmoColor = glm::vec3(0.0f);
    float               atmoStrength = 0.0f;
    bool                hasClouds = false;
    bool                isStar = false;
    bool                supernovaed = false;
    int                 uid = 0;
};

inline double kG = 0.0005;

inline std::vector<Body> bodies;

inline unsigned int g_asteroidTexture = 0;

inline const int kMaxBodies = 60;

inline glm::vec3 battleStationPos = glm::vec3(85.0f, 18.0f, 40.0f);

inline bool      kPressedLastFrame = false;

inline const float kBattleStationCooldown = 5.0f;

inline float     battleStationCooldownRemaining = 0.0f;

inline int statsCollisions = 0;

inline int statsDestroyed = 0;

inline float fpsAccumTime = 0.0f;

inline int   fpsFrameCount = 0;

inline float displayFps = 0.0f;



struct LaserBeam { glm::vec3 from, to; float spawnTime; };

inline std::vector<LaserBeam> laserBeams;



struct PlanetInfo {
    std::string type;
    std::string diameter;
    std::string mass;
    std::string distance;
    std::string dayLength;
    std::string yearLength;
    std::string moons;
    std::string funFact;
};

inline std::vector<PlanetInfo> planetInfo = {

    { "Star (G-type main-sequence)", "1,392,700 km", "1.989 x 10^30 kg (~333,000 Earths)",
      "0 km (center of the system)", "~27 Earth days", "-", "0",
      "Contains 99.8% of the Solar System's total mass." },

    { "Terrestrial planet", "4,879 km", "3.30 x 10^23 kg (0.055 Earths)",
      "57.9 million km (0.39 AU)", "58.6 Earth days", "88 Earth days", "0",
      "Closest planet to the Sun, with almost no atmosphere." },

    { "Terrestrial planet", "12,104 km", "4.87 x 10^24 kg (0.815 Earths)",
      "108.2 million km (0.72 AU)", "243 Earth days", "225 Earth days", "0",
      "Hottest planet due to a runaway greenhouse effect." },

    { "Terrestrial planet", "12,742 km", "5.97 x 10^24 kg (1 Earth)",
      "149.6 million km (1.00 AU)", "24 hours", "365.25 days", "1",
      "The only known planet with liquid water and life." },

    { "Terrestrial planet", "6,779 km", "6.42 x 10^23 kg (0.107 Earths)",
      "227.9 million km (1.52 AU)", "24.6 hours", "687 Earth days", "2",
      "Home to Olympus Mons, the tallest volcano we know of." },

    { "Gas giant", "139,820 km", "1.90 x 10^27 kg (317.8 Earths)",
      "778.5 million km (5.20 AU)", "9.9 hours", "~12 Earth years", "95",
      "Largest planet; its Great Red Spot is a giant storm." },

    { "Gas giant", "116,460 km", "5.68 x 10^26 kg (95.2 Earths)",
      "1.43 billion km (9.58 AU)", "10.7 hours", "~29 Earth years", "146",
      "Famous for its dramatic rings made of ice and rock." },

    { "Ice giant", "50,724 km", "8.68 x 10^25 kg (14.5 Earths)",
      "2.87 billion km (19.2 AU)", "17.2 hours", "~84 Earth years", "27",
      "Rotates on its side, almost like it's rolling along its orbit." },

    { "Ice giant", "49,244 km", "1.02 x 10^26 kg (17.1 Earths)",
      "4.50 billion km (30.1 AU)", "16.1 hours", "~165 Earth years", "14",
      "Windiest planet, with storms over 2,000 km/h." },
};



struct CollisionEffect
{
    glm::vec3 pos;
    glm::vec3 sparkDir[8];
    float     spawnTime;
    float     scaleMul = 1.0f;
    float     lifetimeMul = 1.0f;
};

inline std::vector<CollisionEffect> collisionEffects;



struct Shockwave { glm::vec3 pos, normal; float spawnTime, size, lifetime; glm::vec3 color; };

inline std::vector<Shockwave> shockwaves;



struct PendingStrike
{
    int       body;
    int       uid;
    glm::vec3 localDir;
    Weapon    weapon;
    float     fireTime;
    float     duration;
    glm::vec3 startWorld;
};

inline std::vector<PendingStrike> pendingStrikes;



struct WeaponSpec { float damage, radius, shake, kick; };

inline const WeaponSpec kWeaponSpecs[W_COUNT] = {
    { 25.0f, 0.38f, 0.9f, 0.5f },
    {  7.0f, 0.15f, 0.3f, 0.08f },
    { 30.0f, 0.32f, 1.2f, 1.2f },
    {  1.3f, 0.06f, 0.05f, 0.0f },
    { 100.0f, 1.0f, 2.0f, 0.0f },
};

inline int g_nextUid = 1;

inline const float kCrashSpeed = 4.0f;

inline const float kMergeSpeed = 2.0f;

inline const float kSidebarWidth = 170.0f;

inline const float kSidebarTop = 20.0f;

inline const float kSidebarItemH = 24.0f;

inline const float kWeaponBtnW = 112.0f;

inline const float kWeaponBtnH = 26.0f;

inline const float kWeaponBtnGap = 6.0f;

inline int hoveredBody = -1;



struct Asteroid {
    float orbitRadius, orbitAngle, orbitSpeed, size, yOffset;
};



struct ShootingStar {
    glm::vec3 origin;
    glm::vec3 direction;
    float     speed;
    float     length;
    float     spawnTime;
    float     lifetime;
    bool      active;
};


// ---------------------------------------------------------------------------
// Function prototypes (implementations live in the matching .cpp file)
// ---------------------------------------------------------------------------
void audioInit();
void audioShutdown();
void audioPlayLaser();
void audioPlayLaunch();
void audioPlayCollision();
void audioPlayExplosion();
void audioPlayUiBlip();
void audioSetAmbient(bool enabled);

// defined inside Audio.cpp (both branches of the #if HAVE_MINIAUDIO guard),
// but toggled from Input.cpp's processInput() with the 'M' key.
extern bool g_audioMuted;
void pushToast(const std::string& text);
bool keyPressedOnce(GLFWwindow* w, int key);
void triggerKonamiEasterEgg();
void feedKonamiKey(int key);
void syncYawPitchFromFront();
void cancelCinematic();
void startCinematic(const glm::vec3& center, float radius);
void orbitFocus(float dYawDeg, float dPitchDeg);
float randf();
void framebuffer_size_callback(GLFWwindow*, int w, int h);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow*, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int compileShader(unsigned int type, const char* src);
unsigned int buildShaderProgram(const char* vs, const char* fs);
unsigned int loadTexture(const char* path);
void buildSphere(unsigned int sectors, unsigned int stacks,
    std::vector<float>& verts, std::vector<unsigned int>& idx);
void buildRing(float inner, float outer, int segs,
    std::vector<float>& verts, std::vector<unsigned int>& idx);
void buildOrbitCircle(int segs, std::vector<float>& verts);
void buildShootingStarLine(std::vector<float>& verts);
GpuMesh uploadIndexed(const std::vector<float>& v, const std::vector<unsigned int>& idx, int stride, bool hasUV);
GpuMesh uploadVertexOnly(const std::vector<float>& v, int fpv);
void drawLine3D(const GpuMesh& m, const glm::vec3& a, const glm::vec3& b);
void drawUiQuad(unsigned int vao, unsigned int vbo,
    float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void drawUiRect(unsigned int vao, unsigned int vbo, float x, float y, float w, float h,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void drawUiLine(unsigned int vao, unsigned int vbo, float x0, float y0, float x1, float y1,
    float thickness, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void drawUiText(unsigned int vao, unsigned int vbo, float x, float y, const char* text,
    unsigned char r, unsigned char g, unsigned char b, unsigned char a);
glm::vec3 bodyWorldPos(const Body& b, float );
void spawnCollisionEffect(const glm::vec3& pos, float scaleMul = 1.0f, float lifetimeMul = 1.0f);
void spawnShockwave(const glm::vec3& pos, const glm::vec3& normal, float size, float lifetime, const glm::vec3& color);
glm::vec3 localDirToWorldPoint(const Body& b, const glm::vec3& localDir);
glm::vec3 worldToLocalDir(const Body& b, const glm::vec3& worldPoint);
glm::mat4 alignYTo(const glm::vec3& n);
glm::vec3 jitterDir(const glm::vec3& d, float maxAngle);
void uploadImpacts(const Body& b, int uCount, int uImp, int uAges);
std::string formatPopulation(double p);
bool addBody(Body b);
void spawnDebrisChunk(const glm::vec3& pos, const glm::vec3& vel, unsigned int rockTexture, double mass, float sizeMul);
void destroyBody(int targetIndex);
void killBody(int idx);
void supernova();
void fireBattleStationAt(int targetIndex);
void applyImpact(int idx, const glm::vec3& localDir, Weapon w);
void fireWeapon(int idx, const glm::vec3& hitWorld);
void updatePendingStrikes();
void damageFromCollision(int idx, int otherIdx, float dmg);
void runScenario(int id);
void spawnProjectile(const glm::vec3& pos, const glm::vec3& vel, bool isRocket = false, bool isBlackHole = false);
std::string saveSlotPath(int slot);
bool saveSimulation(const char* path);
bool loadSimulation(const char* path);
void computeAccelerations(const std::vector<Body>& srcBodies, std::vector<glm::vec3>& outAccel);
void stepPhysics(float dt);
std::vector<glm::vec3> predictTrajectory(const glm::vec3& startPos, const glm::vec3& startVel, int steps, float dt);
bool raySphereIntersect(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& center, float radius, float& outT);
bool screenToWorldRay(GLFWwindow* window, glm::vec3& outOrigin, glm::vec3& outDir);
int sidebarHitTest(GLFWwindow* window);
int weaponMenuHitTest(GLFWwindow* window);
glm::vec3 computeSlingVelocity(GLFWwindow* window);
void selectBody(int idx);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void showLoading(GLFWwindow* window, const char* message);

