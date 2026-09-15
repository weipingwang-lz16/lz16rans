//
//  lz16encode.c
//
//  Created by weiping wang on 2026/6/6.
//  Copyright © 2026 Amber. All rights reserved.
//

#include "lz16encode.h"

/*
 The main ways to improve the compression speed：
 1. The processing unit was changed from 8bit(Byte) to 16bit(Word)。Hence the name：lz16rans。
    （Because the matching length and distance are 1bit less，it is more beneficial to the compression rate.）
 2. Do not use the sliding window,but replace it matching distance detection.
    if( matchdistance>=MYMATCHOFFSET_MAX )
 3. When the literal value of the match is written back to hash，the minimum sample size is used。
    lz16encode.h: #define  myWriteMatchtoHash(pht,pin,len,dis,pb)
   （Because the amount of filling hash is reduced，it is beneficial to the compression rate.）
*/

static inline int CheckBuffer(BYTE * poutbuffer,const int olen,const int ilen){
    if ( ilen<=LIMIT_LEN || olen< 2*ilen || olen<1024 ) return 1;
    //Input and output buffer length requirements。
    memset(poutbuffer,0,2*ilen);
    return 0;
}

int lz16ENhashm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen)
{
    if( CheckBuffer(pout,outlen,inlen)) return -10;
    
    int Islarge=(outlen>=inlen*2.5)? 1:0;
    mylz16ENheadInit(COMPRESS_MYLZ16,0,Islarge);
    
    DWORD *pmyht=(DWORD *)(MYMATCHLISTCDL *)calloc((1<<HASH4_BIT),sizeof(DWORD));
    if(!pmyht) return -100;

    myWriteHash(pmyht,pi,pbase);
    *pchar++=*pi++;
    int charlen=1;
    int halfid=0;
    while(pi<pinEnd){
        DWORD hashkey=myGetHashkey(pi);
        DWORD *pmatch=(DWORD *)(pbase+pmyht[hashkey]);
        pmyht[hashkey]=(DWORD)(pi-pbase);
        if( *pmatch ^ ((DWORD *)pi)[0] ){
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchdistance=(DWORD)(pi-(WORD *)pmatch);
        if( matchdistance>=MYMATCHOFFSET_MAX ){   //over matchdistance,back!!!
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchlen;
        CharEQcountmacro(pi,pmatch,matchlen);
        mylz16ENhashmCode();
    }
    
    mylz16ENtailEnd(Islarge);
    free(pmyht);
    return ph->myTotalbytes;
}

int lz16ENhashmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen)
{
    if( CheckBuffer(pout,outlen,inlen)) return -10;
    
    int Islarge=(outlen>=inlen*2.5)? 1:0;
    mylz16ENheadInit(COMPRESS_MYLZ16,1,Islarge);
    
    DWORD *pmyht=(DWORD *)(MYMATCHLISTCDL *)calloc((1<<HASH4_BIT),sizeof(DWORD));
    if(!pmyht) return -100;

    myWriteHash(pmyht,pi,pbase);
    *pchar++=*pi++;
    int charlen=1;
    int halfid=0;
    while(pi<pinEnd){
        DWORD hashkey=myGetHashkey(pi);
        DWORD *pmatch=(DWORD *)(pbase+pmyht[hashkey]);
        pmyht[hashkey]=(DWORD)(pi-pbase);
        if( *pmatch ^ ((DWORD *)pi)[0] ){
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchdistance=(DWORD)(pi-(WORD *)pmatch);
        if( matchdistance>=MYMATCHOFFSET_MAX ){   //over matchdistance,back!!!
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchlen;
        CharEQcountmacro(pi,pmatch,matchlen);
        mylz16ENhashmcCode();
    }
    
    mylz16ENtailEnd(Islarge);
    free(pmyht);
    return ph->myTotalbytes;
}


int lz16ENhashDm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen)
{
    if( CheckBuffer(pout,outlen,inlen)) return -10;
    
    int Islarge=(outlen>=inlen*2.5)? 1:0;
    mylz16ENheadInit(COMPRESS_MYLZ16,0,Islarge);
    
    DWORD *pmyht=(DWORD *)(MYMATCHLISTCDL *)calloc((1<<HASH4_BIT), sizeof(DWORD)+sizeof(MYMATCHLISTCDL));  //LHL
    const MYMATCHLISTCDL * pcdll=(MYMATCHLISTCDL *)(pmyht+(1<<HASH4_BIT));
    if(!pmyht) return -100;

    myWriteHash(pmyht,pi,pbase);
    *pchar++=*pi++;
    int charlen=1;
    int halfid=0;

    while(pi<pinEnd){
        DWORD hashkey=myGetHashkey(pi);
        DWORD *pmatch=(DWORD *)(pbase+pmyht[hashkey]);
        pmyht[hashkey]=(DWORD)(pi-pbase);
        if( *pmatch ^ ((DWORD *)pi)[0] ){
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchlen;
        CharEQcountmacro(pi,pmatch,matchlen);
        //------------------------------- Once again hash, Improve the hit rate -------------------------------
        DWORD *pcdl= (DWORD *)(pcdll + hashkey)+myGetKeyListIndex(pi);
        DWORD * pm=(DWORD *)(pbase+*pcdl);
        *pcdl=(DWORD)(pi-pbase);
        if((*pm ^ *((DWORD *)pi))==0  ){
            DWORD n;
            CharEQcountmacro(pi,pm,n);
            if( n > matchlen ){
                pmatch = pm;
                matchlen=n;
            }
        }
        //------------------------------- Once again hash, Improve the hit rate -------------------------------
        DWORD matchdistance=(DWORD)(pi-(WORD *)pmatch);
        if( matchdistance>=MYMATCHOFFSET_MAX )
        {
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        mylz16ENhashmCode();
    }
    
    mylz16ENtailEnd(Islarge);
    free(pmyht);
    return ph->myTotalbytes;
}

int lz16ENhashDmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen)
{
    if( CheckBuffer(pout,outlen,inlen)) return -10;
    
    int Islarge=(outlen>=inlen*2.5)? 1:0;
    mylz16ENheadInit(COMPRESS_MYLZ16,1,Islarge);
    
    DWORD *pmyht=(DWORD *)(MYMATCHLISTCDL *)calloc((1<<HASH4_BIT), sizeof(DWORD)+sizeof(MYMATCHLISTCDL));  //LHL
    const MYMATCHLISTCDL * pcdll=(MYMATCHLISTCDL *)(pmyht+(1<<HASH4_BIT));

    myWriteHash(pmyht,pi,pbase);
    *pchar++=*pi++;
    int charlen=1;
    int halfid=0;

    while(pi<pinEnd){
        DWORD hashkey=myGetHashkey(pi);
        DWORD *pmatch=(DWORD *)(pbase+pmyht[hashkey]);
        pmyht[hashkey]=(DWORD)(pi-pbase);
        if( *pmatch ^ ((DWORD *)pi)[0] ){
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        DWORD matchlen;
        CharEQcountmacro(pi,pmatch,matchlen);
        //------------------------------- Once again hash, Improve the hit rate -------------------------------
        DWORD *pcdl= (DWORD *)(pcdll + hashkey)+myGetKeyListIndex(pi);
        DWORD * pm=(DWORD *)(pbase+*pcdl);
        *pcdl=(DWORD)(pi-pbase);
        if((*pm ^ *((DWORD *)pi))==0  ){
            DWORD n;
            CharEQcountmacro(pi,pm,n);
             if( n > matchlen ){
                pmatch=pm;
                matchlen=n;
            }
        }
        //------------------------------- Once again hash, Improve the hit rate -------------------------------
        DWORD matchdistance=(DWORD)(pi-(WORD *)pmatch);
        if( matchdistance>=MYMATCHOFFSET_MAX )
        {
            *pchar++=*pi++;
            charlen++;
            continue;
        }
        mylz16ENhashmcCode();
    }
    
    mylz16ENtailEnd(Islarge);
    free(pmyht);
    return ph->myTotalbytes;
}
