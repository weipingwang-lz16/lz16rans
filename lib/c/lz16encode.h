//
//  lz16encode.h
//
//  Created by weiping wang on 2026/6/6.
//  Copyright © 2026 Amber. All rights reserved.
//

#include "lz16.h"
#ifndef lz16Encode_h
#define lz16Encode_h

#define myGetHashkey(p) (DWORD)((*((DWORD *)p) * 514229U) >> (32 - HASH4_BIT))
#define myWriteHash(pt,p, pb) pt[myGetHashkey(p)]=(DWORD)(p-pb)

#define myGetKeyListIndex(p) ( p[8] & (MYRINGQUEUE_COUNT-1))

#define mylz_WriteLength(p, l)   \
l-=15;              \
while( l>=255 ){    \
    l-=255;         \
    *p++=0xff;      \
}                   \
*p++=l;

#define mylz_ENWritematchdistance4(pdisflag,id,pdis,matchdistance)  \
DWORD dis =matchdistance;                                           \
if( dis<0x20){                                                      \
    *pdisflag++ = id | dis ;                                        \
    continue;                                                       \
}else{                                                              \
    dis >>=4;                       \
    *((DWORD *)pdis)=dis;           \
    int i= (dis + 0xff00)>>16;      \
    pdis+=++i;                      \
    *pdisflag++ = id | ((++i)<<4) | (matchdistance & 0x0f) ;        \
}                                                                   \
continue; 

#define mylz_WriteLength44(p,n,l)   \
if(l<15){                           \
    *p |= l <<(n<<2);               \
    p +=n;                          \
    n ^=1;                          \
}else if(l<30){                     \
    l-=15;                          \
    WORD *pw=(WORD *)p++;           \
    *pw |=((l<<4) | 0x0f )<<(n<<2); \
}else{                              \
    l+=255-30;                      \
    WORD *pw=(WORD *)p++;           \
    int x=0xff<<(n<<2);             \
    do{                             \
        *pw |=x;                    \
        l-=255;                     \
        pw=(WORD *)p++;             \
    }while(l>=255);                 \
    *pw |=l<<(n<<2);                \
}

#define CharEQcountmacro(p,pm,ml)   \
{   \
    QWORD d;                                \
    QWORD *pMatch=(QWORD *)(pm+1);          \
    const WORD *pIn=p+2;                    \
    do{                                     \
        d= *pMatch++ ^ ((QWORD *)pIn)[0];   \
        pIn+=4;                             \
    }while ( (pIn<pinEnd ) && !d);          \
    int i=(((d & 0xffffffffffff ) + 0xffffffffffff)>>48)+(((d & 0xffffffff ) + 0xffffffff)>>32)+(((d & 0xffff) + 0xffff)>>16); \
    ml=(int)(pIn-p)-i-3;            \
}

