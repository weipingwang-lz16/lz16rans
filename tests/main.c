//
//  main.c
//
//  Created by weiping wang on 2026/8/23.
//
#include "common.h"
#include "../lib/TPdynamic/threadpool.h"

#define LIMIT_LEN  8 //from: lz16.h


void mylz16ranstest(const char *fname,int level);
void mylz16Randomtest(int maxlen);
void myComparetest(char *path,int count,int compress,int *plevel);
void mythreadpooltest(char *fname,int inlevel);

const char * ShowText[] ={   \
    "  argv[1]    argv[2]    argv[3]     argv[4]",   \
    "  command    level     filename      Other          Remark",               \
    "  -c          in       -S|-M     lz4|zstd|lzfse     Compare: lz16rans & (lz4|zstd|lzfse) ",  \
    "  -z          in        in                          test file",        \
    "  -t          in        in                          test Thread pool",                   \
    "  -r         maxlen                                 test random data"};

const char * example[] ={   "mylz16 -c 1 -S  lz4",\
                            "mylz16 -z 2 ../data/other/book1",\
                            "mylz16 -t 3 ../data/other/book1",  \
                            "mylz16 -r 16"};

int main(int argc, const char * argv[]) {
    if(argc<=2 ){
        printlz16(ShowText,6);
        exit(0);
    }
    
    const char *command = argv[1];
    int l = 0;
    if(argv[2]) l=atoi(argv[2]);
    int level[2]={l,l};
    const char * filename = argv[3];

    if( strcmp(command, "-z") == 0 && argc==4 && argv[2] && filename){
        mylz16ranstest(filename,level[0]);
        return 0;
    }else if( strcmp(command, "-c") == 0 && argc==5 && argv[2] && filename){
        int othercompress=3;
        if(strcmp(argv[4], "lzfse") == 0 ){
            othercompress=1;
        }else if(strcmp(argv[4], "zstd") == 0 ){
            othercompress=2;
        }
        if(strcmp(filename, "-M") == 0){
            myComparetest("../data/other",10,othercompress,level);
        }else{
            myComparetest("../data/silesia",10,othercompress,level);
        }
        return 0;
    }else if( strcmp(command, "-t") == 0 && argc==4  && argv[2] && filename){
        mythreadpooltest((char *)filename,level[0]);
        return 0;
    }else if( strcmp(command, "-r") == 0 && argc==3  && argv[2]){
        mylz16Randomtest(level[0]);
        return 0;
    }
    printlz16(example,4);
    return 0;
}

extern int lz16ENhashmasm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
extern int lz16ENhashmcasm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);

extern int lz16DEmasm(BYTE * const pout,BYTE * const pin,int outlen) asm("_Lz16DEmcasm");
extern int lz16DEmcasm(BYTE * const pout,BYTE * const pin,int outlen)asm ("_Lz16DEmcasm");

void mylz16ranstest(const char *fname,int level){
    char fnameEx[128]={0};
    sprintf(fnameEx, "%s",fname);

    int slen=1,clen=1,dlen=1;
    int64_t et,dt;
    
    BYTE *psrc=NULL;
    slen=myGetFilesrcbuf(fnameEx,(void **)&psrc);
    if( slen<0 ){
        printf("error !!!\n");
        return;
    }
    
    BYTE *ptemp=psrc+slen;
    BYTE *pdecodebuf=(BYTE *)calloc(slen, 1);
    assert(pdecodebuf);

    et=myGettimes();
    clen=mylz16Encode(&ptemp,(BYTE *)psrc,2*slen,slen,level);
    et=myGettimes()-et;
    
    dt=myGettimes();
    dlen=mylz16Decode((BYTE *)pdecodebuf,(BYTE *)ptemp,slen);
    dt=myGettimes()-dt;
    
    printf("%s: (%8d,%8d)\n",fnameEx,clen,dlen);

    GetfixedlenStr(fnameEx,(char *)fname,64);
    printf("%s",fnameEx);
    printf("[%9d]{%7.4f et:%6.1f,dt:%6.1f}\n",slen,slen/(float)clen,(float)et/1000.0,(float)dt/1000.0);
    myprintCheckinfo((WORD *)pdecodebuf,(WORD *)psrc,slen/2);
    free(pdecodebuf);
    free(psrc);
}

