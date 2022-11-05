/****************************************************************************
 *   MiniVNC (c) 2022 Marcio Teixeira                                       *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   To view a copy of the GNU General Public License, go to the following  *
 *   location: <http://www.gnu.org/licenses/>.                              *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <Devices.h>
#include <OSUtils.h>

#include "aux.h"
#include "auxsock.h"
#include "vblutils.h"
#include "lowmem.h"
#include "VNCServer.h"
#include "VNCKeyboard.h"
#include "VNCTypes.h"
#include "VNCFrameBuffer.h"
#include "VNCScreenHash.h"
#include "VNCEncodeTRLE.h"
#include "ChainedTCPHelper.h"

#define VNC_DEBUG

#ifndef USE_STDOUT
    #define printf ShowStatus
    int ShowStatus(const char* format, ...);
#endif

enum {
    VNC_STOPPED,
    VNC_STARTING,
    VNC_WAITING,
    VNC_CONNECTED,
    VNC_RUNNING,
    VNC_STOPPING,
    VNC_ERROR
} vncState = VNC_STOPPED;

Boolean allowControl = true, sendGraphics = true, allowIncremental = true, fbColorMapNeedsUpdate = true;

pascal void poller(VBLTaskPtr t);
long auxsendw(long sock, char* msg, long length, long flags);
long auxrecvw(long sock, char* msg, long length, long flags);

pascal void tcpAcceptConnection(TCPiopb *pb);
pascal void tcpStreamClosed(TCPiopb *pb);
pascal void tcpSendProtocolVersion(TCPiopb *pb);
pascal void tcpReceiveClientProtocolVersion(TCPiopb *pb);
pascal void tcpRequestClientProtocolVersion(TCPiopb *pb);
pascal void tcpSendSecurityHandshake(TCPiopb *pb);
pascal void tcpRequestClientInit(TCPiopb *pb);
pascal void tcpSendServerInit(TCPiopb *pb);
pascal void vncPeekMessage(TCPiopb *pb);
pascal void vncFinishMessage(TCPiopb *pb);
pascal void vncStartSlowMessageHandling(TCPiopb *pb);
pascal void vncHandleMessageSlowly(TCPiopb *pb);
pascal void vncSendColorMapEntries(TCPiopb *pb);

void processMessageFragment(const char *src, unsigned long len);

unsigned long getSetEncodingFragmentSize(unsigned long bytesRead);
void processSetEncodingsFragment(unsigned long bytesRead, char *&dst);

void vncSetPixelFormat(const VNCSetPixFormat &);
void vncEncoding(unsigned long);
void vncKeyEvent(const VNCKeyEvent &);
void vncPointerEvent(const VNCPointerEvent &);
void vncClientCutText(const VNCClientCutText &);
void vncFBUpdateRequest(const VNCFBUpdateReq &);
void vncDeferUpdateRequest(const VNCFBUpdateReq &req);
void vncSendFBUpdate(Boolean incremental);

pascal void vncGotDirtyRect(int x, int y, int w, int h);
pascal void vncSendFBUpdateColorMap(TCPiopb *pb);
pascal void vncSendFBUpdateHeader(TCPiopb *pb);
pascal void vncSendFBUpdateRow(TCPiopb *pb);
pascal void vncSendFBUpdateFinished(TCPiopb *pb);

long               listenFD = -1;
long               vncFD = -1;
long               clientFD = -1;
OSErr              vncError;
char              *vncServerVersion = "RFB 003.003\n";
char               vncClientVersion[13];
long               vncSecurityHandshake = 1;
char               vncClientInit;
VNCServerInit      vncServerInit;
VNCClientMessages  vncClientMessage;
VNCServerMessages  vncServerMessage;
TCPCompletionPtr wantsRecv;

wdsEntry           myWDS[3] = {0};

char horribleBuffer[512];

VNCRect            fbUpdateRect;
Boolean            fbUpdateInProgress, fbUpdatePending;

EVBLTask pollTask;

void vncCheckForActivity() {
	if (vncFD < 0) { return; }
	
	static long oldVNCFD = -1;
	static int first_time = 0;
	long ready_fds;
	unsigned long wr_fdset;
	unsigned long rd_fdset;
	unsigned long err_fdset;
	struct timeval timeout;
	TCPCompletionPtr wr;
	
	wr_fdset = 0;
	rd_fdset = 1 << vncFD;
	err_fdset = 0;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	
	if (oldVNCFD != vncFD) {
		printf("vncFD changed, %ld => %ld\n", oldVNCFD, vncFD);
		oldVNCFD = vncFD;
		first_time = 0;
	}
	
	if (!first_time) {
		printf("\nvncFD = %ld, rd_fdset: %ld\n", vncFD, rd_fdset);
		first_time++;
	}
	
	ready_fds = auxselect(vncFD + 1, &rd_fdset, &wr_fdset, &err_fdset, &timeout);
	while (ready_fds > 0) {
		if (first_time == 1) {
			printf("saw a ready fd! rd_fset = %ld\n", rd_fdset);
			first_time++;
		}
		if (!wantsRecv) { return; }
		if (first_time == 2) {
			printf("calling wantsRecv\n");
			first_time++;
		}
		
		wr = wantsRecv;
		wantsRecv = nil;
		wr(nil);
		
		wr_fdset = 0;
		rd_fdset = 1 << vncFD;
		err_fdset = 1 << vncFD;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		ready_fds = auxselect(1, &rd_fdset, &wr_fdset, &err_fdset, &timeout);
	}
	if (ready_fds < 0) {
		// do some error handling I gue7ss
		printf("select returned %ld; errno %ld\n", ready_fds, sockerr());
	}
}

long auxsendw(long sock, char* msg, long length, long flags) {
	long ret;
	ret = auxsend(sock, msg, length, flags);
	
	if (ret < 0 && sockerr() != 32) {
		printf("send err: %ld fd %ld\n", sockerr(), sock);
	} else if (ret < length) {
		printf("sent too little: %ld vs %ld\n", ret, length);
	}
	
	return ret;
}

long auxrecvw(long sock, char* msg, long length, long flags) {
	long ret;
	long oldflags;
	long newflags;

	// force blocking	
	oldflags = auxfcntl(sock, F_GETFL, 0);
	newflags = oldflags & ~O_NDELAY;
	auxfcntl(sock, F_SETFL, newflags);
	
top:
	ret = auxrecv(sock, msg, length, flags);
	
	if (ret < 0) {
		printf("recv err: %ld fd %ld\n", sockerr(), sock);
		
		if (sockerr() == 4) { // EINTR
			goto top;
		}
	}
	
	auxfcntl(sock, F_SETFL, oldflags);
		
	return ret;
}

pascal void poller(VBLTaskPtr t) {
	vncCheckForActivity();
	
	t->vblCount = 1;
}

OSErr vncServerStart() {
	struct sockaddr_in addr;
	long ret;
	long flags;

    VNCKeyboard::Setup();

    vncError = VNCFrameBuffer::setup();
    if (vncError != noErr) return vncError;

    vncError = VNCEncoder::setup();
    if (vncError != noErr) return vncError;

    vncError = VNCScreenHash::setup();
    if (vncError != noErr) return vncError;
    
    pollTask.ourProc = &poller;
    pollTask.vblTask.vblCount = 1;
    EVInstall(&pollTask);

    fbUpdateInProgress = false;
    fbUpdatePending = false;
    fbColorMapNeedsUpdate = true;

    fbUpdateRect.x = 0;
    fbUpdateRect.y = 0;
    fbUpdateRect.w = 0;
    fbUpdateRect.h = 0;
    
    printf("Starting network...\n");
    listenFD = auxsocket(AF_INET, SOCK_STREAM, 6);
    
    printf("listenFD => %ld\n", listenFD);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 5900;
	ret = auxbind(listenFD, (sockaddr*) &addr, sizeof(addr));
	
	flags = auxfcntl(listenFD, F_GETFL, 0);
	flags |= O_NDELAY;
	auxfcntl(listenFD, F_SETFL, flags);
	
	ret = auxlisten(listenFD, 1);
	printf("listen returned %ld, errno %ld\n", ret, sockerr());
	wantsRecv = tcpAcceptConnection;
	vncFD = listenFD;
	
    return noErr;
}

OSErr vncServerStop() {
    if (vncState != VNC_STOPPED) {
        vncState = VNC_STOPPING;
        vncFD = -1;
        
        if (clientFD > -1) {
        	// todo: neat shutdown
        	auxclose(clientFD);
        	clientFD = -1;
        }
        
        if (listenFD > -1) {
        	auxclose(listenFD);
        	clientFD = -1;
        }
        
        vncState = VNC_STOPPED;
    }

    VNCScreenHash::destroy();
    VNCEncoder::destroy();
    VNCFrameBuffer::destroy();
    
    EVRemove(&pollTask);

    return noErr;
}

Boolean vncServerStopped() {
    return vncState == VNC_STOPPED;
}

OSErr vncServerError() {
    if(vncState == VNC_ERROR) {
        return vncError;
    } else {
        return noErr;
    }
}

pascal void tcpAcceptConnection(TCPiopb *pb) {
	struct sockaddr_in cliaddr;
	long addr_size = sizeof(cliaddr);
	
	printf("in tcpAcceptConnection...\n");

	clientFD = auxaccept(listenFD, (sockaddr*)&cliaddr, &addr_size);
	printf("clientFD => %ld\n", clientFD);
	vncFD = clientFD;
	tcpSendProtocolVersion(nil);
}

pascal void tcpStreamClosed(TCPiopb *pb) {
    vncState = VNC_STOPPED;
}

pascal void tcpSendProtocolVersion(TCPiopb *pb) {
    vncState = VNC_CONNECTED;

    #ifdef VNC_DEBUG
    	printf("socket fd is %ld", vncFD);
        printf("Sending Protocol Version!\n");
    #endif
    
    wantsRecv = tcpRequestClientProtocolVersion;
    auxsendw(vncFD, (Ptr) vncServerVersion, strlen(vncServerVersion), 0);
}

pascal void tcpRequestClientProtocolVersion(TCPiopb *pb) {
    auxrecvw(vncFD, vncClientVersion, 12, 0);
    tcpSendSecurityHandshake(pb);
}

pascal void tcpSendSecurityHandshake(TCPiopb *pb) {
    #ifdef VNC_DEBUG
        printf("Client VNC Version: %s\n", vncClientVersion);
    #endif

    printf("Sending VNC Security Handshake\n");

    wantsRecv = tcpRequestClientInit;
    auxsendw(vncFD, (Ptr) &vncSecurityHandshake, sizeof(long), 0);
}

pascal void tcpRequestClientInit(TCPiopb *pb) {
    auxrecvw(vncFD, (Ptr) &vncClientInit, 1, 0);
    tcpSendServerInit(pb);
}

pascal void tcpSendServerInit(TCPiopb *pb) {
    #ifdef VNC_DEBUG
        printf("Client Init: %d\n", vncClientInit);
    #endif
    // send the VNC security handshake
    printf("Connection established!\n");
    #ifdef VNC_FB_WIDTH
        vncServerInit.fbWidth = VNC_FB_WIDTH;
        vncServerInit.fbHeight = VNC_FB_HEIGHT;
    #else
        vncServerInit.fbWidth = fbWidth;
        vncServerInit.fbHeight = fbHeight;
    #endif
    vncServerInit.format.bigEndian = 1;
    vncServerInit.format.bitsPerPixel = 8;
    #ifdef VNC_FB_BITS_PER_PIX
        vncServerInit.format.depth = VNC_FB_BITS_PER_PIX;
    #else
        vncServerInit.format.depth = fbDepth;
    #endif
    vncServerInit.format.trueColor = 0;
    vncServerInit.nameLength = 10;
    memcpy(vncServerInit.name, "Macintosh ", 10);

    vncState = VNC_RUNNING;

    wantsRecv = vncStartSlowMessageHandling;
    auxsendw(vncFD, (Ptr) &vncServerInit, sizeof(vncServerInit), 0);   
}

void processMessageFragment(const char *src, unsigned long len) {
    static char *dst = (char *)&vncClientMessage;

    /**
     * Process unfragmented mPointerEvent and mFBUpdateRequest messages,
     * the common case
     *
     * Whole messages will be processed without copying. Upon exit, either
     * len = 0 and all data has been consumed, or len != 0 and src will
     * point to messages for further processing.
     */

    if (  (dst == (char *)&vncClientMessage) &&        // If there are no prior fragments...
         ( ((unsigned long)src & 0x01) == 0) ) { // ... and word aligned for 68000
        while(true) {
            if((*src == mPointerEvent) && (len >= sizeof(VNCPointerEvent))) {
                vncPointerEvent(*(VNCPointerEvent*)src);
                src += sizeof(VNCPointerEvent);
                len -= sizeof(VNCPointerEvent);
            }

            else if((*src == mFBUpdateRequest) && (len >= sizeof(VNCFBUpdateReq))) {
                vncDeferUpdateRequest(*(VNCFBUpdateReq*)src);
                src += sizeof(VNCFBUpdateReq);
                len -= sizeof(VNCFBUpdateReq);
            }

            else
                break; // Exit loop for any other message
        }
    }

    /**
     * Deal with fragmented messages, a less common case
     *
     * While processing fragmented messages, src is a read pointer into the
     * RDS while dst is a write pointer into vncClientMessage. Bytes are copied
     * from src to dst until a full message detected, at which point it
     * is processed and dst is reset to the top of vncClientMessage.
     *
     * Certain messages in the VNC protocol are variable size, so a message
     * may be copied in bits until its total size is known.
     */

    unsigned long bytesRead, msgSize;

    while(len) {
        // If we've read no bytes yet, get the message type
        if(dst == (char *)&vncClientMessage) {
            vncClientMessage.message = *src++;
            len--;
            dst++;
        }

        // How many bytes have been read up to this point?
        bytesRead = dst - (char *)&vncClientMessage;

        // Figure out the message length
        switch(vncClientMessage.message) {
            case mSetPixelFormat:  msgSize = sizeof(VNCSetPixFormat); break;
            case mFBUpdateRequest: msgSize = sizeof(VNCFBUpdateReq); break;
            case mKeyEvent:        msgSize = sizeof(VNCKeyEvent); break;
            case mPointerEvent:    msgSize = sizeof(VNCPointerEvent); break;
            case mClientCutText:   msgSize = sizeof(VNCClientCutText); break;
            case mSetEncodings:    msgSize = getSetEncodingFragmentSize(bytesRead); break;
            default:
                printf("Invalid message: %d\n", vncClientMessage.message);
                return;
        }

        // Copy message bytes
        if(bytesRead < msgSize) {
            unsigned long bytesToCopy = msgSize - bytesRead;

            // Copy the message bytes
            if(bytesToCopy > len) bytesToCopy = len;

            BlockMove(src, dst, bytesToCopy);
            src += bytesToCopy;
            len -= bytesToCopy;
            dst += bytesToCopy;
        }

        // How many bytes have been read up to this point?
        bytesRead = dst - (char *)&vncClientMessage;

        if(bytesRead == msgSize) {
            // Dispatch the message
            switch(vncClientMessage.message) {
                // Fixed sized messages
                case mSetPixelFormat:  vncSetPixelFormat(  vncClientMessage.pixFormat); break;
                case mFBUpdateRequest: vncDeferUpdateRequest( vncClientMessage.fbUpdateReq); break;
                case mKeyEvent:        vncKeyEvent(        vncClientMessage.keyEvent); break;
                case mPointerEvent:    vncPointerEvent(    vncClientMessage.pointerEvent); break;
                case mClientCutText:   vncClientCutText(   vncClientMessage.cutText); break;
                // Variable sized messages
                case mSetEncodings:
                    processSetEncodingsFragment(bytesRead, dst);
                    continue;
            }
            // Prepare to receive next message
            dst = (char *)&vncClientMessage;
        }
    }
}

