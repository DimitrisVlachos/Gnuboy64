#ifndef _neo2_asm_
#define _neo2_asm_

extern void neo_xferto_psram(void *src, int pstart, int len);
extern void neo_game_xferto_psram(void *src, int pstart, int len,int swap);
extern int neo2_recv_sd_multi(unsigned char *buf, int count);
extern void neo_memcpy64(void* destination,const void* source,unsigned int size);
extern void neo_memcpy32(void* destination,const void* source,unsigned int size);
extern void neo_memcpy16(void* destination,const void* source,unsigned int size);
#endif

