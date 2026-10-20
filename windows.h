#ifndef WINDOWS_H
#define WINDOWS_H
#include <unistd.h>
#define Sleep(ms) usleep((ms) * 1000)
#endif
