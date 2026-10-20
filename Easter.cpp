#include "Globals.h"



const int kKonamiSequence[10] = {
    GLFW_KEY_UP, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_DOWN,
    GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
    GLFW_KEY_B, GLFW_KEY_A
};

int g_konamiProgress = 0;



void triggerKonamiEasterEgg()
{
    pushToast("Never gonna give you up...");
    audioPlayUiBlip();
    const std::string url = "nevergonna.mp4";
#ifdef _WIN32
    std::string cmd = "start \"\" \"" + url + "\"";
#elif defined(__APPLE__)
    std::string cmd = "( open \"" + url + "\"; sleep 1; osascript -e 'tell application \"QuickTime Player\" to play document 1' ) >/dev/null 2>&1 &";
#else
    std::string cmd = "xdg-open \"" + url + "\" >/dev/null 2>&1 &";
#endif
    system(cmd.c_str());
}



void feedKonamiKey(int key)
{
    if (key == kKonamiSequence[g_konamiProgress])
    {
        ++g_konamiProgress;
        if (g_konamiProgress >= (int)(sizeof(kKonamiSequence) / sizeof(kKonamiSequence[0])))
        {
            g_konamiProgress = 0;
            triggerKonamiEasterEgg();
        }
    }
    else
    {
        g_konamiProgress = (key == kKonamiSequence[0]) ? 1 : 0;
    }
}

