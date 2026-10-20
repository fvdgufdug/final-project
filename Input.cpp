#include "Globals.h"



bool keyPressedOnce(GLFWwindow* w, int key)
{
    static bool last[GLFW_KEY_LAST + 1] = {};
    bool now = (glfwGetKey(w, key) == GLFW_PRESS);
    bool fired = now && !last[key];
    last[key] = now;
    return fired;
}



void syncYawPitchFromFront()
{
    yaw = glm::degrees(atan2f(cameraFront.z, cameraFront.x));
    pitch = glm::degrees(asinf(glm::clamp(cameraFront.y, -1.0f, 1.0f)));
}



void cancelCinematic()
{
    if (!cinematicActive) return;
    cinematicActive = false;
    syncYawPitchFromFront();
}



void startCinematic(const glm::vec3& center, float radius)
{
    cinematicCenter = center;
    cinematicOffset = cameraPos - center;
    float len = glm::length(cinematicOffset);
    if (len < 0.001f) { cinematicOffset = glm::vec3(0.6f, 0.35f, 0.7f); len = glm::length(cinematicOffset); }
    float want = std::max(len, radius * 5.0f + 6.0f);
    cinematicOffset = cinematicOffset / len * want;
    cinematicActive = true;
    cinematicTimer = 3.5f;
}



void orbitFocus(float dYawDeg, float dPitchDeg)
{
    float len = glm::length(focusOffset);
    if (len < 0.001f) return;
    glm::vec3 off = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(dYawDeg), glm::vec3(0, 1, 0)) * glm::vec4(focusOffset, 0.0f));
    glm::vec3 right = glm::cross(off, glm::vec3(0, 1, 0));
    if (glm::length(right) > 0.0001f)
    {
        right = glm::normalize(right);
        glm::vec3 t = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(dPitchDeg), right) * glm::vec4(off, 0.0f));
        if (fabsf(t.y / len) < 0.97f) off = t;
    }
    focusOffset = off;
}



float randf() { return (float)rand() / (float)RAND_MAX; }



void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }



void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

    if (!cursorDisabled)
    {
        float dx = (float)xpos - lastX, dy = (float)ypos - lastY;
        lastX = (float)xpos; lastY = (float)ypos;
        if (selectedBody >= 0 && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
            orbitFocus(-dx * 0.3f, dy * 0.3f);
        return;
    }

    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xo = ((float)xpos - lastX) * 0.1f, yo = (lastY - (float)ypos) * 0.1f;
    lastX = (float)xpos; lastY = (float)ypos;
    yaw += xo; pitch += yo;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(f);
}



void scroll_callback(GLFWwindow*, double xoffset, double yoffset)
{

    if (selectedBody >= 0)
    {
        float len = glm::length(focusOffset);
        if (len > 0.001f)
        {
            float nl = glm::clamp(len * (1.0f - (float)yoffset * 0.08f), focusMinDist, 150.0f);
            focusOffset *= nl / len;
        }
        return;
    }

    fov -= (float)yoffset * 2.0f;
    if (fov < 15.0f) fov = 15.0f;
    if (fov > 90.0f) fov = 90.0f;
}



