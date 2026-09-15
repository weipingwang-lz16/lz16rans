//
//  lz16.h
//  SWBApp
//
//  Created by weiping wang on 2026/6/6.
//  Copyright © 2026 Amber. All rights reserved.
//

#ifndef lz16_h
#define lz16_h

#if defined(ENV_MACOSX)      //mac osx X64
    #include <malloc/malloc.h>
    #include <inttypes.h>
    #include <unistd.h>
#endif

#ifdef MYBUILD_X64
    #include <stdint.h>
    #include <windows.h>
    #include <windef.h>
#endif

#include <stdio.h>          //printf
#include <stdlib.h>         //malloc,calloc,free
#include <string.h>         //memcpy,memset
#include <assert.h>

typedef uint64_t QWORD;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

#define _MYIOHEAD \
    union{                                  \
        QWORD sumbytes;                     \
        struct{                             \
            unsigned int  myTotalbytes;     \
            unsigned int  len;              \
        };                                  \
    };                                      \
    union{                                  \
        unsigned int myflag;                \
        struct{                             \
            WORD    flag;                   \
            WORD    flagEx;                 \
        };                                  \
    };                                      \
    WORD    id;                             \
    BYTE    items;                          \
    BYTE    offset

#define _MYNET_ID \
union { \
union {                     \
    QWORD myServerID;       \
    struct {                \
        DWORD ServerID;     \
        DWORD ServerIDex;   \
    };                      \
};  \
union {                     \
    QWORD netmeID;          \
    struct {                \
        DWORD meID;         \
        DWORD meIDex;       \
    };                      \
};  \
union {                     \
    DWORD myBitmapID;       \
    union { \
        DWORD mysubflag;    \
        struct {            \
            WORD subindex;  \
            WORD subcount;  \
        };                  \
    };                      \
};\
};

typedef struct {
    _MYIOHEAD;
} MYIOHEAD, *PMYIOHEAD;

#define _MYIOHEADEX         \
        _MYIOHEAD;          \
        _MYNET_ID
 
 typedef struct {
     _MYIOHEADEX;
 } MYIOHEADEX, * PMYIOHEADEX;

#define TRUE 1
#define FALSE 0
//#############################Compress flag ####################################
#define COMPRESS_MYLZ16         0x1116
#define COMPRESS_RANS_64_02     0x1811
//#############################Compress flag ####################################
#define LIMIT_LEN           8
#define MYREST_LEN          8
#define MYSECTION_COUNT     5
#define MYRINGQUEUE_COUNT   4
#define HASH4_BIT           20
#define MYMATCHOFFSET_MAX   (1<<(16+4))
#define SECTION_OVER       -10000

typedef struct{
    DWORD dis[MYRINGQUEUE_COUNT];
}MYMATCHLISTCDL,*PMYMATCHLISTCDL;

typedef struct {
    BYTE * pbuf;
    int len;
    int offset;
} MYLZITEM,*PMYLZITEM;

typedef struct{
    int inlen;
    int outlen;
    int len[MYSECTION_COUNT];
    union{
        DWORD flagEx;
        struct{
            WORD    flag;
            WORD    flagsub;
        };
    };
    struct {
        WORD id;
        BYTE index;
        BYTE items;
    };
} MYCOMPRESSITEM, *PMYCOMPRESSITEM;

typedef struct {
    const WORD * pinEnd;
    const WORD * pbase;
    PMYLZITEM pmylzitems;
    PMYCOMPRESSITEM pitem;
    DWORD * pmyht;
    PMYMATCHLISTCDL pcdll;
}MYPUBLICVAL,*PMYPUBLICVAL;


int lz16ENhashm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashDm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashDmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);

int lz16DEm(const BYTE *pout,const BYTE *pin,const int outlen);
int lz16DEmc(const BYTE *pout,const BYTE *pin,const int outlen);

#endif /* lz16_h */
