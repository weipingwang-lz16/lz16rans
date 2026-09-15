//  Created by weiping wang on 2026/8/22.
//  Copyright © 2026 Amber. All rights reserved.
//
#include "threadpool.h"


#define TIMEOVER_THREAD_INC 0x400
#define TIMEOVER_THREAD_DEC 0x400

#define FAIL_LIMIT_BIT 11
#define FAIL_LIMITCOUNT (1<<FAIL_LIMIT_BIT)

int64_t Getms_Time(void){
    struct timespec t;
    clock_gettime(CLOCK_REALTIME,&t);
    return (t.tv_sec*1000+t.tv_nsec/1000000);
}

void *WorkThreads(void *arg);
void *WorkMonitor(void *arg);

QUEUEFLAG read_Qmonitor(PMYQUEUE q,PMONITORINFO pinfo);
QUEUEFLAG write_Qmonitor(PMYQUEUE q,PMONITORINFO pinfo);
QUEUEFLAG readQmonitor(PMYQUEUE pq,PMONITORINFO pinfo);
QUEUEFLAG writeQmonitor(PMYQUEUE pq,PMONITORINFO pinfo);
QUEUEFLAG read_queue(PMYQUEUE q,PWORKTHREADINFO pinfo);
QUEUEFLAG write_queue(PMYQUEUE q,PWORKTHREADINFO pinfo);

//===============================================================================
//parameter:
//  int t:  sleep time.( usleep(t & 0xfffff);  )
//          line 245,262
QUEUEFLAG readqueue(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo,int t);
QUEUEFLAG writequeue(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo,int t);
//===============================================================================

//===============================================================================
//parameter:
//  int64_t t:  current time,unit is ms.(int64_t Getms_Time(void);  )
//
static inline void inc_workthread_end(PMYTHREADPOOLQUEUE q,int64_t t,pthread_t pid);
//              line:   301
static inline void dec_workthread_end(PMYTHREADPOOLQUEUE q,pthread_t pid,int64_t t);
//              line:   326,420
int inc_workthread(PMYTHREADPOOLQUEUE q,int64_t t);
//              line:   350,364
int dec_workthread(PMYTHREADPOOLQUEUE q,int64_t t);
//              line:   352
int dec_workthread_auto(PMYTHREADPOOLQUEUE q,pthread_t pid,int64_t t);
//              line:   410
//===============================================================================

QUEUEFLAG writ_fail_Qmonitor(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo);

//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ mypthread.h $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//parameter:
//  int ThreadPoolStarnum:  the initial number of threads(2,...,16).
int myqueueInit(PMYTHREADPOOLQUEUE q,int ThreadPoolStarnum,int queuelen,DWORD stateid){
    q->state.Qflag.run=1;
    q->state.stateid=stateid;
    q->state.queueLen=queuelen;
    q->state.ThreadLen=ThreadPoolStarnum;
    q->state.ThreadMinLen=TP_MIN_COUNT;
    q->state.ThreadMaxLen=TP_MAX_COUNT;

    int64_t m=Getms_Time();
    for(int i=0;i<=ThreadPoolStarnum;i++){
        q->state.t[0][i]=m;
        q->state.t[1][i]=0;
    }
    pthread_mutex_init(&(q->state.mux), 0);

    q->Qmonitor.flag.run=1;
    q->Qmonitor.flag.Queueid=ID76BIT_QMONITOR;
    q->Qmonitor.maxlen=(TP_MAX_COUNT+2);

    q->Qread.flag.Queueid=ID76BIT_QREAD;
    q->Qread.maxlen=queuelen;
    q->Qwrite.flag.Queueid=ID76BIT_QWRITE;
    q->Qwrite.maxlen=queuelen;
    q->Qmonitor.buf= malloc((TP_MAX_COUNT+2) * sizeof(MONITORINFO)+3*(queuelen+1) * sizeof(WORKTHREADINFO));
    assert( q->Qmonitor.buf);
    q->Qwrite.infobufEx=(void *)((BYTE *)(q->Qmonitor.buf)+(TP_MAX_COUNT+2) * sizeof(MONITORINFO));
    q->Qread.infobuf=(void *)((BYTE *)(q->Qwrite.infobufEx)+(queuelen+1) * sizeof(WORKTHREADINFO));

    pthread_mutex_init(&(q->Qmonitor.mux), 0);
    pthread_mutex_init(&(q->Qread.mux), 0);
    pthread_mutex_init(&(q->Qwrite.mux), 0);

    pthread_t p[ThreadPoolStarnum+1];
    if(pthread_create(&p[0], NULL, WorkMonitor, (void*)q)){
        return -1;
    }
    pthread_detach(p[0]);

    for(int i = 1; i <= ThreadPoolStarnum; ++i){
        int ret = pthread_create(&p[i], NULL, WorkThreads, (void*)q);
        if(ret){
            return -1;
        }
        pthread_detach(p[i]);
        printf("%2d thread:%16llx created\n", i, (int64_t)p[i]);
    }
    return 0;
}