void processInput(GLFWwindow* window)
{
    if (keyPressedOnce(window, GLFW_KEY_UP))    feedKonamiKey(GLFW_KEY_UP);
    if (keyPressedOnce(window, GLFW_KEY_DOWN))  feedKonamiKey(GLFW_KEY_DOWN);
    if (keyPressedOnce(window, GLFW_KEY_LEFT))  feedKonamiKey(GLFW_KEY_LEFT);
    if (keyPressedOnce(window, GLFW_KEY_RIGHT)) feedKonamiKey(GLFW_KEY_RIGHT);
    if (keyPressedOnce(window, GLFW_KEY_B))     feedKonamiKey(GLFW_KEY_B);
    if (keyPressedOnce(window, GLFW_KEY_A))     feedKonamiKey(GLFW_KEY_A);

    float speed = 8.0f * deltaTime;
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += speed * right;
    bool tabNow = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
    if (tabNow && !tabPressedLastFrame)
    {
        cursorDisabled = !cursorDisabled;
        glfwSetInputMode(window, GLFW_CURSOR, cursorDisabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (cursorDisabled) firstMouse = true;
    }
    tabPressedLastFrame = tabNow;

    bool pNow = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
    if (pNow && !pPressedLastFrame) paused = !paused;
    pPressedLastFrame = pNow;

    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        timeScale = std::min(timeScale + 1.5f * deltaTime, 8.0f);
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        timeScale = std::max(timeScale - 1.5f * deltaTime, 0.0f);

    bool tNow = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
    if (tNow && !tPressedLastFrame)
    {
        tourActive = !tourActive;
        if (tourActive) { tourIndex = -1; tourTimer = 0.0f; }
        else selectedBody = -1;
    }
    tPressedLastFrame = tNow;

    bool rNow = (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS);
    if (rNow && !rPressedLastFrame) resetRequested = true;
    rPressedLastFrame = rNow;

    bool oNow = (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS);
    if (oNow && !oPressedLastFrame) saveRequested = true;
    oPressedLastFrame = oNow;

    bool lNow = (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS);
    if (lNow && !lPressedLastFrame) loadRequested = true;
    lPressedLastFrame = lNow;

    bool d1Now = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
    if (d1Now && !digit1PressedLastFrame) { activeSaveSlot = 1; pushToast("Save slot 1 selected"); }
    digit1PressedLastFrame = d1Now;

    bool d2Now = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
    if (d2Now && !digit2PressedLastFrame) { activeSaveSlot = 2; pushToast("Save slot 2 selected"); }
    digit2PressedLastFrame = d2Now;

    bool d3Now = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
    if (d3Now && !digit3PressedLastFrame) { activeSaveSlot = 3; pushToast("Save slot 3 selected"); }
    digit3PressedLastFrame = d3Now;

    bool hNow = (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS);
    if (hNow && !hPressedLastFrame) helpVisible = !helpVisible;
    hPressedLastFrame = hNow;

    bool mNow = (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS);
    if (mNow && !mPressedLastFrame)
    {
        g_audioMuted = !g_audioMuted;
        pushToast(g_audioMuted ? "Sound muted" : "Sound on");
    }
    mPressedLastFrame = mNow;

    if (keyPressedOnce(window, GLFW_KEY_E))
    {
        currentWeapon = (Weapon)((currentWeapon + 1) % W_COUNT);
        pushToast(std::string("Weapon: ") + weaponNames[currentWeapon]);
    }
    if (keyPressedOnce(window, GLFW_KEY_Q))
    {
        currentWeapon = (Weapon)((currentWeapon + W_COUNT - 1) % W_COUNT);
        pushToast(std::string("Weapon: ") + weaponNames[currentWeapon]);
    }
    if (keyPressedOnce(window, GLFW_KEY_C))
    {
        cinematicEnabled = !cinematicEnabled;
        if (!cinematicEnabled) cancelCinematic();
        pushToast(cinematicEnabled ? "Cinematic camera on" : "Cinematic camera off");
    }

    if (keyPressedOnce(window, GLFW_KEY_7)) scenarioRequested = 1;
    if (keyPressedOnce(window, GLFW_KEY_8)) scenarioRequested = 2;
    if (keyPressedOnce(window, GLFW_KEY_9)) scenarioRequested = 3;
    if (keyPressedOnce(window, GLFW_KEY_0)) scenarioRequested = 4;

    if (selectedBody >= 0)
    {
        float rot = 70.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  orbitFocus(-rot, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) orbitFocus(rot, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    orbitFocus(0.0f, rot);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  orbitFocus(0.0f, -rot);
    }

    if (cinematicActive &&
        (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS))
        cancelCinematic();
}



bool raySphereIntersect(const glm::vec3& ro, const glm::vec3& rd, const glm::vec3& center, float radius, float& outT)
{
    glm::vec3 oc = ro - center;
    float b = glm::dot(oc, rd);
    float c = glm::dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) return false;
    float sq = sqrtf(disc);
    float t0 = -b - sq, t1 = -b + sq;
    float t = (t0 > 0.0f) ? t0 : t1;
    if (t < 0.0f) return false;
    outT = t;
    return true;
}



bool screenToWorldRay(GLFWwindow* window, glm::vec3& outOrigin, glm::vec3& outDir)
{
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW <= 0 || winH <= 0) return false;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (cursorDisabled) { mx = winW * 0.5; my = winH * 0.5; }
    float nx = (float)(mx / winW) * 2.0f - 1.0f;
    float ny = 1.0f - (float)(my / winH) * 2.0f;

    glm::mat4 invVP = glm::inverse(g_lastProj * g_lastView);
    glm::vec4 nearP = invVP * glm::vec4(nx, ny, -1.0f, 1.0f);
    glm::vec4 farP = invVP * glm::vec4(nx, ny, 1.0f, 1.0f);
    nearP /= nearP.w; farP /= farP.w;

    outOrigin = glm::vec3(nearP);
    outDir = glm::normalize(glm::vec3(farP - nearP));
    return true;
}



int sidebarHitTest(GLFWwindow* window)
{
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float listX = (float)winW - kSidebarWidth;
    for (int i = 0; i < (int)bodies.size(); ++i)
    {
        float top = kSidebarTop + 30.0f + i * kSidebarItemH;
        float bottom = top + kSidebarItemH;
        if (mx >= listX && mx <= (double)winW && my >= top && my <= bottom)
            return bodies[i].destroyed ? -1 : i;
    }
    return -1;
}