unsigned long getSetEncodingFragmentSize(unsigned long bytesRead) {
    return ((bytesRead >= sizeof(VNCSetEncoding)) && vncClientMessage.setEncoding.numberOfEncodings) ?
        sizeof(VNCSetEncodingOne) :
        sizeof(VNCSetEncoding);
}

void processSetEncodingsFragment(unsigned long bytesRead, char *&dst) {
    if(bytesRead == sizeof(VNCSetEncodingOne)) {
        vncEncoding(vncClientMessage.setEncodingOne.encoding);
        vncClientMessage.setEncodingOne.numberOfEncodings--;
    } 

    if(vncClientMessage.setEncoding.numberOfEncodings) {
        // Preare to read the next encoding
        dst = (char *) &vncClientMessage.setEncodingOne.encoding;
    } else {
        // Prepare to receive next message
        dst = (char *) &vncClientMessage;
    }
}

pascal void vncStartSlowMessageHandling(TCPiopb *pb) {
    long len;
    len = auxrecv(vncFD, horribleBuffer, 512, 0);
    wantsRecv = vncStartSlowMessageHandling;

    if (len > 0) {
    	processMessageFragment(horribleBuffer, len);
    }    
}

void vncEncoding(unsigned long) {
    #ifdef VNC_DEBUG
        printf("Got encoding %ld\n", vncClientMessage.setEncodingOne.encoding);
    #endif
}

