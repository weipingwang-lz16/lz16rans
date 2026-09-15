//
//  lz16decode.h
//
//  Created by weiping wang on 2026/6/6.
//  Copyright © 2026 Amber. All rights reserved.
//

#ifndef lz16Decode_h
#define lz16Decode_h

#include "lz16.h"

#define mylz_ReadLength(p, l)   \
do{                             \
    l+=*p++;                    \
}while( p[-1]==255 );

#define mylz16_Copy64(po, ps,len)       \
QWORD *ptEx=(QWORD *)po;                \
QWORD *psEx=(QWORD *)ps;                \
for(int i=0; i< (len+3)>>2 ; i++){      \
    *ptEx++=*psEx++;                    \
}                                       \
po+=len;

#define mylz_ReadLength44(p,n,l)    \
int m=*p;                           \
m =(m>>(n<<2)) & 0x0f;              \
if(m<15){                           \
    p +=n;                          \
    n ^=1;                          \
}else{                              \
    WORD *pw=(WORD *)p++;           \
    m+=( (*pw)>>((n + 1)<<2)) & 0x0f;  \
    if(m>=30){                          \
        int y;                          \
        do{                             \
            pw=(WORD *)p++;             \
            y=((*pw)>>(n<<2)) & 0xff;   \
            m+=y;                       \
        }while(y>=255);                 \
    }                                   \
}                                       \
l+=m;

#define mylzDE_Getpmatch(p,dis,pouthead)   \
WORD *pmatch=p-dis;                        \
if( pmatch < (WORD *)pouthead  ){           \
    printf("(pmatch < (WORD *)pout):%8d  dis[%8x]pmatch:%12llx,pout:%12llx\n",(int)(p-(WORD *)pouthead),dis,(int64_t)pmatch,(int64_t)pouthead);\
    return -4;                              \
}

#endif /* lz16Decode_h */
