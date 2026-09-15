//
//  lz16decode.c
//
//  Created by weiping wang on 2026/6/6.
//  Copyright © 2026 Amber. All rights reserved.
//

#include "lz16decode.h"

int lz16DEmc(const BYTE *pout,const BYTE *pin,const int outlen)
{
    static const DWORD mask[4]={0,0x0ff,0xffff,0xffffff};
    PMYCOMPRESSITEM pitem=(PMYCOMPRESSITEM)(pin+sizeof(MYIOHEAD));
    if( pitem->flag != COMPRESS_MYLZ16 ) return -1;
    //Compress id:pitem->flag.
    if( pitem->inlen >outlen || pitem->index != MYSECTION_COUNT ) return -2;
    //sections count:pitem->index.
    
    const PMYIOHEAD ph=(PMYIOHEAD)pin;
    WORD *pchar=(WORD *)((BYTE *)ph+ph->offset);
    BYTE * pdistanceflag=(BYTE *)pchar+pitem->len[0];
    BYTE * pdistance=pdistanceflag+pitem->len[1];
    BYTE * pflag=pdistance+pitem->len[2];
    BYTE * pflagExx=pflag+pitem->len[3];

    WORD * po=(WORD *)pout;
    const WORD * poend=(WORD *)(pout+pitem->inlen);
    const BYTE * piend=pdistanceflag+(pitem->len[1]-1);     //-1:last block。
    int halfid=0;
    BYTE c;
    DWORD matchdistance;
    while( pdistanceflag<piend )
    {
            c=*pdistanceflag++;         //iixx,dddd
            matchdistance = c & 0x1f;
            if( c & 0x20 )
            {   //1ixxdddd
                int i= ((c >>4) & 3) -1;
                matchdistance = ((((DWORD *)pdistance)[0] & mask[i]) <<4) | (c & 0x0f);
                pdistance+=i;
            }
            if( c & 0x80 )
            {   //1ixxdddd
                if((c & 0x40)==0)
                {   //10xx,dddd,    //pflag: mmmd,dddd;
                    matchdistance =  (matchdistance<<5) | (*pflag & 0x1f);
                }
                else
                {   //11xx,dddd,    //pflag:mmmc,cddd;
                    int charlen=(*pflag >> 3) & 0x03;
                    matchdistance =  (matchdistance<<3) | (*pflag & 0x07);
                    if( charlen>=0x03 )
                    {
                        mylz_ReadLength44(pflagExx,halfid,charlen);
                    }
                    charlen++;
                    if( po+charlen > poend) return -3;//goto errorExit3;
                    
                    mylz16_Copy64(po, pchar,charlen);
                    pchar+=charlen;
                }
                mylzDE_Getpmatch(po,matchdistance,pout);
                int matchlen=( *pflag++ >>5) & 0x07;
                if(matchlen>=0x07)
                {
                    mylz_ReadLength44(pflagExx,halfid,matchlen);
                }
                matchlen+=2+1;
                if( po+matchlen > poend) return -3;
                
                if(matchdistance>3 )
                {
                    mylz16_Copy64(po,pmatch,matchlen);
                    continue;
                }
                else
                {
                    //$$$$$$$$$$$$$$$$$$$$$$$$ the minimum sample size is used $$$$$$$$$$$$$$$$$$$$$$$$
                    QWORD x=((QWORD *)pmatch)[0];
                    if( matchdistance <=0 ) return -5;
                    int n=1+matchlen/matchdistance;
                    for(int i=0; i< n ; i++)
                    {
                        ((QWORD *)po)[0]=x;
                        po+=matchdistance;
                    }
                    po-=n*matchdistance-matchlen;
                    //$$$$$$$$$$$$$$$$$$$$$$$$ the minimum sample size is used $$$$$$$$$$$$$$$$$$$$$$$$
                   continue;
                }
            }
            else
            {
                if( (c & 0x40)==0 )
                {//00xxdddd         //pflagExx: dddddddd
                    WORD *pw=(WORD *)pflagExx++;
                    matchdistance =  (matchdistance<<8) | ((*pw>>(halfid<<2)) & 0xff);
                }
                else
                {//01xxdddd,        //pflag:ccdddddd;
                    int charlen=*pflag++;
                    matchdistance =  (matchdistance<<6) | (charlen & 0x3f);
                    charlen= (charlen>>6) & 0x03;
                    if( charlen==0x03 ){
                        mylz_ReadLength44(pflagExx,halfid,charlen);
                    }
                    charlen++;
                    if( po+charlen > poend) return -3;//goto errorExit3;
                    
                    mylz16_Copy64(po, pchar,charlen);
                    pchar+=charlen;
                 }
                 mylzDE_Getpmatch(po,matchdistance,pout);
                 *po++=*pmatch++;*po++=*pmatch++;
            }
    }
    //===================== last block: matchlen==0 =====================
    int charlen=*pflag++;
    if(charlen==15)  mylz_ReadLength(pflag,charlen);
    if( (po+(charlen-1)) >= poend) return -3;
    
    for(int i=0; i<charlen-1; i++) *po++=*pchar++;
    
    //++++++++++ source length is odd or even ++++++++++++
    ((BYTE *)po)[0]=((BYTE *)pchar)[0];
    charlen=(int)((BYTE *)po-pout)+1;
    if( (pitem->inlen & 1) ==0 )
    {
        ((BYTE *)po)[1]=((BYTE *)pchar)[1];
        charlen++;
    }
    //++++++++++ source length is odd or even ++++++++++++
    //===================== last block: matchlen==0 =====================
    return charlen;
}