void vncSetPixelFormat(const VNCSetPixFormat &pixFormat) {
    const VNCPixelFormat &format = pixFormat.format;
    #ifdef VNC_FB_BITS_PER_PIX
        const unsigned char fbDepth = VNC_FB_BITS_PER_PIX;
    #endif
    if (format.trueColor || format.depth != fbDepth) {
    	// Invalid colour depth
    	printf("invalid colour depth %ld\n", format.depth);
        vncState = VNC_STOPPED;
        vncServerStop();
    }
}

void vncKeyEvent(const VNCKeyEvent &keyEvent) {
    if (allowControl) {
        VNCKeyboard::PressKey(keyEvent.key, keyEvent.down);
    }
}

void vncPointerEvent(const VNCPointerEvent &pointerEvent) {
    if (allowControl) {
        //printf("Got pointerEvent %d,%d,%d\n", pointerEvent.x, pointerEvent.y, (short) pointerEvent.btnMask);

        //static UInt32 lastEventTicks = 0;

        // From chromiumvncserver.340a5.src, VNCResponderThread.cp

        /*if(msg.btnMask != lastBMask) {
            while((TickCount() - lastEventTicks) < 2)
                ;
            lastEventTicks = TickCount();
        }*/

        Point newMousePosition;
        newMousePosition.h = pointerEvent.x;
        newMousePosition.v = pointerEvent.y;
        LMSetMouseTemp(newMousePosition);
        LMSetRawMouseLocation(newMousePosition);
        LMSetCursorNew(LMGetCrsrCouple());

        // On the Mac Plus, it is necessary to prevent the VBL task from
        // over-writing the button state by keeping MBTicks ahead of Ticks

        #define DELAY_VBL_TASK(seconds) LMSetMBTicks(LMGetTicks() + 60 * seconds);

        if(pointerEvent.btnMask) {
            DELAY_VBL_TASK(10);
        }

        // Use low memory globals to handle clicks
        static unsigned char lastBMask = 0;

        if(lastBMask != pointerEvent.btnMask) {
            lastBMask = pointerEvent.btnMask;

            if(pointerEvent.btnMask) {
                DELAY_VBL_TASK(10);
                LMSetMouseButtonState(0x00);
                PostEvent(mouseDown, 0);
            } else {
                DELAY_VBL_TASK(0);
                LMSetMouseButtonState(0x80);
                PostEvent(mouseUp, 0);
            }
        }
    }
}

