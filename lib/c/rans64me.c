//
//  rans_main.c
//
//  Created by weiping wang on 2026/6/19.
//  Copyright © 2026 Amber. All rights reserved.
//
#include "../rans/rans64_main.h"

#include "lz16.h"

int ranscompress(uint8_t* pin,uint8_t** pout,int inlen,int outlen)
{
    static const uint32_t prob_bits = 14;
    static const uint32_t prob_scale = 1 << prob_bits;
    SSTATS ss={0};
    count_freqs(pin, inlen,&ss);
    normalize_freqs(prob_scale,&ss);
    
    Rans64EncSymbol esyms[256];
    for (int i=0; i < 256; i++) {
         Rans64EncSymbolInit(&esyms[i], ss.cum_freqs[i], ss.freqs[i], prob_bits);
    }
    Rans64State rans0,rans1;
    Rans64EncInit(&rans0);
    Rans64EncInit(&rans1);
    
    uint32_t* ptr = (uint32_t *)((*pout) + outlen); // *end* of output buffer

    // odd number of bytes?
    if (inlen & 1) {
        int s = pin[inlen - 1];
        Rans64EncPutSymbol(&rans0, &ptr, &esyms[s],prob_bits);
    }

    for (size_t i=(inlen & ~1); i > 0; i -= 2) { // NB: working in reverse!
        int s1 = pin[i-1];
        int s0 = pin[i-2];
        //#####################################################################
        Rans64Assert(esyms[s0].freq != 0);
        Rans64Assert(esyms[s1].freq != 0);
         // renormalize
        uint64_t x_max = ((RANS64_L >> prob_bits) << 32) * esyms[s1].freq; // this turns into a shift.
        if (rans1 >=x_max ) {    //  esyms[s1].x_max
             *(--ptr) = (uint32_t) rans1;
            rans1 >>= 32;
        }
        // x = C(s,x)
        uint64_t q = Rans64MulHi(rans1, esyms[s1].rcp_freq) >> esyms[s1].rcp_shift;
        rans1 = rans1 + esyms[s1].bias + q * esyms[s1].cmpl_freq;
        
        // renormalize
        x_max = ((RANS64_L >> prob_bits) << 32) * esyms[s0].freq; // this turns into a shift.
        if (rans0 >=x_max ) {     //esyms[s0].x_max
             *(--ptr) = (uint32_t) rans0;
            rans0 >>= 32;
        }
        // x = C(s,x)
        q = Rans64MulHi(rans0, esyms[s0].rcp_freq) >> esyms[s0].rcp_shift;
        rans0 = rans0 + esyms[s0].bias + q * esyms[s0].cmpl_freq;
        //#####################################################################
    }
    Rans64EncFlush(&rans1, &ptr);
    Rans64EncFlush(&rans0, &ptr);
    //++++++++++++++++++++++++++++++++++++++++ update ++++++++++++++++++++++++++++++++++++++++
    BYTE * pfreqs =(BYTE *)ptr;
    for(int i=255 ; i >= 0 ; i-- ){
        if( ss.freqs[i] <= 0x3f){
            pfreqs--;
            *pfreqs=((BYTE)ss.freqs[i])<<2 | 0;
        }else if( ss.freqs[i] <= 0x3fff ){
            pfreqs-=2;
            *((WORD *)pfreqs)= ((WORD)ss.freqs[i])<<2  | 1;
        }else if( ss.freqs[i] <= 0x3fffff ){
            pfreqs-=2;
            *((WORD *)pfreqs)= (WORD)ss.freqs[i];
            pfreqs-=1;
            *pfreqs = ((BYTE)(ss.freqs[i] & 0x3f0000))>>14 | 2;
        }else{
            pfreqs-=4;
            *((DWORD *)pfreqs)= ss.freqs[i]<<2 | 3;
        }
        //printf("%3d %8d(%8x)\n",i,ss.freqs[i],ss.freqs[i]);
    }
    int enlen=(int) (*pout+outlen - pfreqs)+4*2;
    *pout=pfreqs-4*2;
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    uint32_t* penbufstart =(uint32_t *)(*pout);
    penbufstart[0]=inlen;
    penbufstart[1]=enlen;
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //++++++++++++++++++++++++++++++++++++++++ update ++++++++++++++++++++++++++++++++++++++++
    return enlen;
}

