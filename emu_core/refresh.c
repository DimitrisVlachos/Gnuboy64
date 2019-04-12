#include "gnuboy.h"
#include "defs.h"
#include "lcd.h"

#define BUF (scan.buf)

#ifdef USE_ASM
#include "asm.h"
#endif
 

#ifndef ASM_REFRESH_1
void refresh_1(byte *dest, byte *src, byte *pal, int cnt)
{
	while(cnt--) *(dest++) = pal[*(src++)];
}
#endif

#ifndef ASM_REFRESH_2
void refresh_2(un16 *dest, byte *src, un16 *pal, int cnt)
{
	while (cnt--) *(dest++) = pal[*(src++)];
}
#endif

#ifndef ASM_REFRESH_3
void refresh_3(byte *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c>>8;
		*(dest++) = c>>16;
	}
}
#endif

#ifndef ASM_REFRESH_4
 
void refresh_4(un32 *dest, byte *src, un32 *pal, int cnt)
{

	while (cnt >= 8) {
		dest[0] = pal[src[0]];
		dest[1] = pal[src[1]];
		dest[2] = pal[src[2]];
		dest[3] = pal[src[3]];
		dest[4] = pal[src[4]];
		dest[5] = pal[src[5]];
		dest[6] = pal[src[6]];
		dest[7] = pal[src[7]];

		dest = &dest[8];
		src += 8;
		cnt += -8;
	}

	while (cnt--) *(dest++) = pal[*(src++)];

}
#endif




#ifndef ASM_REFRESH_1_2X
void refresh_1_2x(byte *dest, byte *src, byte *pal, int cnt)
{
	byte c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
	}
}
#endif

#ifndef ASM_REFRESH_2_2X
void refresh_2_2x(un16 *dest, byte *src, un16 *pal, int cnt)
{
	un16 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
	}
}
#endif

#ifndef ASM_REFRESH_3_2X
void refresh_3_2x(byte *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		dest[0] = dest[3] = c;
		dest[1] = dest[4] = c>>8;
		dest[2] = dest[5] = c>>16;
		dest += 6;
	}
}
#endif

#ifndef ASM_REFRESH_4_2X
void refresh_4_2x(un32 *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
	}
}
#endif

#ifndef ASM_REFRESH_2_3X
void refresh_2_3x(un16 *dest, byte *src, un16 *pal, int cnt)
{
	un16 c;
#if 0
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
		*(dest++) = c;
	}
#else

	//fb is always aligned in the n64 port
	if (cnt >= 8) {
		int ccnt = cnt;
		register un32* ptr = (un32*)dest;
		un32 c1,c2,c3,c4;
		do {
			c1 = pal[src[0]];
			c2 = pal[src[1]];
			c3 = pal[src[2]];
			c4 = pal[src[3]];

			ptr[0] = (c1 << 16) | c1;  
			ptr[1] = (c1 << 16) | c2;  
			ptr[2] = (c2 << 16) | c2;
			ptr[3] = (c3 << 16) | c3;
			ptr[4] = (c3 << 16) | c4;
			ptr[5] = (c4 << 16) | c4;

			ptr = &ptr[6];
			src += 4;
			cnt += -4;
		} while (cnt >= 4);

		dest = &dest[ccnt - cnt];
	}

	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
		*(dest++) = c;
	}
#endif
}
#endif

#ifndef ASM_REFRESH_3_3X
void refresh_3_3x(byte *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		dest[0] = dest[3] = dest[6] = c;
		dest[1] = dest[4] = dest[7] = c>>8;
		dest[2] = dest[5] = dest[8] = c>>16;
		dest += 9;
	}
}
#endif

#ifndef ASM_REFRESH_4_3X
void refresh_4_3x(un32 *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
		*(dest++) = c;
	}
}
#endif

#ifndef ASM_REFRESH_3_4X
void refresh_3_4x(byte *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		dest[0] = dest[3] = dest[6] = dest[9] = c;
		dest[1] = dest[4] = dest[7] = dest[10] = c>>8;
		dest[2] = dest[5] = dest[8] = dest[11] = c>>16;
		dest += 12;
	}
}
#endif

#ifndef ASM_REFRESH_4_4X
void refresh_4_4x(un32 *dest, byte *src, un32 *pal, int cnt)
{
	un32 c;
	while (cnt--)
	{
		c = pal[*(src++)];
		*(dest++) = c;
		*(dest++) = c;
		*(dest++) = c;
		*(dest++) = c;
	}
}
#endif