int myqueuedestroy(PMYTHREADPOOLQUEUE ptpq){
    assert(ptpq);
    ptpq->state.Qflag.run=0;
    for(int j=0;j<ptpq->state.ThreadLen;j++){
        MONITORINFO minfo={0};
        QUEUEFLAG flag = readQmonitor(&(ptpq->Qmonitor),&minfo);
        printf("%2d myqueuedestroy:thread:%16llx{%2x}\n",j, (int64_t)minfo.threadid, flag.flag);
    }
    pthread_mutex_destroy(&(ptpq->Qread.mux));
    pthread_mutex_destroy(&(ptpq->Qwrite.mux));
    ptpq->Qmonitor.flag.run=0;
    sleep(1);
    pthread_mutex_destroy(&(ptpq->Qmonitor.mux));
    pthread_mutex_destroy(&(ptpq->state.mux));
    
    int64_t ms=Getms_Time();
    int rt=(int)(ms-ptpq->state.t[0][0]);
    printf("sum:%8d ms:\n",rt);
    for(int i=1;i<=TP_MAX_COUNT;i++){
        if(ptpq->state.t[1][i]){
            ptpq->state.t[1][i]+=ms-ptpq->state.t[0][i];
            printf("Thread %2d: %8d ms:\n",i,(int)ptpq->state.t[1][i]);
        }
    }
    free(ptpq->Qmonitor.buf);
    return rt;
}

QUEUEFLAG UpdateThreads(PMYTHREADPOOLQUEUE q,bool IsINC){
    MONITORINFO m={0};
    m.Qflag.ID=ID76BIT_APP | (IsINC?0:1);        //manual operation the number of threads: inc or dec.
    return write_Qmonitor(&(q->Qmonitor),&m);
}

QUEUEFLAG readThread_ret(PMYTHREADPOOLQUEUE q,PWORKTHREADINFO pinfo,int t){
    return readqueue(q,&(q->Qread), pinfo,t);
}
QUEUEFLAG writeThread_job(PMYTHREADPOOLQUEUE q,PWORKTHREADINFO pinfo,int t){
    return writequeue(q,&(q->Qwrite), pinfo,t);
}
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ mypthread.h $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

QUEUEFLAG read_Qmonitor(PMYQUEUE q,PMONITORINFO pinfo)
{
    QUEUEFLAG flag={0};
    pthread_mutex_lock(&(q->mux));
    if( q->num>0){
        memcpy(pinfo, &(q->buf[q->head]),sizeof(MONITORINFO));
        q->head= (q->head+1)  % q->maxlen ;
        q->num--;
        flag.flag=1;
    }
    pthread_mutex_unlock(&(q->mux));
    return flag;
}

QUEUEFLAG  write_Qmonitor(PMYQUEUE q,PMONITORINFO pinfo)
{
    QUEUEFLAG flag={0};
    pthread_mutex_lock(&(q->mux));
    if( q->num < q->maxlen ){
        memcpy( &(q->buf[q->tail]),pinfo,sizeof(MONITORINFO));
        q->tail= (q->tail+1) % q->maxlen ;
        q->num++;
        flag.flag=1;
    }
    pthread_mutex_unlock(&(q->mux));
    return flag;
}

QUEUEFLAG readQmonitor(PMYQUEUE pq,PMONITORINFO pinfo){
    QUEUEFLAG flag={0};
    while( 1 ){
        flag = read_Qmonitor(pq, pinfo);
        if( flag.flag ) break;
    }
    return flag;
}

QUEUEFLAG writeQmonitor(PMYQUEUE pq,PMONITORINFO pinfo){
    QUEUEFLAG flag={0};
    while( 1 ){
        flag = write_Qmonitor(pq,pinfo);
        if( flag.flag ) break;
    }
    return flag;
}

QUEUEFLAG read_queue(PMYQUEUE q,PWORKTHREADINFO pinfo)
{
    QUEUEFLAG flag={0};
    pthread_mutex_lock(&(q->mux));
    if( q->num>0 ){
        memcpy(pinfo, &(q->infobufEx[q->head]),sizeof(WORKTHREADINFO));
        q->head= (q->head+1)  % q->maxlen ;
        q->num--;
        flag.flag=1;
        q->RFcount=0;
    }else{
        q->RFcount++;
    }
    pthread_mutex_unlock(&(q->mux));
    if( FAIL_LIMITCOUNT < q->RFcount){
        flag.flag= FLAG_FAIL_READ;
        flag.level= q->RFcount>>FAIL_LIMIT_BIT;
        q->RFcount=0;
    }
    return flag;
}

