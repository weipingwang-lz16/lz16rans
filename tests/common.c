//
//  mycommon.c
//  mycompress
//
//  Created by weiping wang on 2026/8/24.
//
#include "common.h"

int myGetDataRandom(BYTE *pbuf,int maxlen){
    static DWORD mask[4]={0,0xff,0xffff,0xfffff};
    //int len=arc4random_uniform(max);
    int len=rand() % maxlen + 1;
    uint32_t *pdw=(uint32_t *)pbuf;
    int i=0;
    do{
        *pdw++=rand();
        i+=4;
        //printf("%8d[%8x]\n",i,pdw[i]);
        int recurlen=rand();
        if( (4< recurlen && recurlen <= 123 ) && recurlen<i){
            memcpy(pdw,pbuf-recurlen,recurlen);
            i+=recurlen;
        }
   }while(i<len/4);
    pdw[i/4]=rand() & mask[len & 0x3];
    //printf("%8d[%8x]  %8x\n",i,pdw[i],mark[len & 0x3]);
    return len;
}
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@??? Random ???@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void GetfixedlenStr(char *s,char *f,int len){
    memcpy(s,f,len);
    int n=(int)strlen(f);
    int i=1;
    s[n]=0x3a;
    for(;i<len-n;i++) s[n+i]=0x20;
    s[n+i]=0;
}

int64_t myGettimes(void){
    struct timespec t;
    clock_gettime(CLOCK_REALTIME,&t);
    int64_t tt=(int64_t)t.tv_sec * 1000000 + t.tv_nsec / 1000;
    return tt;
}

void myEncodeExit(double *penlen,int enlen,double *psum,int64_t cur_t){
    int64_t t=myGettimes();
    *psum+=(int)(t-cur_t);
    *penlen=(double)enlen;
    if( enlen<=0 )  printf("compress error!!!(ret: %6d) \n",enlen);
}

void myDecodeExit(double *pdelen,int delen,double *psum,int64_t cur_t){
    int64_t t=myGettimes();
    *psum+=(int)(t-cur_t);
    *pdelen=(double)delen;
    if(delen<=0 )  printf("uncompress error!!!(ret: %6d) \n",delen);
}

size_t myGetfilelen(char *fname){
    struct stat file_stat;
    if (stat(fname, &file_stat) == 0) {
        //printf("文件大小: %ld 字节\n", file_stat.st_size);
        return file_stat.st_size;
    } else {
        perror("无法获取文件信息\n");
        return -1;
    }
}

long myreadwritefile(char *fname, void *pbuffer,int inlen,int Isread){
    FILE *f;
    long len=0;
    if(Isread){
        len=myGetfilelen(fname);
        if( len<0 ) return len;
        f = fopen(fname,"rb");
    }else{
        //f = fopen(fname,"wb");
        f = fopen(fname,"ab+");
    }
    if ( f==NULL ){
        return 0;
    }
    if(Isread){
        fseek(f, 0, SEEK_SET);
        fread(pbuffer,1,len,f);
    }else{
        fseek(f, 0, SEEK_END);
        len=fwrite(pbuffer,1,inlen,f);
    }
    fclose(f);
    return len;
}

void printlz16(const char *strs[],int n){
    for(int i=0;i<n;i++) printf("%s\n",strs[i]);
}

void print_nnEx(char *prompt, unsigned int *a, int len) {
        char msg[4096];
        int i, point = 0;
        for (i = 0; i < len; i++) {
            sprintf(msg + point, "%08X-", a[i]);
            point += sizeof(unsigned int) * 2 + 1;
        }
        sprintf(msg + point, "%08X", a[i]);
        printf("%s %s",prompt,msg);
}

void myprintCheckinfo(WORD *ptag,WORD *psrc,int len){
    for(int i=0;i<len;i++){
        if(psrc[i] !=ptag[i]){
            printf("16bit: Error!!! %8d(%8xh)] [%4x,%4x]\n",i,i,psrc[i],ptag[i]);
            print_nnEx(" \n", (DWORD *)(psrc+i-4 ), 7);
            print_nnEx(" \n", (DWORD *)(ptag+i-4 ), 7);
            break;
        }
    }
    if (memcmp(psrc,ptag,len) != 0)
        printf("ERROR: bad decoder!\n");
    //else
        //printf("decode ok!\n");
}

int  myGetFilesrcbuf(char * fname,void ** pps)
{
    int filelen=(int)myGetfilelen((char *)fname);
    int len=filelen<1024?512:filelen;

    BYTE *ps=(BYTE *)calloc(len,3);
    //rest the lenght of compress out buffer that the lenght of in buffer
    if( ps == NULL ) return -1;
    int n =(int)myreadwritefile((char *)fname, ps,0,1);
    if( filelen==n || (len==512  && filelen<=512 ) ) {
        *pps=ps;
        return n;
    }
    free(ps);
    return -2;
}