void vncClientCutText(const VNCClientCutText &) {
}

void vncFBUpdateRequest(const VNCFBUpdateReq &fbUpdateReq) {
    if(!sendGraphics) return;
    vncDeferUpdateRequest(fbUpdateReq);
}

void vncDoDeferredUpdate(const VNCFBUpdateReq &fbUpdateReq);
void vncDoDeferredUpdate(const VNCFBUpdateReq &fbUpdateReq) {
        fbUpdateRect = fbUpdateReq.rect;
        vncSendFBUpdate(allowIncremental && fbUpdateReq.incremental);
}

// Callback for the VBL task
pascal void vncGotDirtyRect(int x, int y, int w, int h) {
    if(fbUpdateInProgress) {
        printf("Got dirty rect while busy\n");
        return;
    }
    //printf("Got dirty rect");
    if (vncState == VNC_RUNNING) {
        //printf("%d,%d,%d,%d\n",x,y,w,h);
        fbUpdateRect.x = x;
        fbUpdateRect.y = y;
        fbUpdateRect.w = w;
        fbUpdateRect.h = h;
        vncSendFBUpdateColorMap(nil);
    }
}

void vncSendFBUpdate(Boolean incremental) {
    if(incremental) {
        // Ask the VBL task to determine what needs to be updated
        OSErr err = VNCScreenHash::requestDirtyRect(
            fbUpdateRect.x,
            fbUpdateRect.y,
            fbUpdateRect.w,
            fbUpdateRect.h,
            vncGotDirtyRect
        );
        if(err != noErr) {
            printf("Failed to request update %d", err);
            vncError = err;
        }
    } else {
        vncSendFBUpdateColorMap(nil);
    }
}