void mylz16Randomtest(int maxlen){
    int len=maxlen<1024?512:maxlen;
    BYTE *ps=(BYTE *)malloc(4*len);
    assert(ps);

    BYTE *pdst=ps+len;
    BYTE *pdecode=ps+3*len;

    srand( (unsigned)time(0) );
    for(int j=1; j<=4;j++){
        int level=j;
        int slen=myGetDataRandom(ps,maxlen);
        if( slen <= LIMIT_LEN ){
            printf("srclen: %2d <= LIMIT_LEN (error!)\n",slen);
            continue;
        }
        BYTE *ptemp=pdst;
        int encodelen=mylz16Encode(&ptemp,ps,2*len,slen,level);
        int decodelen=mylz16Decode(pdecode,ptemp,slen);
        printf("%8d{%2.4f}(encodelen: %8d, decodelen: %8d), level:%1d\n",slen,(float)slen/encodelen,encodelen,decodelen,level);
        myprintCheckinfo((WORD *)pdecode,(WORD *)ps,slen/2);
    }
    free(ps);
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Compare @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
void lzCompare(char *path,char *fname,int compress,int *plevel,int count,double *pme,double *pother){
    double me[4]={0},other[4]={0};
    char fnameEx[128]={0};
    sprintf(fnameEx, "%s/%s",path,fname);

    int slen=1,clen=1,dlen=1;
    int64_t cur_t1,cur_t2;
    
    BYTE *psrc=NULL ,*pdst=NULL;
    slen=myGetFilesrcbuf(fnameEx,(void **)&psrc);
    if( slen<0 ){
        printf("error !!!\n");
        return;
    }
    pdst=psrc+slen;
    BYTE *pdecodebuf=(BYTE *)calloc(slen, 1);
    assert(pdecodebuf);
    
    void *Enaux = malloc(lzfse_encode_scratch_size());
    void *Deaux = malloc(lzfse_decode_scratch_size());
    
    pme[4]+=(double)slen;
    
    for(int j=0;j<count;j++){
        BYTE *ptemp=pdst;
        BYTE *pdetemp=pdecodebuf;
        
        if(compress ==1 ){
            ptemp=pdst;
            cur_t1=myGettimes();
            clen=(int)lzfse_encode_buffer((uint8_t *)ptemp,2*slen,(uint8_t *)psrc,slen,Enaux);
            myEncodeExit(&other[0],clen,&other[2],cur_t1);

            pdetemp=pdecodebuf;
            cur_t2=myGettimes();
            dlen = (int)lzfse_decode_buffer(pdetemp,slen,ptemp,clen, Deaux);
            myDecodeExit(&other[1],dlen,&other[3],cur_t2);
        }else if(compress == 2 ){
            ptemp=pdst;
            cur_t1=myGettimes();
            clen=(int)ZSTD_compress((uint8_t *)ptemp,2*slen,(uint8_t *)psrc,slen,plevel[0]);    //default:  3
            myEncodeExit(&other[0],clen,&other[2],cur_t1);

            pdetemp=pdecodebuf;
            cur_t2=myGettimes();
            dlen = (int)ZSTD_decompress(pdetemp,slen,ptemp,clen);
            myDecodeExit(&other[1],dlen,&other[3],cur_t2);
        }else{
            ptemp=pdst;
            cur_t1=myGettimes();
            clen = LZ4_compress_default((char *)psrc,(char *)ptemp, slen,2*slen);
            myEncodeExit(&other[0],clen,&other[2],cur_t1);

            pdetemp=pdecodebuf;
            cur_t2=myGettimes();
            dlen=LZ4_decompress_safe((char *)ptemp,(char *)pdetemp,clen,slen);
            myDecodeExit(&other[1],dlen,&other[3],cur_t2);
       }
        ptemp=pdst;
        cur_t1=myGettimes();
        clen=mylz16Encode(&ptemp,(BYTE *)psrc,2*slen,slen,plevel[0]);
        myEncodeExit(&me[0],clen,&me[2],cur_t1);
        
        pdetemp=pdecodebuf;
        cur_t2=myGettimes();
        dlen=mylz16Decode((BYTE *)pdetemp,(BYTE *)ptemp,slen);
        myDecodeExit(&me[1],dlen,&me[3],cur_t2);
    }
    myprintCheckinfo((WORD *)pdecodebuf,(WORD *)psrc,slen/2);

    GetfixedlenStr(fnameEx,fname,16);
    printf("%s",fnameEx);
    printf("[%9d]{%7.4f et:%6.1f,dt:%6.1f} ",slen,slen/(float)me[0],(float)me[2]/1000.0/count,(float)me[3]/1000.0/count);
    printf(" {%7.4f et:%6.1f,dt:%6.1f} \n",slen/(float)other[0],(float)other[2]/1000.0/count,(float)other[3]/1000.0/count);

    pme[0]+=(float)me[0];
    pme[2]+=(float)me[2]/1000.0/count;
    pme[3]+=(float)me[3]/1000.0/count;
    pother[0]+=(float)other[0];
    pother[2]+=(float)other[2]/1000.0/count;
    pother[3]+=(float)other[3]/1000.0/count;
    free(pdecodebuf);
    free(psrc);
}
void myComparetest(char *path,int count,int compress,int *plevel){
    double me[5]={0},other[5]={0};
    int filenum=0;
    
    DIR *dir = opendir(path);
    if (dir == NULL) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue;
        if(entry->d_type == DT_REG ){
            filenum++;
            lzCompare(path,entry->d_name,compress,plevel,count,me,other);
        }
    }

    printf("lz16(%2d):   [sl:%10d]",plevel[0],(int)me[4]);
    printf("{%7.4f et:%6.1f,dt:%6.1f} ",me[4]/me[0],me[2]/filenum,me[3]/filenum);
        
    printf(" {%7.4f et:%6.1f,dt:%6.1f}",me[4]/other[0],other[2]/filenum,other[3]/filenum);
    if(compress ==1 ){          //lzfse
        printf(" lzfse\n");
    }else if(compress == 2 ){   //zstd
        printf(" zstd(%2d)\n",plevel[1]);
    }else{                      //lz4
        printf(" lz4\n");
    }
}
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Compare @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


