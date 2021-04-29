#pragma once

#include "cons.h"

typedef struct
{
    console *c;
    bool *thFin;
} dumpArgs;

dumpArgs *dumpArgsCreate(console *c, bool *status);
void dumpArgsDestroy(dumpArgs *a);
void dumpThread(void *arg);
void cleanThread(void *arg);
void cleanPending(void *arg);
void daybreak();
void deletedump();
void deleteDirectoryContents(const std::string& dir_path);