pascal void vncSendFBUpdateColorMap(TCPiopb *pb) {
	int ret;
    fbUpdateInProgress = true;
    fbUpdatePending = false;
    if(fbColorMapNeedsUpdate) {
        fbColorMapNeedsUpdate = false;

        // Send the header
        #ifdef VNC_FB_BITS_PER_PIX
            const unsigned char fbDepth = VNC_FB_BITS_PER_PIX;
        #endif
        const unsigned int paletteSize = 1 << fbDepth;
        vncServerMessage.fbColorMap.message = mSetCMapEntries;
        vncServerMessage.fbColorMap.padding = 0;
        vncServerMessage.fbColorMap.firstColor = 0;
        vncServerMessage.fbColorMap.numColors = paletteSize;

        auxsendw(vncFD, (Ptr) &vncServerMessage, sizeof(VNCSetColorMapHeader), 0);
        auxsendw(vncFD, (Ptr) VNCFrameBuffer::getPalette(), paletteSize * sizeof(VNCColor), 0);
        vncSendFBUpdateHeader(pb);
    } else {
        vncSendFBUpdateHeader(pb);
    }
}

pascal void vncSendFBUpdateHeader(TCPiopb *pb) {
    fbUpdateInProgress = true;
    fbUpdatePending = false;

    // Make sure x falls on a byte boundary
    unsigned char dx = fbUpdateRect.x & 7;
    fbUpdateRect.x -= dx;
    fbUpdateRect.w += dx;

    // Make sure width is a multiple of 16
    fbUpdateRect.w = (fbUpdateRect.w + 15) & ~15;

    #ifdef VNC_FB_WIDTH
        const unsigned int fbWidth = VNC_FB_WIDTH;
    #endif
    if((fbUpdateRect.x + fbUpdateRect.w) > fbWidth) {
        fbUpdateRect.x = fbWidth - fbUpdateRect.w;
    }

    // Prepare to stream the data
    VNCEncoder::begin();

    // Send the header
    vncServerMessage.fbUpdate.message = mFBUpdate;
    vncServerMessage.fbUpdate.padding = 0;
    vncServerMessage.fbUpdate.numRects = 1;
    vncServerMessage.fbUpdate.rect = fbUpdateRect;
    vncServerMessage.fbUpdate.encodingType = mTRLEEncoding;
    auxsendw(vncFD, (Ptr) &vncServerMessage, sizeof(VNCFBUpdate), 0);

    vncSendFBUpdateRow(pb);
}

