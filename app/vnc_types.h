#ifndef VNC_TYPES_H
#define VNC_TYPES_H


enum {
    mSetPixelFormat   = 0,
    mSetEncodings     = 2,
    mFBUpdateRequest  = 3,
    mKeyEvent         = 4,
    mPointerEvent     = 5,
    mClientCutText    = 6
};

enum {
    mFBUpdate         = 0,
    mSetCMapEntries   = 1,
    mBell             = 2,
    mServerCutText    = 3
};

enum {
    mRawEncoding      = 0,
    mTRLEEncoding     = 15
};

typedef struct VNCRect {
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
} VNCRect;

typedef struct VNCPixelFormat {
    unsigned char  bitsPerPixel;
    unsigned char  depth;
    unsigned char  bigEndian;
    unsigned char  trueColour;
    unsigned short redMax;
    unsigned short greenMax;
    unsigned short blueMax;
    unsigned char redShift;
    unsigned char greenShift;
    unsigned char blueShift;
    char padding[3];
} VNCPixelFormat;

#define VNC_SERVER_INIT_NAME_LEN 10

typedef struct VNCServerInit {
    unsigned short fbWidth;
    unsigned short fbHeight;
    VNCPixelFormat format;
    unsigned long  nameLength;
    char name[VNC_SERVER_INIT_NAME_LEN];
} VNCServerInit;

typedef struct VNCSetPixFormat {
    unsigned char message;
    unsigned char padding[3];
    VNCPixelFormat format;
} VNCSetPixFormat;

typedef struct VNCSetEncoding {
    unsigned char message;
    unsigned char padding;
    unsigned short numberOfEncodings;
} VNCSetEncoding;

typedef struct VNCSetEncodingOne {
    unsigned char message;
    unsigned char padding;
    unsigned short numberOfEncodings;
    unsigned long encoding;
} VNCSetEncodingOne;

typedef struct VNCColour {
    unsigned short red;
    unsigned short green;
    unsigned short blue;
} VNCColour;

typedef struct VNCSetColourMapHeader {
    unsigned char  message;
    unsigned char  padding;
    unsigned short firstColour;
    unsigned short numColours;
} VNCSetColourMapHeader;

typedef struct VNCFBUpdateReq{
    unsigned char  message;
    unsigned char  incremental;
    VNCRect        rect;
} VNCFBUpdateReq;

typedef struct VNCFBUpdate {
    unsigned char  message;
    unsigned char  padding;
    unsigned short numRects;
    VNCRect        rect;
    long           encodingType;
} VNCFBUpdate;

typedef struct VNCKeyEvent {
    unsigned char  message;
    unsigned char down;
    unsigned short padding;
    unsigned long key;
} VNCKeyEvent;

typedef struct VNCPointerEvent {
    unsigned char message;
    unsigned char btnMask;
    unsigned short x;
    unsigned short y;
} VNCPointerEvent;

typedef struct VNCClientCutText {
    unsigned char message;
    unsigned char padding[3];
    unsigned long length;
} VNCClientCutText;

typedef union VNCClientMessages {
    char               message;
    VNCSetPixFormat    pixFormat;
    VNCSetEncoding     setEncoding;
    VNCSetEncodingOne  setEncodingOne;
    VNCFBUpdateReq     fbUpdateReq;
    VNCKeyEvent        keyEvent;
    VNCPointerEvent    pointerEvent;
    VNCClientCutText   cutText;
} VNCClientMessages;

typedef union VNCServerMessages {
    VNCFBUpdate           fbUpdate;
    VNCSetColourMapHeader  fbColourMap;
} VNCServerMessages;

#endif