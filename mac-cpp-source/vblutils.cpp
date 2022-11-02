#include "vblutils.h"

asm pascal void preVBLTask();
OSErr disposePersistentVBLTask(VBLTaskPtr task);
OSErr makeVBLTaskPersistent(VBLTaskPtr task);


OSErr EVInstall(EVBLTask *task) {
	VBLTaskPtr ptr;
	ptr = &(task->vblTask);
	
	task->ourA5 = SetCurrentA5();
	ptr->qType = 1;
	ptr->vblAddr = &preVBLTask;
	
	// makeVBLTaskPersistent(ptr);
	return VInstall((QElem*)ptr);
}

OSErr EVRemove(EVBLTask *task) {
	VBLTaskPtr ptr;
	OSErr err;
	ptr = &(task->vblTask);
	err = VRemove((QElem*)ptr);
	//disposePersistentVBLTask(ptr);
	return err;
}

// From Inside Macintosh: Process page 4-20, Using the Vertical Retrace Manager
OSErr makeVBLTaskPersistent(VBLTaskPtr task) {
    struct JMPInstr {
        unsigned short  opcode;
        void            *address;
    } *sysHeapPtr;

    // get a block in the system heap
    sysHeapPtr = (JMPInstr*) NewPtrSysClear(sizeof(JMPInstr));
    OSErr err = MemError();
    if(err != noErr) return err;

    // populate the instruction
    sysHeapPtr->opcode  = 0x4EF9;       // this is an absolute JMP
    sysHeapPtr->address = task->vblAddr; // this is the JMP address

    task->vblAddr = (VBLUPP) sysHeapPtr;
    return noErr;
}

OSErr disposePersistentVBLTask(VBLTaskPtr task) {
    DisposPtr((Ptr)task->vblAddr);
    task->vblAddr = 0;
    return MemError();
}

asm pascal void preVBLTask() {
    link    a6,#0                // Link for the debugger
    movem.l a5,-(sp)             // Preserve the A5 register
    move.l  a0,-(sp)             // Pass PB pointer as the parameter
    move.l  -8(a0),a5            // Set A5 to passed value (ourA5).
    move.l  -4(a0),a0            // A0 = real completion routine address
    jsr     (a0)                 // Transfer control to ourCompletion
    movem.l (sp)+,a5             // Restore A5 register
    unlk    a6                   // Unlink.
    rts                          // Return
    dc.b    0x8A,"PreVBLTask"
    dc.w    0x0000
}