pascal void vncSendFBUpdateRow(TCPiopb *pb) {
	int i;
	int ret;
    // Add the termination
    myWDS[1].ptr = 0;
    myWDS[1].length = 0;
    
    // Get a row from the encoder
    const Boolean gotMore = VNCEncoder::getChunk(fbUpdateRect.x, fbUpdateRect.y, fbUpdateRect.w, fbUpdateRect.h, myWDS);
    ret = auxsendw(vncFD, myWDS[0].ptr, myWDS[0].length, 0);
    if (ret < 0 && sockerr() != 32) {
    	printf("send err: %ld\n", sockerr());
    }
    if (gotMore) {
    	vncSendFBUpdateRow(pb);
    } else {
        fbUpdateRect.w = fbUpdateRect.h = 0;
        vncSendFBUpdateFinished(pb);
    }
}

pascal void vncSendFBUpdateFinished(TCPiopb *pb) {
    fbUpdateInProgress = false;
    if(fbUpdatePending) {
        vncSendFBUpdate(true);
    }
}

Boolean queuedFBUpdate = false;
VNCFBUpdateReq deferredRequest;

void vncDeferUpdateRequest(const VNCFBUpdateReq &req) {
	// defer an update to be run after the next event loop
	deferredRequest = req;
	queuedFBUpdate = true;
}

void yieldToVNCIfNecessary() {
	int i;
		
	// Do we have a queued update?
	if (queuedFBUpdate) {
		printf("attempting deferred update\n");
		queuedFBUpdate = false;
		vncDoDeferredUpdate(deferredRequest);
	}
}