int ransuncompress(uint8_t* pin,uint8_t* pout,int inlen,int outlen)
{
    static const uint32_t prob_bits = 14;
    static const uint32_t prob_scale = 1 << prob_bits;
    enum{enum_prob_scale=1 << prob_bits};
    SSTATS ss={0};
    uint32_t * penbuf=(uint32_t *)pin;
    //++++++++++++++++++++++++++++++++++++++++ update ++++++++++++++++++++++++++++++++++++++++
    int rawlen=penbuf[0];
    if( rawlen > outlen ) return -1;
    penbuf+=2;
    BYTE * pfreqs =(BYTE *)penbuf;
    for(int i=0;i<256;i++){
        int n= (*pfreqs & 3)+1;
        switch ( n ) {
            case 1:
                ss.freqs[i]=((*pfreqs)>>2) & 0x3f ;
                break;
            case 2:
                ss.freqs[i]=(*((WORD *)pfreqs)>>2) & 0x3fff ;
                break;
            case 3:
                ss.freqs[i]= (((*pfreqs) >>2 ) & 0x3f)<<16 ;
               ss.freqs[i] |=*((WORD *)(pfreqs+1)) & 0xffff ;
                break;
            case 4:
                ss.freqs[i]=(*((DWORD *)pfreqs)>>2) & 0x3fffffff ;
               break;
        }
        pfreqs+=n;
        //printf("%3d %8d(%8x)\n",i,ss.freqs[i],ss.freqs[i]);
    }
    penbuf=(uint32_t *)pfreqs;
    //++++++++++++++++++++++++++++++++++++++++ update ++++++++++++++++++++++++++++++++++++++++
    normalize_freqs(prob_scale,&ss);
    uint8_t cum2sym[enum_prob_scale]={0};
    for (int s=0; s < 256; s++)
        for (uint32_t i=ss.cum_freqs[s]; i < ss.cum_freqs[s+1]; i++)
            cum2sym[i] = s;
    
    Rans64DecSymbol dsyms[256];
    for (int i=0; i < 256; i++) {
        Rans64DecSymbolInit(&dsyms[i], ss.cum_freqs[i], ss.freqs[i]);
    }

    uint32_t* ptr = penbuf;
    Rans64State rans[2];
    Rans64DecInit(&rans[0], &ptr);
    Rans64DecInit(&rans[1], &ptr);

    const DWORD mask=(prob_scale - 1);
    for (size_t i=0; i < (rawlen & ~1); i += 2) {
        uint8_t s0= cum2sym[rans[0] & mask];
        uint8_t s1= cum2sym[rans[1] & mask];
        pout[i+0] = s0;
        pout[i+1] = s1;
        //#####################################################################
        uint64_t x = rans[0];
        x = dsyms[s0].freq * (x >> prob_bits) + (x & mask) - dsyms[s0].start;
        while (x < RANS64_L)  x = (x << 32) | *ptr++;
        rans[0] = x;
        //+++++++++++++++++++++++++++++++++++++++++++++++
        x = rans[1];
        x = dsyms[s1].freq * (x >> prob_bits) + (x & mask) - dsyms[s1].start;
        while (x < RANS64_L)  x = (x << 32) | *ptr++;
        rans[1] = x;
        //#####################################################################
    }
    // last byte, if number of bytes was odd
    if (rawlen & 1) {
        uint32_t s0 = cum2sym[Rans64DecGet(&rans[0], prob_bits)];
        pout[rawlen - 1] = (uint8_t) s0;
        Rans64DecAdvanceSymbol(&rans[0], &ptr, &dsyms[s0], prob_bits);
    }
    return rawlen;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ encode , decode @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int mylz16torans64(BYTE **pout,const PMYIOHEAD ph,const int outlen){
    const PMYCOMPRESSITEM pitem=(PMYCOMPRESSITEM)((BYTE *)ph+sizeof(MYIOHEAD));

    if( pitem->flag !=COMPRESS_MYLZ16 ) return -1;
    
    int lz16headlen=ph->offset;
    int len[MYSECTION_COUNT]={0};                   // len[i](lz16 section):rans lenght(not compress: 0)
    int sum=0;                                      // the data length of rans Encode
    int sum_notrans=0;                              // the data length of not rans Encode
    int index=0;                                    // the count of rans Encode section
    int checkok=1;                                  // check rans rans

    BYTE *pitail=(BYTE *)ph+ph->offset+pitem->outlen;
    for(int i=MYSECTION_COUNT-1; 0 <= i;i--){
        pitail-=pitem->len[i];
        if( pitem->len[i] < 4*1024 && checkok ){    //current section the lenght of rans Encode < 4*1024,not rans Encode
            //--------------------  not rans Encode  --------------------
            sum_notrans+=pitem->len[i];
            BYTE * potail=*pout+(outlen-sum_notrans);
            memcpy(potail,pitail,pitem->len[i]);
            //--------------------  not rans Encode  --------------------
            //printf("not rans:[%2d,%2d] [%8d,%8d]%8d\n",i,index,sum,sum_notrans,outlen-(sum+sum_notrans));
            continue;
        }
        checkok=0;  // once rans Encode，rest sections all rans Encode。
        BYTE *po=(*pout);
        len[i]=ranscompress(pitail,&po,pitem->len[i],outlen-(sum+sum_notrans));
        //printf("[%2d,%2d](lz:%8d,rans:%8d) %6.4f \n",i,index,pitem->len[i],len[i],len[i]/(float)pitem->len[i]);
        sum+=len[i];
        index++;
    }
    if(sum == 0 )   return 0;                                       // lz16 data not rans Encode。
    //printf("   ransEN:%8d sum=%8d[%8d]   %6.4f[%6.4f](%6.4f) \n",pitem->outlen,sum,pitem->outlen-sum,sum/(float)pitem->outlen,pitem->outlen/(float)pitem->inlen,sum/(float)pitem->inlen);
    *pout+=outlen-(sum+sum_notrans);
    //############################# copy lz16 protocol head #############################
    ph->items++;
    *pout-=(lz16headlen+sizeof(MYIOHEAD)+sizeof(MYCOMPRESSITEM));
    memcpy(*pout,(void *)ph,sizeof(MYIOHEAD)+ph->offset);       //copy lz16 protocol head
    //############################# copy lz16 protocol head #############################
//++++++++++++++++++++++++++++ rans protocol head  ++++++++++++++++++++++++++++
    PMYIOHEAD phrans=(PMYIOHEAD)(*pout+lz16headlen);
    PMYCOMPRESSITEM pransitem=(PMYCOMPRESSITEM)((BYTE *)phrans+sizeof(MYIOHEAD));
    phrans->items=ph->items-1;
    memcpy(pransitem->len,len,MYSECTION_COUNT*sizeof(int));

    pransitem->flag=COMPRESS_RANS_64_02;                        //Encode id
    pransitem->items=pitem->items+1;//???????
    pransitem->index=index;                                     //the count of rans Encode the lz16 sections
    pransitem->inlen=pitem->outlen;                             //the in length of rans Encode = the out lenght of lz16 Encode
    pransitem->outlen=(sum+sum_notrans);                        //the out length of rans Encode
    phrans->offset=sizeof(MYIOHEAD)+sizeof(MYCOMPRESSITEM);     //the offset of rans Encode data
    phrans->items=ph->items-1;      //???????
    phrans->len=sum+sum_notrans;
    phrans->myTotalbytes=phrans->offset+phrans->len;            //out Total bytes: the lenght of protocol head + the length of rans Encode data
    //++++++++++++++++++++++++++++ rans protocol head  ++++++++++++++++++++++++++++
    return phrans->myTotalbytes;
}

int myrans64tolz16(BYTE *pout,const BYTE * pin,const int outlen){
    PMYIOHEAD phlz16=(PMYIOHEAD)pin;
    int lz16headlen=phlz16->offset;
    PMYCOMPRESSITEM pitemlz16=(PMYCOMPRESSITEM)((BYTE *)phlz16+sizeof(MYIOHEAD));
    BYTE *polz16buf= pout + lz16headlen;
    
    PMYIOHEAD phrans=(PMYIOHEAD)(pin+lz16headlen);
    PMYCOMPRESSITEM pitemrans=(PMYCOMPRESSITEM)((BYTE *)phrans+sizeof(MYIOHEAD));
    
    if( pitemrans->flag !=COMPRESS_RANS_64_02 ) return -1;

    if( pitemrans->inlen > outlen ) return -2;

    BYTE *pransbuf=(BYTE *)phrans+phrans->offset;   //rans data start
    for(int i=0;i<=MYSECTION_COUNT-1 ;i++){
        //printf("[index:%2d,i:%2d] (lzlen:%8d,ranslen:%8d)\n",pitemrans->index,i,pitemlz16->len[i],pitemrans->len[i]);
        int lzlen=pitemlz16->len[i];
        int ranslen=pitemrans->len[i];
        if( pitemrans->index > i){  //lz16 section the count of rans Encode
            assert(lzlen==((DWORD *)pransbuf)[0]);
            assert(ranslen==((DWORD *)pransbuf)[1]);
            //--------------------------- rans Decode ---------------------------
            ransuncompress(pransbuf,polz16buf,ranslen,outlen);
            pransbuf+=ranslen;
            //--------------------------- rans Decode ---------------------------
        }else{
            //==================== copy lz16 Data(not rans Encode) ====================
            memcpy(polz16buf,pransbuf,lzlen);
            pransbuf+=lzlen;                       //ranslen=0;
            //==================== copy lz16 Data(not rans Encode) ====================
        }
        polz16buf+=lzlen;
    }
    pitemlz16->items--;
    memcpy(pout,pin,lz16headlen);
    return pitemrans->inlen;
}
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ encode , decode @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