int lz16DEm(const BYTE *pout,const BYTE *pin,const int outlen)
{
    static const DWORD mark[4]={0,0x0ff,0xffff,0xffffff};
    PMYCOMPRESSITEM pitem=(PMYCOMPRESSITEM)(pin+sizeof(MYIOHEAD));
    if( pitem->flag != COMPRESS_MYLZ16 ) return -1;
    //Compress id:pitem->flag.
    if( pitem->inlen >outlen || pitem->index != MYSECTION_COUNT ) return -2;
    //sections count:pitem->index.

    const PMYIOHEAD ph=(PMYIOHEAD)pin;
    WORD *pchar=(WORD *)((BYTE *)ph+ph->offset);
    BYTE * pdistanceflag=(BYTE *)pchar+pitem->len[0];
    BYTE * pdistance=pdistanceflag+pitem->len[1];
    BYTE * pflag=pdistance+pitem->len[2];
    BYTE * pflagExx=pflag+pitem->len[3];
    WORD * po=(WORD *)pout;
    const WORD * poend=(WORD *)(pout+pitem->inlen);
    const BYTE * piend=pdistanceflag+(pitem->len[1]-1);     //-1:last block。
    int halfid=0;
    BYTE c;
    DWORD matchdistance;
    while( pdistanceflag<piend )
    {
        c=*pdistanceflag++;         //iixxdddd
        matchdistance = c & 0x1f;
        if( c & 0x20 )
        {   //ii1xdddd
            int i= ((c>>4) & 3) -1;
            matchdistance = ((((DWORD *)pdistance)[0] & mark[i]) <<4) | (c & 0x0f);
            pdistance+=i;
        }
        if( c & 0x80 )
        {   //1ixxdddd
            if(c & 0x40)
            {   //11xxdddd          //pflag:mmmd,dddd
                int charlen=1;
                mylz_ReadLength44(pflagExx,halfid,charlen);
                //pflagExx==>charlen
                if( po+charlen > poend) return -3;      //goto errorExit3;
                
                mylz16_Copy64(po, pchar,charlen);
                pchar+=charlen;
            }
            //11xxdddd or 10xxdddd, //pflag: mmmddddd;
            matchdistance =  (matchdistance<<5) | (*pflag & 0x1f);
            int matchlen=( *pflag++ >>5 ) & 0x07;
            if(matchlen>=0x07)
            {
                mylz_ReadLength44(pflagExx,halfid,matchlen);
                //pflagExx==>matchlen.
            }
            mylzDE_Getpmatch(po,matchdistance,pout);
            matchlen+=2+1;
            if( po+matchlen > poend) return -3;
            
            if(matchdistance>3 )
            {
                mylz16_Copy64(po,pmatch,matchlen);
                continue;
            }
            else
            {
                //$$$$$$$$$$$$$$$$$$$$$$$$ the minimum sample size is used $$$$$$$$$$$$$$$$$$$$$$$$
                QWORD x=((QWORD *)pmatch)[0];
                if( matchdistance <=0 ) return -5;
                int n=1+matchlen/matchdistance;
                for(int i=0; i< n ; i++)
                {
                    ((QWORD *)po)[0]=x;
                    po+=matchdistance;
                }
                po-=n*matchdistance-matchlen;//matchdistance-(matchlen % matchdistance);
                //$$$$$$$$$$$$$$$$$$$$$$$$ the minimum sample size is used $$$$$$$$$$$$$$$$$$$$$$$$
              continue;
            }
        }
        else
        {
            if( c & 0x40 )
            {   //01xxdddd          //pflag:NULL
                int charlen=1;
                mylz_ReadLength44(pflagExx,halfid,charlen);
                if( po+charlen > poend) return -3;//goto errorExit3;
                //mylz16Copy(po, pchar,charlen);
                mylz16_Copy64(po, pchar,charlen);
                pchar+=charlen;
            }
            //01xxdddd or 00xxdddd   //pflag:NULL
            mylzDE_Getpmatch(po,matchdistance,pout);
            *po++=*pmatch++;*po++=*pmatch;
        }
    }
    //===================== last block: matchlen==0 =====================
    int charlen=*pflag++;
    if(charlen==15)
    {
        mylz_ReadLength(pflag,charlen);
    }
    if( (po+(charlen-1)) >= poend)  return -3;
    
    for(int i=0; i<charlen-1; i++)
    {
        *po++=*pchar++;
    }
    //++++++++++ source length is odd or even ++++++++++++
    ((BYTE *)po)[0]=((BYTE *)pchar)[0];
    charlen=(int)((BYTE *)po-pout)+1;
    if( (pitem->inlen & 1) ==0 )
    {
        ((BYTE *)po)[1]=((BYTE *)pchar)[1];
        charlen++;
    }
    //++++++++++ source length is odd or even ++++++++++++
    //===================== last block:  matchlen==0 =====================
    return charlen;
}