QUEUEFLAG  write_queue(PMYQUEUE q,PWORKTHREADINFO pinfo)
{
    QUEUEFLAG flag={0};
    pthread_mutex_lock(&(q->mux));
    if( q->num < q->maxlen ){
        memcpy( &(q->infobufEx[q->tail]),pinfo,sizeof(WORKTHREADINFO));
        q->tail= (q->tail+1) % q->maxlen ;
        q->num++;
        flag.flag=1;
        q->WFcount=0;
    }else{
        q->WFcount++;
    }
    pthread_mutex_unlock(&(q->mux));
    if( FAIL_LIMITCOUNT < q->WFcount){
        flag.flag= FLAG_FAIL_WRITE;
        flag.level= q->WFcount>>FAIL_LIMIT_BIT;
        q->WFcount=0;
    }
    return flag;
}

QUEUEFLAG readqueue(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo,int t){
    QUEUEFLAG flag={0};
    while( 1 ){
        flag = read_queue(pq,pinfo);
        if( flag.flag==1 ){
            break;
        }else if( flag.flag==FLAG_FAIL_READ ){
            flag =writ_fail_Qmonitor(q,pq,pinfo);
            flag.flag=FLAG_FAIL_READ;
            if( t & 0xfffff ){
                usleep(t & 0xfffff);
            }
        }
    }
    return flag;
}

QUEUEFLAG writequeue(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo,int t){
    QUEUEFLAG flag={0};
    while( 1 ){
        flag = write_queue(pq,pinfo);
        if( flag.flag==1 ){
            break;
        }else if( flag.flag==FLAG_FAIL_WRITE ){
            flag = writ_fail_Qmonitor(q,pq,pinfo);
            flag.flag=FLAG_FAIL_WRITE;
            if( t & 0xfffff ){
                usleep(t & 0xfffff);
            }
        }
    }
    return flag;
}

//###################################### Update Thread ######################################
static inline void inc_workthread_end(PMYTHREADPOOLQUEUE q,int64_t t,pthread_t pid){
    pthread_mutex_lock(&(q->state.mux));
    q->state.t[1][q->state.ThreadLen]+=t-q->state.t[0][q->state.ThreadLen];
    q->state.ThreadLen++;
    q->state.t[0][q->state.ThreadLen]=t;
    pthread_mutex_unlock(&(q->state.mux));
    printf("%2d  New thread created{%16llx} ok!\n",q->state.ThreadLen,(int64_t)pid);
}

static inline void dec_workthread_end(PMYTHREADPOOLQUEUE q,pthread_t pid,int64_t t){
    pthread_mutex_lock(&(q->state.mux));
    q->state.t[1][q->state.ThreadLen]+=t-q->state.t[0][q->state.ThreadLen];
    q->state.t[0][q->state.ThreadLen]=t;
    q->state.ThreadLen--;
    pthread_mutex_unlock(&(q->state.mux));
    printf("%2d  deleted:%16llx thread: ok!\n",q->state.ThreadLen,(int64_t)pid);
}

int inc_workthread(PMYTHREADPOOLQUEUE q,int64_t t){
    //printf("%2d  inc_workthread: %8x\n",q->state.ThreadLen,t-q->state.time_thread_inc);
    if(q->state.ThreadLen >= q->state.ThreadMaxLen) return 0;
    
    pthread_t p;
    int m=0;
    if( TIMEOVER_THREAD_INC < t-q->state.time_thread_inc){
        if(pthread_create(&p, NULL, WorkThreads, (void*)q)==0 ){
            pthread_detach(p);
            inc_workthread_end(q,t,p);
            q->state.time_thread_inc=t;
            m=1;
        }
    }
    return m;
}

int dec_workthread(PMYTHREADPOOLQUEUE q,int64_t t){
    int m=0;
    if( q->state.ThreadMinLen < q->state.ThreadLen){
        WORKTHREADINFO info={0};
        info.arg.Qflag.ID= (ID76BIT_QMONITOR<<6)| ID50_DEC_THREAD;
        info.arg.Qflag.run=0xff;
        info.arg.ms=t;
        QUEUEFLAG flag=writequeue(q,&(q->Qwrite),&info,0);
        m=1;
    }
    return m;
}

int dec_workthread_auto(PMYTHREADPOOLQUEUE q,pthread_t pid,int64_t t){
    int m=0;
    if( q->state.ThreadMinLen < q->state.ThreadLen){
        if( TIMEOVER_THREAD_DEC < t-q->state.time_thread_dec){
            dec_workthread_end(q,pid,t);
            q->state.time_thread_dec=t;
            m=1;
        }
    }
    return m;
}

