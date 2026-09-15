//
//  mycommon.h
//  mycompress
//
//  Created by weiping wang on 2026/8/24.
//

#ifndef mycommon_h
#define mycommon_h

#if defined(ENV_MACOSX)      //mac osx X64
    #include <malloc/malloc.h>
    #include <inttypes.h>
    #include <unistd.h>
#endif

#ifdef MYBUILD_X64
    #include <stdint.h>
    #include <windows.h>
#endif

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

typedef uint64_t QWORD;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

int myGetDataRandom(BYTE *pbuf,int maxlen);
void GetfixedlenStr(char *s,char *f,int len);
int64_t myGettimes(void);

void myEncodeExit(double *penlen,int len,double *psum,int64_t cur_t);
void myDecodeExit(double *pdelen,int delen,double *psum,int64_t cur_t);

size_t myGetfilelen(char *fname);
long myreadwritefile(char *fname, void *pbuffer,int inlen,int Isread);

int GetRandomData(BYTE *pbuf,int max);

void printlz16(const char *strs[],int n);
void print_nnEx(char *prompt, unsigned int *a, int len);
void myprintCheckinfo(WORD *ptag,WORD *psrc,int len);

int  myGetFilesrcbuf(char * fname,void ** pbuf);

extern int mylz16Encode(uint8_t** pout,const uint8_t* pin,const int outlen,const int inlen,int level);
extern int mylz16Decode(uint8_t* pout,uint8_t * pin,const int outlen);

extern size_t lzfse_encode_scratch_size(void);
extern size_t lzfse_decode_scratch_size(void);
extern size_t lzfse_encode_buffer(uint8_t *__restrict dst_buffer, size_t dst_size,
                           const uint8_t *__restrict src_buffer,
                           size_t src_size, void *__restrict scratch_buffer);
extern size_t lzfse_decode_buffer(uint8_t *__restrict dst_buffer, size_t dst_size,
                           const uint8_t *__restrict src_buffer,
                           size_t src_size, void *__restrict scratch_buffer);

extern int LZ4_compress_default(const char* source, char* dest, int inputSize, int maxOutputSize);
extern int LZ4_decompress_safe(const char* source, char* dest, int compressedSize, int maxDecompressedSize);
extern size_t ZSTD_compress( void* dst, size_t dstCapacity,const void* src, size_t srcSize,int Level);
extern size_t ZSTD_decompress( void* dst, size_t dstCapacity, const void* src, size_t compressedSize);

extern void mythreadpoolqueuetest(char *fname,int level);
#endif /* mycommon_h */
