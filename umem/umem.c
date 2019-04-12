/*Some mem copy hacks ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
#include "umem.h"
#include <stdint.h>
#include "../lcd_tables/u32_mask.h"
#ifndef ptrdiff_t
#define ptrdiff_t unsigned int
#endif

#define MEMCPY8(d, s) {\
	(d)[0] = (s)[0];\
	(d)[1] = (s)[1];\
	(d)[2] = (s)[2];\
	(d)[3] = (s)[3];\
	(d)[4] = (s)[4];\
	(d)[5] = (s)[5];\
	(d)[6] = (s)[6];\
	(d)[7] = (s)[7];\
}
 

#define MEMSET8(d,_v_) {\
	(d)[0] = (_v_);\
	(d)[1] = (_v_);\
	(d)[2] = (_v_);\
	(d)[3] = (_v_);\
	(d)[4] = (_v_);\
	(d)[5] = (_v_);\
	(d)[6] = (_v_);\
	(d)[7] = (_v_);\
}

void umemcpy(void* d,const void* s,int cnt) {
	register unsigned char* pd = d;
	register const unsigned char* ps = s,*pse = ps + cnt;
	
	if ((cnt >= 8) && (! (((ptrdiff_t)pd) & 3) ) && (! (((ptrdiff_t)ps) & 3) ) ) { /*Src & dst aligned*/
		for (;(ps + 8) <= pse; pd += 8,ps += 8) {
			*(unsigned int*)&pd[0] = *(unsigned int*)&ps[0];
			*(unsigned int*)&pd[4] = *(unsigned int*)&ps[4];
		}
	} else if ((cnt >= 8) && (! (((ptrdiff_t)pd) & 3) ) ) {/*Dst aligned*/
		for (;(ps + 8) <= pse; pd += 8,ps += 8) {
			*(unsigned int*)&pd[0] = ((unsigned int)ps[0] << 24) | ((unsigned int)ps[1] << 16) | 
									((unsigned int)ps[2] << 8) | ((unsigned int)ps[3]);
			*(unsigned int*)&pd[4] = ((unsigned int)ps[4] << 24) | ((unsigned int)ps[5] << 16) | 
									((unsigned int)ps[6] << 8) | ((unsigned int)ps[7]);
		}
	} else { /*Don't care*/
		for (;(ps + 8) <= pse; pd += 8,ps += 8) {
			MEMCPY8(pd,ps);
		}
	}

	while (ps < pse) {
		*(pd++) = *(ps++);
	}
}

void umemset(void* d,const unsigned char v,int cnt) {
	register unsigned char* pd = d,*pde = pd + cnt;
	register const unsigned char w = v;

	if (cnt >= 16) {
		if ( (((ptrdiff_t)pd) & 3) ) {
			unsigned char* pdc = (unsigned char*)((ptrdiff_t)pd + (4 - ((ptrdiff_t)pd&3)));
			int a = (ptrdiff_t)pdc - (ptrdiff_t)pd;
			cnt -= a;
			while (a--) {*(pd++) = w; }
			pde = pd + cnt;
		}
	
		register const unsigned int val = u32_mask_table[w];
			for (;pd + 16 <= pde;pd += 16) {
				*(unsigned int*)&pd[0] = val;
				*(unsigned int*)&pd[4] = val;
				*(unsigned int*)&pd[8] = val;
				*(unsigned int*)&pd[12] = val;
			}
			for (;pd + 8 <= pde;pd += 8) {
				*(unsigned int*)&pd[0] = val;
				*(unsigned int*)&pd[4] = val;
			}
			while (pd < pde) {
				*(pd++)=w;
			}
		return;
	}

	while ((pd + 8) <= pde) {
		MEMSET8(pd,w);
		pd += 8;
	}
	
	while (pd < pde) {
		*(pd++)=w;
	}
}