QUEUEFLAG writ_fail_Qmonitor(PMYTHREADPOOLQUEUE q,PMYQUEUE pq,PWORKTHREADINFO pinfo){
    QUEUEFLAG flag={0};
    MONITORINFO minfo={0};
    minfo.queueid=pq->flag.Queueid;
    minfo.Qflag.ID=minfo.queueid<<6;
    minfo.Fcount=flag.level;
    flag = write_Qmonitor(&(q->Qmonitor),&minfo);
    flag.ID=flag.flag;
    return flag;
}
//###################################### Update Thread ######################################

QUEUEFLAG APP_Monitor(PMYTHREADPOOLQUEUE q,PMONITORINFO pm,int64_t t){
    QUEUEFLAG flag={0};
    BYTE op=pm->Qflag.ID & 0x3f;
    if( op == ID50_INC_THREAD  ){
        flag.flag=inc_workthread(q,t);
     }else if( op == ID50_DEC_THREAD ){
        flag.flag=dec_workthread(q,t);
     }else if( (pm->Qflag.ID & 0x3f )==ID50_GET_TPSTATE ){    //read: TP_state.
        
    }
    return flag;
}

QUEUEFLAG Qwrite_Monitor(PMYTHREADPOOLQUEUE q,PMONITORINFO pm,int64_t t){
    QUEUEFLAG flag={0};
    if( pm->Qflag.ID>>6 ) {
        if( (q->state.stateid & STATEID_AUTO_BIT)==STATEID_AUTO_BIT ){  //auto
            if( pm->Fcount && pm->Qflag.Queueid==ID76BIT_APP ){         //from APP write to queue busy!!!
                flag.flag=inc_workthread(q,t);
            }
        }
    }
    return flag;
}

void *WorkMonitor(void *arg)
{
    PMYTHREADPOOLQUEUE q=(PMYTHREADPOOLQUEUE)arg;
    while( q->state.Qflag.run ){
        MONITORINFO info={0};
        QUEUEFLAG flag=read_Qmonitor(&(q->Qmonitor),&info);
        if( flag.flag==0 )  continue;
        
        int64_t ms=Getms_Time();
        q->state.t[1][q->state.ThreadLen]+=ms-q->state.t[0][q->state.ThreadLen];
        q->state.t[0][q->state.ThreadLen]=ms;
        
        switch ( (info.Qflag.ID>>6) ) {
            case 0:
                flag=APP_Monitor(q,&info,ms);
                break;
            case 2:
                flag=Qwrite_Monitor(q,&info,ms);
                break;
        }
    }
    return NULL;
}
//###################################### Monitor Thread ######################################

void *WorkThreads(void *arg)
{
    int exitid=1;
    pthread_t pid=pthread_self();
    PMYTHREADPOOLQUEUE q=(PMYTHREADPOOLQUEUE)arg;
    while(q->state.Qflag.run ){
        WORKTHREADINFO info={0};
        PWORKTHREADINFO pinfo=&info;
        QUEUEFLAG flag=read_queue(&(q->Qwrite),pinfo);
        if( flag.flag==0 ){
            continue;
        }else if( flag.flag==FLAG_FAIL_READ ){
            if( (q->state.stateid & STATEID_AUTO_BIT)==STATEID_AUTO_BIT ){
                int64_t ms=Getms_Time();
                if( dec_workthread_auto(q,pid,ms) ){        //read from queue idle? is Thread exit.
                    exitid=0;
                    break;
                }
            }
            continue;
        }
        
        if( info.arg.Qflag.ID == ((ID76BIT_QMONITOR<<6)| ID50_DEC_THREAD ) ){
            //from APP: UpdateThreads( *** ,flase);     Thread exit!!!
            dec_workthread_end(q,pid,info.arg.ms);
            exitid=0;
            break;
        }
        //#################### run worker ####################
        if(pinfo->func){
            pinfo->func(&(pinfo->arg));
        }
        //#################### run worker ####################
        while( (q->state.stateid & STATEID_QREAD_BIT)==STATEID_QREAD_BIT )
        {
            QUEUEFLAG flag=write_queue(&(q->Qread),pinfo);
            if( flag.flag ){
                if( flag.flag==FLAG_FAIL_WRITE ){
                    flag = writ_fail_Qmonitor(q,&(q->Qread),pinfo);
                    flag.flag=FLAG_FAIL_WRITE;
             }
                break;
            }
        }
    }
    if( exitid ){
        MONITORINFO minfo={0};
        minfo.threadid=pid;
        writeQmonitor(&(q->Qmonitor),&minfo);
    }
    return NULL;
}