static inline void mylz16GetCharlen(const PMYLZITEM p,const BYTE *pt,PMYCOMPRESSITEM poutitems,int i){
    if( poutitems->outlen >=0 ){
        poutitems->len[i]=(int)((BYTE *)pt-p[i].pbuf);
        poutitems->outlen+=poutitems->len[i];
        //printf("out: %8d, %8d\n",poutitems->outlen,poutitems->len[i]);
        if( poutitems->len[i] > p[i].len ){
            poutitems->outlen=SECTION_OVER;
        }
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Prove: The number of sample information units in the found matching information is not greater than the number of information units between the matching start position and the search start position.
// Let:
// The number of matching information units: m
// The number of units between the matching start position and the search start position: n
// Current state: A(1),...,A(i),A(i+1),...,A(n),A(n+1),...
// Matching information start unit position: 1
// Matching information start unit: A(1)
// Search information start unit position: n+1
// Search information start unit: A(n+1)
// We have:
// (1.0) A(1)=A(n+1),A(2)=A(n+2),...,A(m)=A(n+m).
//
// Case A: There are overlapping information units (m > n).
// Divide the matching information into s+1 segments. Each segment contains n information units, and the last segment contains t information units (t: 0, ..., n-1). We have m = s*n + t. If t = 0, then the number of segments is s.
// Matching information segments: X(1), ..., X(s). Where X(s) = {A(1 + (s-1)*n)), ..., A(n + (s-1)*n))}; the last segment is A(1 + s*n)), ..., A(t + s*n)).
// Searching for information segments: Y(1), ..., Y(s). Where Y(s) = {A(1 + n + (s-1)*n)), ..., A(n + n +(s-1)*n)}; the last segment is A(1 + n + s*n), ..., A(t + n + s*n).
// (2.1) X(i) = Y(i). i = 1, ..., s.
// X(i)={A(1+(i-1)*n)),…,A(n+(i-1)*n))}; Y(i)={ A(1+i*n),…,A(n+i*n)}.
// (2.2) A(1+(i-1)*n))=A(1+i*n),...,A(n+(i-1)*n))=A(n+i*n).
// Let: the sample information be X(1), then the number of sample information units is equal to n.
// And we have:
// X(1)=Y(1)=X(1),...,X(s)=Y(s)=X(1).
//
// Proof: i=2 is true.
// i=2, there are:
// From (2.2): A(1+n)=A(1+2*n),...,A(n+n)=A(n+2*n).
// From (1.0) and (2.1) we have:
// A(1)=A(1+n)=A(1+2*n),...,A(n)=A(n+n)=A(n+2*n).
// X(1)=Y(1)=X(2)=Y(2);
// Assume i=k is true.
// (2.3) X(k)=Y(k)=X(1)
// (2.4) A(1+(k-1)*n)=A(1+k*n)=A(1),...,A(n+(k-1)*n)=A(n+k*n)=A(n).
// when i=k+1
// X(k+1)={A(1+((k+1)-1)*n)=A(1+k*n),...,A(n+((k+1)-1)*n)=A(n+k*n)}。
// From (2.4) we have:
// A(1+((k+1)-1)*n)=A(1+k*n)=A(1),...,A(n+((k+1)-1)*n)=A(n+k*n)=A(n); X(k+1)=X(1).
// From (2.3): X(k+1)=Y(k+1)=X(1)
// So i=k+1 is true.
// Last X, Y segment (t>0):
// X(s+1)={A(1+s*n),...,A(t+s*n))}; Y(s)={A(1 + n + (s-1)*n)), ..., A(n + n +(s-1)*n)}={A(1+s*n), ..., A(t+s*n), ..., A(n+s*n)}.
// From (2.4):
// A(1+s*n)=A(1),...,A(t+s*n)=A(t).
// From (2.3):
// X(s+1)=Y(s+1)
// That is: the last X and Y segments are equal to the front t information units of X(1).
// Because t<n. This is true.
// Conclusion:
// The number of information units in the matching sample X(1) is not greater than the number of information units n between the matching start position and the search start position.
//
// Case B: No overlapping information units (m< n+1).
// The number of information units in the matched sample m, m is less than or equal to the number of units n (m <= n).
// Obviously true!
// So the number of information units in the matched sample: min(m , n)

#define  myWriteMatchtoHash(pht,pin,len,dis,pb) \
    {\
        WORD *p=pin+1;                  \
        int min=((len)>dis)?dis:(len);  \
        for(int i=1; i<min; i++){       \
            myWriteHash(pht,p,pb);      \
            p++;                        \
        }                               \
        pin+=len;                       \
    }

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
The sections of lz16 storage buffer is 5. pchar,pdistanceflag,pdistance,pflag and pflagExx.
WORD *pchar:            not match characters
BYTE * pdistanceflag:   the id of match & the byte count of distance & distance (iixx,dddd,8bit.)
                        ii(match id,2bit.):
                        matchlen:           the length of match (0: least match length is 2 word)
                        charlen:            the length of not match( Recently searched characters)
                        00: matchlen=0,charlen=0.
                        10: matchlen>0,charlen=0.
                        01: matchlen=0,charlen>0.
                        11: matchlen>0,charlen>0.
                        xx(out to pdistance bytes,2bit.):
                        the rest distance of through pflag processed: dis
                        0x(ii0d,dddd): dis<0x20.(pdistance: NULL)
                        1x(111x,dddd): dis>=0x20.put in pdistance bytes between 1 to 2(#define MYMATCHOFFSET_MAX (1<<(16+4))).
                                       1110,dddd,put in pdistance bytes is 1.
                                       1111,dddd,put in pdistance bytes is 2.
BYTE * pdistance:       the highest part of distance(in bytes),0---2 byte.
BYTE * pflag:           merge matchlen,charlen and the part of distance
                        (m(bit): matchlen, c(bit):charlen, d(bit): the part of distance)
                        //================== compress level(2,4) ==================
                        00xx(pdistanceflag): NULL               (pflagExx: dddddddd)
                        10xx(pdistanceflag): mmmd,dddd
                        01xx(pdistanceflag): ccdd,dddd
                        11xx(pdistanceflag): mmmc,cddd
                        //================== compress level(2,4) ==================
                        //------------------ compress level(1,3) ------------------
                        00xx(pdistanceflag): NULL               (pflagExx: NULL)
                        10xx(pdistanceflag): mmmd,dddd
                        01xx(pdistanceflag): NULL
                        11xx(pdistanceflag): mmmd,dddd
                        //------------------ compress level(1,3) ------------------
BYTE * pflagExx:        put in matchlen or charlen.
                        ( #define mylz_ENWritematchdistance4(pdisflag,id,pdis,matchdistance) )
*/
//######################################## lz16  Encode head  initalization #########################################
#define mylz16ENheadInitsectionlen(f)\
    int savelen[MYSECTION_COUNT]={0};   /*Lz16 sections capacity*/          \
    savelen[0]=inlen*3/4 +f*(inlen>>2)+(inlen & 1)+MYREST_LEN; /* inlen*75% (pchar:capacity) */ \
    pitem->len[0]=savelen[0];                                               \
    int endlen=savelen[0];                                                  \
for(int i=1;i<MYSECTION_COUNT;i++){                                         \
    if( i==2 ){\
        savelen[i]=(inlen>>1) +f*(inlen>>3)+MYREST_LEN;                     \
        /* inlen*50%(pdistance: capacity) */               \
    }else{\
        savelen[i]=(inlen>>2)+f*(inlen>>4)+MYREST_LEN;                      \
        /* inlen*25%(pdistanceflag,pflag,pflagExx: capacity) */             \
    }\
    pitem->len[i]=savelen[i];                                           \
    endlen+=savelen[i];                                                 \
}

#define mylz16ENheadInitEx( enid,codeid, f)    \
    const PMYIOHEAD ph=(PMYIOHEAD)pout;                                     \
    const PMYCOMPRESSITEM pitem=(PMYCOMPRESSITEM)(pout+sizeof(MYIOHEAD));   \
    ph->offset=sizeof(MYIOHEAD)+sizeof(MYCOMPRESSITEM);                     \
    pitem->flag=enid;                               /* compress id */       \
    pitem->flagsub=codeid;                                                  \
    pitem->items=1;                                 /* the count of protocol head */    \
    pitem->index=MYSECTION_COUNT;                   /* the count of lz16 sections */    \
    pitem->inlen=inlen;                             /* put in length */          \
    pitem->outlen=0;                                /* put out length*/          \
    mylz16ENheadInitsectionlen(f);  /* 0826 new update */        \
    MYLZITEM mylzitems[MYSECTION_COUNT]={0};                                \
    PMYLZITEM pmylzitems= &mylzitems[0];                                    \
                                                                            \
    mylzitems[0].pbuf=(BYTE *)ph+ph->offset;                                \
    mylzitems[0].len=pitem->len[0];                                         \
    for(int i=1;i<MYSECTION_COUNT;i++){                                     \
        mylzitems[i].pbuf=mylzitems[i-1].pbuf+pitem->len[i-1];              \
        mylzitems[i].len=pitem->len[i];                                     \
    }

#define mylz16ENheadInit( enid,codeid ,f)     \
    mylz16ENheadInitEx( enid,codeid,f )       \
    WORD *pchar=(WORD *)mylzitems[0].pbuf;  /* pchar: not match characters  */  \
    BYTE * pdistanceflag=mylzitems[1].pbuf; /* pdistanceflag: the id of match & the byte count of distance & distance */    \
    BYTE * pdistance=mylzitems[2].pbuf;     /* pdistance: match distance    */  \
    BYTE * pflag=mylzitems[3].pbuf;         /* pflag: merge matchlen,charlen and the part of distance */                    \
    BYTE * pflagExx=mylzitems[4].pbuf;      /* pflagExx: matchlen or charlen or the part of distance */  \
    \
    const WORD * pbase=(WORD *)pin;                     \
    const WORD *pinEnd=pbase + (inlen>>1)-LIMIT_LEN;    \
    WORD *pi=(WORD *)pin;

//######################################## lz16  Encode head  initalization #########################################

//######################################## lz16 Encode End #########################################
#define mylz16ENtailEndEx()   \
/* ===================== last block encode: matchlen==0 ===================== */           \
int len=(inlen+1)/2-(int)(pi-(WORD *)pin);  /* 1---4( BYTE *pInEnd=pin+inlen-4;) */ \
for(int i=0;i<len;i++)  pchar[i]=pi[i];                     \
charlen+=len;                                               \
pchar+=len;                                                 \
*pflag++=charlen;                                           \
if(charlen>=15){                                            \
    pflag[-1]=0x0f;                                         \
    mylz_WriteLength(pflag,charlen);                        \
}                                                           \
pflagExx++;                                                 \
*pdistanceflag++=0;                                         \
/* ===================== last block encode: matchlen==0 ===================== */           \
mylz16GetCharlen(pmylzitems, (BYTE *)pchar,pitem,0);        \
mylz16GetCharlen(pmylzitems, pdistanceflag,pitem,1);        \
mylz16GetCharlen(pmylzitems, pdistance,pitem,2);            \
mylz16GetCharlen(pmylzitems, pflag,pitem,3);                \
mylz16GetCharlen(pmylzitems, pflagExx,pitem,4);

#define mylz16ENtailEndExx(f)   \
if( pitem->outlen > 0 ) {                                           \
    /* =========================== Merge lz16 sections =========================== */             \
    BYTE *pouthead=(BYTE *)ph+ph->offset;                                                \
    BYTE * pend=pouthead + pitem->len[0];                                                \
    for(int i=1; i<MYSECTION_COUNT;i++){                                                 \
        pouthead+=savelen[i-1];                     /* savelen[i]:the capacity of lz16 section */      \
        memcpy(pend,pouthead,pitem->len[i]);                                             \
        pend+=pitem->len[i];                        /* pitem->len[i]: then decode lenght of lz16 section */     \
    }                                                                                    \
    /* =========================== Merge lz16 sections =========================== */             \
    pitem->items=1;                                             \
    ph->items=1;                                                \
    ph->len=pitem->outlen;                                      \
    ph->myTotalbytes=ph->len+ph->offset;                        \
}else{                                                          \
    if( f ){    \
        ph->myTotalbytes= -1;                                   \
    }else{                                                      \
ph->len=0;                                                      \
for(int i=0;i<MYSECTION_COUNT;i++) ph->len+=pitem->len[i]; \
        ph->myTotalbytes= pitem->outlen;                        \
    }                                                           \
}

#define mylz16ENtailEnd(f)   \
mylz16ENtailEndEx()           \
mylz16ENtailEndExx(f);
//######################################## lz16 Encode End #########################################

//######################################## lz16 encode out (level:2,4) #########################################
#define mylz16ENhashmcCode()   \
BYTE cid=0;                                 \
if( charlen==0){                                                                \
    BYTE cid=0;                                                                     \
    if( matchlen ){                                                                 \
        myWriteMatchtoHash(pmyht,pi,matchlen+2,matchdistance,pbase);            \
        cid=0x80;       /* 10xx: pflag:mmmd,dddd; */            \
        matchlen--;                                                                 \
        *pflag=((matchlen & 0x07)<<5) | (matchdistance & 0x1f); \
        matchdistance >>=5;                                                         \
        if( matchlen>=7 ){                                                          \
            *pflag |= 0xe0 ;  \
            matchlen-=7;                                                            \
            mylz_WriteLength44(pflagExx,halfid,matchlen);                           \
        }                                                                           \
        pflag++;\
        mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);      \
   }else{               /* 00xx: pflag:NULL, pflagExx:dddddddd */                  \
        pi++; myWriteHash(pmyht,pi,pbase); pi++;                                \
        WORD *pdisEx=(WORD *)pflagExx++;                                       \
        *pdisEx |= (matchdistance & 0xff)<<(halfid<<2);                        \
        matchdistance >>=8;                                                     \
        mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);  \
    }                                                                           \
}                                                                               \
cid=0x40;               /* 01xx:  pflag:ccdd,dddd; */           \
charlen--;                                                      \
BYTE c= (charlen & 0x03);                                       \
if( charlen>=3 ){                                               \
    c=3;                                                        \
    charlen-=3;                                                 \
    mylz_WriteLength44(pflagExx,halfid,charlen);                \
}                                                               \
charlen=0;                                                      \
if( matchlen ){ /*11xx: pflag:mmmc,cddd */                      \
    myWriteMatchtoHash(pmyht,pi,matchlen+2,matchdistance,pbase);        \
    matchlen--;                                                 \
    cid=0xc0;   /* 11xx*/                                       \
    *pflag = ((matchlen & 0x07)<<5) | (c<<3) | (matchdistance & 0x07);  \
    matchdistance >>=3;                                 \
    if( matchlen>=7 ){                                  \
        *pflag |=0xe0;                                  \
        matchlen-=7;                                    \
        mylz_WriteLength44(pflagExx,halfid,matchlen);   \
    }                                                   \
    pflag++;                                            \
    mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);  \
}                                                       \
/* 01xx: pflag:ccdd,dddd; */                            \
pi++; myWriteHash(pmyht,pi,pbase);pi++;                 \
*pflag= (c<<6)|(matchdistance & 0x3f);                  \
matchdistance >>=6;                                     \
pflag++;                                                \
mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);
//######################################## lz16 encode out (level:2,4) #########################################


