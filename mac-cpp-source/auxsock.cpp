#include "auxsock.h"

long scerrno = 0;

long sockerr() {
	return scerrno;
}

asm long auxsocket(long domain, long type, long protocol) {
	move.l d2, -(sp)
	movea.l 8(sp), a0
	move.l 12(sp), d1
	movea.l 16(sp), a1
	moveq #0x5c, d0
	trap #0xf
	bcs.w err
	move.l (sp)+, d2
	rts
err:
	move.l (sp)+, d2
	jmp cerror
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxbind(long socket, struct sockaddr* address, long address_len) {
	move.l d2, -(sp)
	movea.l 8(sp), a0
	move.l 12(sp), d1
	movea.l 16(sp), a1
	moveq #0x47, d0
	trap #0xf
	bcs.w err
	move.l (sp)+, d2
	rts
err:
	move.l (sp)+, d2
	jmp cerror
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxlisten(long socket, long backlog) {
	movea.l 4(sp), a0
	move.l 8(sp), d1
	moveq #0x4e, d0
	trap #0xf
	bcs.w err
	rts
err:
	jmp cerror
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxaccept(long socket, struct sockaddr* address, long* address_len) {
	move.l d2, -(sp)
	movea.l 8(sp), a0
	move.l 12(sp), d1
	movea.l 16(sp), a1
	moveq #0x46, d0
	trap #0xf
	bcs.w err
	move.l (sp)+, d2
	rts
err:
	move.l (sp)+, d2
	jmp cerror
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}


asm long auxfcntl(long filedes, long x...) {
	moveq #0x3e, d0
	trap #0x0
	bcc.w noerr
	jmp cerror

noerr:
	rts
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}


asm long auxsend(long sock, char* msg, long len, long flags) {
	move.l d2, -(sp)
	movea.l 8(sp), a0
	move.l 12(sp), d1
	movea.l 16(sp), a1
	move.l 20(sp), d2
	moveq #0x53, d0
	trap #0xf
	bcs.w senderr
	move.l (sp)+, d2
	rts
senderr:
	move.l (sp)+, d2
	jmp scerror
	
scerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxrecv(long sock, char* msg, long len, long flags) {
	move.l d2, -(sp)
	movea.l 8(sp), a0
	move.l 12(sp), d1
	movea.l 16(sp), a1
	move.l 20(sp), d2
	moveq #0x4f, d0
	trap #0xf
	bcs.w recverr
	move.l (sp)+, d2
	rts
recverr:
	move.l (sp)+, d2
	jmp scerror
	
scerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}


asm long auxselect(long nfds, unsigned long* readfds, unsigned long* writefds, unsigned long* errfds, struct timeval *timeout) {
	movem.l d2/a2, -(sp)
	movea.l 12(sp), a0
	move.l 16(sp), d1
	movea.l 20(sp), a1
	move.l 24(sp), d2
	move.l 28(sp), a2
	moveq #0x52, d0
	trap #0xf
	bcs.w selerr
	movem.l (sp)+, d2/a2
	rts
	
selerr:
	movem.l (sp)+, d2/a2
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxioctl(unsigned long x...) {
	moveq #0x36, d0
	trap #0
	bcc.s noerror
	jmp cerror
noerror:
	rts
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts

}


asm long auxopen(char* path, long flags) {
	moveq #5, d0
	trap #0
	bcc.s noerror
	jmp cerror
noerror:
	rts
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}

asm long auxclose(long fd) {
	moveq #6, d0
	trap #0
	bcc.s noerror
	jmp cerror
noerror:
	rts
	
cerror:
	move.l d0, scerrno
	moveq #-1, d0
	movea.l d0, a0
	rts
}
