#include "Globals.h"


void pushToast(const std::string& text)
{
    if (toasts.size() > 4) toasts.erase(toasts.begin());
    toasts.push_back({ text, g_wallTime });
    audioPlayUiBlip();
}

