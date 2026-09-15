//
//  threadpool.h
//
//  Created by weiping wang on 2026/7/22.
//  Copyright © 2026 Amber. All rights reserved.
//

#ifndef threadpool_h
#define threadpool_h

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

typedef uint64_t QWORD;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

#define TP_MIN_COUNT 2
#define TP_MAX_COUNT 16

#define STATEID_AUTO_BIT        0x8000          //WorkThreads: automate inc or dec.
#define STATEID_QREAD_BIT       0x4000          //out result to Qread.

#define FLAG_FUNC_NUM_MASK          0x0f

#define ID76BIT_APP                 0
#define ID76BIT_QREAD               1
#define ID76BIT_QWRITE              2
#define ID76BIT_QMONITOR            3

#define ID50_INC_THREAD             0x0
#define ID50_DEC_THREAD             0x1
#define ID50_GET_TPSTATE            0x2

#define ID50_READ_OVER              0x20
#define ID50_WRITE_OVER             0x21

#define FLAG_FAIL_READ 0x10
#define FLAG_FAIL_WRITE 0x11

//+++++++++++++++++++++++++++++++++++++++++++  struct ++++++++++++++++++++++++++++++++++
typedef struct _QUEUEFLAG{
    BYTE flag;
    BYTE ID;
    union{
        BYTE IsEncode;
        BYTE run;
    };
    union{
        BYTE Queueid;
        BYTE level;
    };
}QUEUEFLAG,* PQUEUEFLAG;

typedef struct _ARGUMENTINFO {
    BYTE *pout;
    BYTE *pin;
    QUEUEFLAG Qflag;
    union{
        int64_t ms;
        struct {
            int inlen;
            int outlen;
        };
    };
    DWORD error;
}ARGUMENTINFO, * PARGUMENTINFO;

typedef struct _MONITORINFO {
    pthread_t threadid;
    QUEUEFLAG Qflag;
    DWORD error;
    WORD Fcount;       //the Count of fail
    BYTE queueid;
    BYTE curQueuelen;
}MONITORINFO, * PMONITORINFO;

//+++++++++++++++++++++++++++++ 0813 ++++++++++++++++++++++++++++
typedef void (*thread_func)(void *arg);

typedef struct _WORKTHREADINFO{
    thread_func      func;
    ARGUMENTINFO   arg;
}WORKTHREADINFO,* PWORKTHREADINFO;
//+++++++++++++++++++++++++++++ 0813 ++++++++++++++++++++++++++++

typedef struct threadpool_work _WORKTHREADINFO;

typedef struct _MYQUEUE{
    pthread_mutex_t mux;
    union{
        PWORKTHREADINFO infobufEx;
        PARGUMENTINFO infobuf;
        PMONITORINFO  buf;
    };
    QUEUEFLAG flag;
    int head;
    int tail;
    int maxlen;
    int num;
    int WFcount;       //the Count of write fail
    int RFcount;       //the Count of read fail
}MYQUEUE,*PMYQUEUE;

typedef struct _MYTHREADPOOLSTATE{;
    pthread_mutex_t mux;
    QUEUEFLAG Qflag;
    DWORD stateid;
    int ThreadLen;
    int ThreadMinLen;
    int ThreadMaxLen;
    int queueLen;
    int64_t time_thread_inc;
    int64_t time_thread_dec;
    int64_t t[2][TP_MAX_COUNT+1];
}MYTHREADPOOLSTATE,*PMYTHREADPOOLSTATE;

typedef struct _MYTHREADPOOLQUEUE{
    MYQUEUE Qread;
    MYQUEUE Qwrite;
    MYQUEUE Qmonitor;
    MYTHREADPOOLSTATE state;
}MYTHREADPOOLQUEUE,*PMYTHREADPOOLQUEUE;
//+++++++++++++++++++++++++++++++++++++++++++ struct ++++++++++++++++++++++++++++++++++
//parameter:
//  int ThreadPoolStarnum:  the initial number of threads(2,...,16).
int myqueueInit(PMYTHREADPOOLQUEUE q,int ThreadPoolStarnum,int queuelen,DWORD stateid);
int myqueuedestroy(PMYTHREADPOOLQUEUE ptpq);

QUEUEFLAG UpdateThreads(PMYTHREADPOOLQUEUE q,bool IsINC);
//==========================================================================
//parameter:
//  int t:  sleep time.( usleep(t & 0xfffff);  )
QUEUEFLAG readThread_ret(PMYTHREADPOOLQUEUE q,PWORKTHREADINFO pinfo,int t);
QUEUEFLAG writeThread_job(PMYTHREADPOOLQUEUE q,PWORKTHREADINFO pinfo,int t);
//==========================================================================
#endif /* threadpool_h */
