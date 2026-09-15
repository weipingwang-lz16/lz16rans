
#ifndef rans64_main_h
#define rans64_main_h

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "rans64.h"
//from: [ryg rans]main64.cpp

typedef uint64_t QWORD;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

typedef struct {
    uint32_t freqs[256];
    uint32_t cum_freqs[257]; //CDF[s]
}SSTATS,*PSSTATS;

static inline void count_freqs(uint8_t const* in, size_t nbytes,PSSTATS pss)
{
    for (int i=0; i < 256; i++)
        pss->freqs[i] = 0;

    for (size_t i=0; i < nbytes; i++)
        pss->freqs[in[i]]++;
}

static inline void calc_cum_freqs(PSSTATS pss)
{
    pss->cum_freqs[0] = 0;
    for (int i=0; i < 256; i++)
        pss->cum_freqs[i+1] = pss->cum_freqs[i] + pss->freqs[i];
}

static inline void normalize_freqs(uint32_t target_total,PSSTATS pss)
{
    assert(target_total >= 256);
    
    calc_cum_freqs(pss);
    uint32_t cur_total = pss->cum_freqs[256];
    
    // resample distribution based on cumulative freqs
    for (int i = 1; i <= 256; i++)
        pss->cum_freqs[i] = ((uint64_t)target_total * pss->cum_freqs[i])/cur_total;

    // if we nuked any non-0 frequency symbol to 0, we need to steal
    // the range to make the frequency nonzero from elsewhere.
    //
    // this is not at all optimal, i'm just doing the first thing that comes to mind.
    for (int i=0; i < 256; i++) {
        if (pss->freqs[i] && pss->cum_freqs[i+1] == pss->cum_freqs[i]) {
            // symbol i was set to zero freq

            // find best symbol to steal frequency from (try to steal from low-freq ones)
            uint32_t best_freq = ~0u;
            int best_steal = -1;
            for (int j=0; j < 256; j++) {
                uint32_t freq = pss->cum_freqs[j+1] - pss->cum_freqs[j];
                if (freq > 1 && freq < best_freq) {
                    best_freq = freq;
                    best_steal = j;
                }
            }
            assert(best_steal != -1);
            // and steal from it!
            if (best_steal < i) {
                for (int j = best_steal + 1; j <= i; j++)
                    pss->cum_freqs[j]--;
            } else {
                assert(best_steal > i);
                for (int j = i + 1; j <= best_steal; j++)
                    pss->cum_freqs[j]++;
            }
        }
    }
    // calculate updated freqs and make sure we didn't screw anything up
    assert(pss->cum_freqs[0] == 0 && pss->cum_freqs[256] == target_total);
    for (int i=0; i < 256; i++) {
        if (pss->freqs[i] == 0)
            assert(pss->cum_freqs[i+1] == pss->cum_freqs[i]);
        else
            assert(pss->cum_freqs[i+1] > pss->cum_freqs[i]);

        // calc updated freq
        pss->freqs[i] = pss->cum_freqs[i+1] - pss->cum_freqs[i];
    }
}
#endif