//######################################### thread pool //#########################################
//$$$$$$$$$$$$$$$$$$$$ thread pool  worker $$$$$$$$$$$$$$$$$$$$
void worker(void *arg)
{
    PARGUMENTINFO pinfo=(PARGUMENTINFO) arg;
    int index=pinfo->error;
    pinfo->error=0;
    int len=0;
    if( pinfo->Qflag.IsEncode ){
        len=mylz16Encode(&(pinfo->pout),pinfo->pin,pinfo->outlen,pinfo->inlen,pinfo->Qflag.level);
    }else{
        len=mylz16Decode(pinfo->pout,pinfo->pin,pinfo->outlen);
    }
    //printf("%6d work: Encode{il:%8d,ol:%8d,flag:%8x} %8d\n",index,pinfo->inlen,pinfo->outlen,pinfo->Qflag,len);
    
    if( len<=0 ){
        pinfo->error=len;
    }else{
        pinfo->inlen=index;
        pinfo->outlen=len;
    }
}
//$$$$$$$$$$$$$$$$$$$$ thread pool  worker $$$$$$$$$$$$$$$$$$$$
QUEUEFLAG  mySetWorkThread(PMYTHREADPOOLQUEUE ptpq,BYTE *ps,int inlen,int level){
    WORKTHREADINFO info={0};
    info.arg.inlen=inlen;
    info.arg.outlen=2*inlen;
    info.arg.pin=(BYTE *)calloc(inlen,3);
    memcpy(info.arg.pin,ps,inlen);
    info.arg.pout=info.arg.pin+inlen;
    info.arg.Qflag.IsEncode=1;
    info.arg.Qflag.level= level;
    info.func=(thread_func)&worker;
    QUEUEFLAG flag=writeThread_job(ptpq,&info,1);
    return flag;
}
//Get the result of WordThread out(Qread.)
QUEUEFLAG myGetWordThread(PMYTHREADPOOLQUEUE ptpq,BYTE *psrc,BYTE *pdecodebuf,int srclen){
    WORKTHREADINFO info={0};
    QUEUEFLAG flag=readThread_ret(ptpq,&info,0);
    if( flag.flag==1 ){
        memset(pdecodebuf,0,srclen);
        int len=mylz16Decode(pdecodebuf,info.arg.pout,srclen);
        printf("(%8d,%3.4f), outlen:%8d\n",info.arg.outlen,(float)srclen/info.arg.outlen,len);
        myprintCheckinfo((WORD *)pdecodebuf,(WORD *)psrc,srclen/2);
        free(info.arg.pin);
    }
    return flag;
}

void mythreadpooltest(char *fname,int inlevel){
    MYTHREADPOOLQUEUE tpq={0};
    DWORD stateid=STATEID_AUTO_BIT | STATEID_QREAD_BIT;
    //STATEID_AUTO_BIT: automate update the number of threads(inc or dec).
    //STATEID_QREAD_BIT: out result to Qread.
    myqueueInit(&tpq,4,3,stateid);
    
    BYTE *psrc=NULL ,*pdst=NULL;
    int srclen=myGetFilesrcbuf(fname,(void **)&psrc);
    if( srclen<0 ){
        printf("error !!!\n");
        return;
    }
    pdst=psrc+srclen;
        
    const int m=300;
    int i=0,j=0;
    QUEUEFLAG flag={0};
    while( 1 ){
        int r=rand();
        int level= inlevel<=0 ? (r % 5) : inlevel;
        if( i<m ){
            flag=mySetWorkThread(&tpq,psrc,srclen,level);
            if( flag.flag==1 ){
                i++;
            }
            //++++++++++++++++++++++++++ Simulate:  manual operation the number of threads ++++++++++++++++++++++++++
            if( i == (r & 0xff) || i == (r & 0xf)){
                flag=UpdateThreads(&tpq,(r & 1));   //1: inc, 0: dec.
                printf("APP to Monitor Thread inc(%2d)\n",(r & 1));
            }
            //++++++++++++++++++++++++++ Simulate:  manual operation the number of threads ++++++++++++++++++++++++++
            if( i>=m && ((tpq.state.stateid & STATEID_QREAD_BIT))!=STATEID_QREAD_BIT) break;
        }
        
        if( (tpq.state.stateid & STATEID_QREAD_BIT)==STATEID_QREAD_BIT ){
            flag=myGetWordThread(&tpq,psrc,pdst,srclen);
            if( flag.flag==1 ){
                j++;
                if( j>=m ) break;
            }else if(flag.flag==FLAG_FAIL_READ){
                
            }
        }
    }
    int st=1;
    sleep(st);
    free(psrc);
    int time=myqueuedestroy(&tpq)-st*1000;
    printf("num:%8d   time:%8d ms(%6.1f ms)\n",m,time,time/(float)m);
}
//######################################### thread pool //#########################################