//######################################## lz16 encode out (level:1,3) #########################################
#define mylz16ENhashmCode()   \
BYTE cid=0;                                                         \
if( matchlen ){ \
    myWriteMatchtoHash(pmyht,pi,matchlen+2,matchdistance,pbase);    \
    matchlen--;                                                     \
    cid=0x80;          /* 10xx: pflag:mmmd,dddd  */         \
    *pflag=((matchlen & 0x07)<<5) |(matchdistance & 0x1f);  \
    matchdistance >>=5;                                     \
    if(charlen){    \
        charlen--;                                      \
        cid=0xc0;       /* 11xx: pflag:mmmd,dddd */     \
        mylz_WriteLength44(pflagExx,halfid,charlen);    \
        charlen=0;                                      \
    }   \
    if( matchlen>=7 ){  \
        *pflag |= 0xe0 ;                                \
        matchlen-=7;                                    \
        mylz_WriteLength44(pflagExx,halfid,matchlen);   \
    }                                                   \
    pflag++;                                            \
    mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);  \
}else if( charlen ){    \
    charlen--;                                          \
    cid=0x40;           /* 01xx: pflag:NULL */          \
    mylz_WriteLength44(pflagExx,halfid,charlen);        \
    charlen=0;                                          \
}   \
/* 01xx: pflag:NULL */                                  \
pi++; myWriteHash(pmyht,pi,pbase); pi++;                \
mylz_ENWritematchdistance4(pdistanceflag,cid,pdistance,matchdistance);
//######################################## lz16 encode out (level:1,3) #########################################

int lz16ENhashm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashDm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
int lz16ENhashDmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);

int lz16DEm(const BYTE *pout,const BYTE *pin,const int outlen);
int lz16DEmc(const BYTE *pout,const BYTE *pin,const int outlen);

#endif /* lz16Encode_h */