int weaponMenuHitTest(GLFWwindow* window)
{
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    float total = W_COUNT * kWeaponBtnW + (W_COUNT - 1) * kWeaponBtnGap;
    float x0 = ((float)winW - total) * 0.5f;
    float y0 = (float)winH - kWeaponBtnH - 14.0f;
    for (int w = 0; w < W_COUNT; ++w)
    {
        float x = x0 + w * (kWeaponBtnW + kWeaponBtnGap);
        if (mx >= x && mx <= x + kWeaponBtnW && my >= y0 && my <= y0 + kWeaponBtnH) return w;
    }
    return -1;
}



glm::vec3 computeSlingVelocity(GLFWwindow* window)
{
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    double dx = mx - slingStartMouseX;
    double dy = my - slingStartMouseY;
    float dragLen = (float)sqrt(dx * dx + dy * dy);
    float speed = glm::clamp(kSlingMinSpeed + dragLen * kSlingDragToSpeed, kSlingMinSpeed, kSlingMaxSpeed);

    glm::vec3 right = glm::normalize(glm::cross(slingBaseDir, cameraUp));
    glm::vec3 up2 = glm::normalize(glm::cross(right, slingBaseDir));

    glm::vec3 dir = slingBaseDir - right * ((float)dx * kSlingDragToAim) + up2 * ((float)dy * kSlingDragToAim);
    if (glm::length(dir) < 0.0001f) dir = slingBaseDir;
    return glm::normalize(dir) * speed;
}



void selectBody(int idx)
{
    selectedBody = idx;
    glm::vec3 pos = bodyWorldPos(bodies[idx], g_simTime);
    float dist = bodies[idx].scaledSize * 8.0f;
    if (dist < 6.0f) dist = 6.0f;
    glm::vec3 dirFromPlanet = cameraPos - pos;
    float len = glm::length(dirFromPlanet);
    dirFromPlanet = (len > 0.001f) ? (dirFromPlanet / len) : glm::vec3(0, 0, 1);
    focusOffset = dirFromPlanet * dist;
}



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS) cancelCinematic();

    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS)
        tourActive = false;

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS && cursorDisabled)
    {
        glm::vec3 spawnPos = cameraPos + cameraFront * 4.0f;
        glm::vec3 spawnVel = cameraFront * 15.0f;
        spawnProjectile(spawnPos, spawnVel, false, true);
        audioPlayLaunch();
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && slingCharging)
    {
        glm::vec3 vel = computeSlingVelocity(window);
        spawnProjectile(slingOrigin, vel);
        audioPlayLaunch();
        slingCharging = false;
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {

        if (!cursorDisabled)
        {
            int wIdx = weaponMenuHitTest(window);
            if (wIdx >= 0)
            {
                currentWeapon = (Weapon)wIdx;
                pushToast(std::string("Weapon: ") + weaponNames[currentWeapon]);
                return;
            }
        }

        if (selectedBody >= 0 && selectedBody < (int)bodies.size() && !bodies[selectedBody].destroyed)
        {
            bool onSidebar = (!cursorDisabled && sidebarHitTest(window) >= 0);
            glm::vec3 ro, rd; float t;
            if (!onSidebar && screenToWorldRay(window, ro, rd) &&
                raySphereIntersect(ro, rd, bodies[selectedBody].position, bodies[selectedBody].scaledSize, t))
            {
                fireWeapon(selectedBody, ro + rd * t);
                return;
            }
        }

        if (cursorDisabled)
        {

            glm::vec3 spawnPos = cameraPos + cameraFront * 3.0f;
            glm::vec3 spawnVel = cameraFront * 30.0f;
            spawnProjectile(spawnPos, spawnVel);
            audioPlayLaunch();
            return;
        }

        float time = g_simTime;
        int   hitIndex = sidebarHitTest(window);

        if (hitIndex < 0)
        {
            glm::vec3 ro, rd;
            if (!screenToWorldRay(window, ro, rd)) return;

            float closestT = 1e9f;
            for (int i = 0; i < (int)bodies.size(); ++i)
            {
                if (bodies[i].destroyed) continue;
                glm::vec3 pos = bodyWorldPos(bodies[i], time);
                float pickRadius = bodies[i].scaledSize * 1.3f;
                float t;
                if (raySphereIntersect(ro, rd, pos, pickRadius, t) && t < closestT)
                {
                    closestT = t;
                    hitIndex = i;
                }
            }
        }

        if (hitIndex >= 0)
        {
            selectBody(hitIndex);
        }
        else
        {

            slingCharging = true;
            glfwGetCursorPos(window, &slingStartMouseX, &slingStartMouseY);
            slingOrigin = cameraPos + cameraFront * 2.0f;
            slingBaseDir = cameraFront;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (cursorDisabled)
        {

            glm::vec3 spawnPos = cameraPos + cameraFront * 3.0f;
            glm::vec3 spawnVel = cameraFront * 40.0f;
            spawnProjectile(spawnPos, spawnVel, true);
            audioPlayLaunch();
            return;
        }

        slingCharging = false;

        if (selectedBody != -1)
        {

            syncYawPitchFromFront();
            selectedBody = -1;
        }
    }
}

