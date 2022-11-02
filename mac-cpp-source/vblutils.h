#ifndef VBLUTILS_H
#define VBLUTILS_H

#include <Retrace.h>

typedef pascal void (*VBLProcPtr)(VBLTaskPtr recPtr);

struct EVBLTask {
    unsigned long ourA5; // Pointer to base of A5 world
    VBLProcPtr ourProc;  // Address of VBL routine written in a high-level language
    VBLTask    vblTask;
};

OSErr EVInstall(EVBLTask *task);
OSErr EVRemove(EVBLTask *task);

#endif