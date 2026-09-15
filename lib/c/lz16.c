//
//  lz16.c
//
//  Created by weiping wang on 2026/6/20.
//  Copyright © 2026 Amber. All rights reserved.
//
#include "lz16.h"

extern int lz16ENhashm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
extern int lz16ENhashmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
extern int lz16ENhashDm(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);
extern int lz16ENhashDmc(BYTE * const pout,BYTE * const pin,const int outlen,const int inlen);

extern int lz16DEmc(const BYTE *pout,const BYTE *pin,const int outlen);
extern int lz16DEm(const BYTE *pout,const BYTE *pin,const int outlen);

extern int mylz16torans64(BYTE **pout,const PMYIOHEAD ph,const int outlen);
extern int myrans64tolz16(BYTE *pout,const BYTE * pin,const int outlen);

int mylz16Encode(uint8_t **pout,const uint8_t* pin,const int outlen,const int inlen,BYTE level){
    int flag=1,len=0;
    if ( inlen<=LIMIT_LEN ) return 0;
    if( outlen < inlen*2 ) return -2;
    assert(pout && pin);
    assert(*pout);
    int outlenEx=outlen;
    BYTE *po,*poEx=NULL;
    BYTE *ps;
    while( 1 ){
        po=*pout;
        ps=(BYTE *)pin;
        switch ( level ) {
            case 1:
                len=lz16ENhashm(po,(BYTE *)ps,outlenEx,inlen);
                break;
            case 3:
                len=lz16ENhashDm(po,(BYTE *)ps,outlenEx,inlen);
                break;
            case 2:
                len=lz16ENhashmc(po,(BYTE *)ps,outlenEx,inlen);//use rans.
                break;
            case 4:
                len=lz16ENhashDmc(po,(BYTE *)ps,outlenEx,inlen);//use rans.
                break;
            default:
                len=lz16ENhashDmc(po,(BYTE *)ps,outlenEx,inlen);//use rans.
                level=4;
        }
        if( len==SECTION_OVER){
            PMYIOHEAD ph=(PMYIOHEAD)po;
            if( flag && (ph->len/(float)inlen)>=1.1 ){
                poEx=(BYTE *)malloc(inlen * 2.5+4);
                assert(poEx);
                po=poEx;
                outlenEx=inlen * 2.5+4;
                flag=0;
                continue;
            }
        }
        if( flag==0 ){
            memcpy(*pout,po,inlen * 2);
            free(poEx);
        }
        break;
    }
     
    if(len>=inlen || len<0){     //not compress!
        PMYIOHEAD ph=(PMYIOHEAD)po;
        int headlen=ph->offset;
        PMYCOMPRESSITEM pitem=(PMYCOMPRESSITEM)((BYTE *)ph+sizeof(MYIOHEAD));
        pitem->inlen=inlen;
        pitem->outlen=0;
        pitem->items=0;
        ph->items=0;
        ph->len=inlen;
        ph->myTotalbytes=headlen+inlen;
        memcpy((BYTE *)po+headlen,pin,inlen);
        return ph->myTotalbytes;
    }
    if( level & 1 ) {
        *pout=po;
    }else{
        //use rans to compress again
        int n = mylz16torans64(pout,(PMYIOHEAD)po,outlen);
        if( n==0 ) return len;// not use rans compress!
        len=n;
    }
    return len;
}

int mylz16Decode(uint8_t* pout,uint8_t * pin,const int outlen){
    int len=0;
    BYTE * plz16=pin;
    BYTE * pransde=NULL;
    PMYIOHEAD phtop=(PMYIOHEAD)pin;
    PMYCOMPRESSITEM pitemlz16=(PMYCOMPRESSITEM)((BYTE *)phtop+sizeof(MYIOHEAD));

    if( phtop->items==0 ) {
        int headlen=phtop->offset;
        memcpy((BYTE *)pout,(BYTE *)pin+headlen,phtop->len);
        return phtop->len;
    }else if(phtop->items==1 ){
        if( pitemlz16->flag != COMPRESS_MYLZ16 ) return -1;
    }else if(phtop->items==2 ){
        PMYIOHEAD phrans=(PMYIOHEAD)((BYTE *)phtop+phtop->offset);
        PMYCOMPRESSITEM pitemrans=(PMYCOMPRESSITEM)((BYTE *)phrans+sizeof(MYIOHEAD));
        if(pitemrans->flag != COMPRESS_RANS_64_02 ) return -1;
            
        int lz16len=pitemrans->inlen +phtop->offset;
        pransde=malloc(lz16len);
        assert(pransde);
        memset(pransde,0,lz16len);
            
        len=myrans64tolz16(pransde,pin,lz16len);
        if( len !=pitemrans->inlen ) return -3;
        plz16=pransde;
    }else{
        return -1;
    }
    if( pitemlz16->flagsub ){
        len = lz16DEmc((BYTE *)pout,(BYTE *)plz16,outlen);
    }else{
        len = lz16DEm((BYTE *)pout,(BYTE *)plz16,outlen);
    }

    if(pransde) free(pransde);
    return len;
}
