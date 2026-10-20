#ifndef DIRECT_H
#define DIRECT_H
#include <unistd.h>
#include <sys/stat.h>
#define _getcwd getcwd
#define _mkdir(dir) mkdir(dir, 0777)
#define _chdir chdir
#endif
