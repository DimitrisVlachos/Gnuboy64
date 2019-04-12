#include <string.h>
#include "../umem/umem.h"
#include "gnuboy.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "mem.h"
#include "lcd.h"
//#include "rc.h"
#include "fb.h"
#ifdef USE_ASM
#include "asm.h"
#endif
#include <libdragon.h>
// RSP DMA registers
#define DMA_LADDR (*(volatile uint32_t *)0xA4040000)
#define DMA_RADDR (*(volatile uint32_t *)0xA4040004)
#define DMA_RDLEN (*(volatile uint32_t *)0xA4040008)
#define DMA_WRLEN (*(volatile uint32_t *)0xA404000C)
#define DMA_FULL  (*(volatile uint32_t *)0xA4040014)
#define DMA_BUSY  (*(volatile uint32_t *)0xA4040018)

#include "../emu_asm/lcd_mips.h"		/*MIPS asm opts ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
#include "../lcd_tables/lcd_upix_flags.h" /*Precomputed flags - (Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
#include "../lcd_tables/lcd_pal4.h" /*Use precomputed r8g8b8&r5g5b5a1 pal    - (Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
#include "../lcd_tables/u32_mask.h"
#include "../lcd_tables/lcd_cgb_attrs.h" /*256x256x2xsizeof(int) Precomputed attrs */
#include "../lcd_tables/lcd_obj_flags.h"

struct sprite_indice_t {	 
	int index;
	int x;
};

struct lcd   __attribute__ ((aligned (16))) lcd;
struct scan   __attribute__ ((aligned (16))) scan;

//static struct sprite_indice_t __attribute__ ((aligned (16))) sprite_sort_buffer[48];
#define BG (scan.bg)
#define WND (scan.wnd)
#define BUF (scan.buf)
#define PRI (scan.pri)

#define PAL1 (scan.pal1)
#define PAL2 (scan.pal2)
#define PAL4 (scan.pal4)

#define VS (scan.vs) /* vissprites */
#define NS (scan.ns)

#define L (scan.l) /* line */
#define X (scan.x) /* screen position */
#define Y (scan.y)
#define S (scan.s) /* tilemap position */
#define T (scan.t)
#define U (scan.u) /* position within tile */
#define V (scan.v)
#define WX (scan.wx)
#define WY (scan.wy)
#define WT (scan.wt)
#define WV (scan.wv)

byte __attribute__ ((aligned (16))) bgdup[256];
byte  patpix[4096][8][8] __attribute__ ((aligned (16)));
byte  patdirty[1024];

/*Improved vram updates ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))*/
unsigned int vram_list_ptr = 0;
unsigned short  vram_list[1024] __attribute__ ((aligned (16)));
byte *s_vdest;
byte *vdest;
uint32_t vdest_stride = 0;

int lcd_scale = 1;
const int density = 1;
const int rgb332 = 0;
const int sprsort = 1;

static const int wraptable[64]  __attribute__ ((aligned (16)))  =
{
			0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,		
			0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,-32
};

#define DEF_PAL { 0x98d0e0, 0x68a0b0, 0x60707C, 0x2C3C3C }

int dmg_pal[4][4] = { DEF_PAL, DEF_PAL, DEF_PAL, DEF_PAL };

const int usefilter=0, filterdmg;
const  int filter[3][4] = {
	{ 195,  25,   0,  35 },
	{  25, 170,  25,  35 },
	{  25,  60, 125,  40 }
};


 
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
 

void updatepatpix()
{

	
	unsigned int i, j, k;
	unsigned int a/*, c*/;
	byte *vram = lcd.vbank[0];
	 
 
 
#if 0
	if (!anydirty) return;
	for (i = 0; i < 1024; i++)
	{
		if (i == 384) i = 512;
		if (i == 896) break;
		if (!patdirty[i]) continue;
		patdirty[i] = 0;
		for (j = 0; j < 8; j++)
		{
			a = ((i<<4) | (j<<1));
			for (k = 0; k < 8; k++)
			{
				c = vram[a] & (1<<k) ? 1 : 0;
				c |= vram[a+1] & (1<<k) ? 2 : 0;
				patpix[i+1024][j][k] = c;
			}
			for (k = 0; k < 8; k++)
				patpix[i][j][k] =
					patpix[i+1024][j][7-k];
		}
		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 8; k++)
			{
				patpix[i+2048][j][k] =
					patpix[i][7-j][k];
				patpix[i+3072][j][k] =
					patpix[i+1024][7-j][k];
			}
		}
	}
	anydirty = 0;
#else


	/*
		My vram indice lock list implementation ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))
	*/

	if (0 == vram_list_ptr) return;

	//asm version
	
	lcd_flush_vram_list_mips(vram_list_ptr,vram);
	vram_list_ptr = 0;

#endif
	

}
 


static void memcpy2(void* d,const void* s,int cnt) {
	register unsigned char* pd = d;
	register const unsigned char* ps = s,*pse = ps + cnt;

	while ((ps + 8) <= pse) {
		MEMCPY8(pd,ps);
		pd += 8;
		ps += 8;
	}

	while (ps < pse) {
		*(pd++) = *(ps++);
	}
}

static void memset2(void* d,const byte v,int cnt) {
	register unsigned char* pd = d,*pde = pd + cnt;
	register const byte w = v;

	while ((pd + 8) <= pde) {
		MEMSET8(pd,w);
		pd += 8;
	}

	while (pd < pde) {
		*(pd++) = v;
	}
}



void tilebuf()
{
	register int i, cnt;
	int base;
	register byte *tilemap, *attrmap;
	register int *tilebuf;
	register const int *wrap;


	base = ((R_LCDC&0x08)?0x1C00:0x1800) + (T<<5) + S;
	tilemap = lcd.vbank[0] + base;
	attrmap = lcd.vbank[1] + base;
	tilebuf = BG;
	wrap = wraptable + S;
	cnt = ((WX + 7) >> 3) + 1;

	

	if ((hw.cgb) && (cnt > 0))
	{
		if (R_LCDC & 0x10) {
 
			lcd_tilebuf_gbc_pass1_mips(tilemap,attrmap,tilebuf,cnt,lcd_cgb_tilemap_attrs_utable,wrap);
 
		}
		else {
 
			lcd_tilebuf_gbc_pass1_mips(tilemap,attrmap,tilebuf,cnt,lcd_cgb_tilemap_attrs2_utable,wrap);
 
		}
	}
	else if (cnt > 0)
	{
		if (R_LCDC & 0x10) {
 
			lcd_tilebuf_gb_pass1_mips( tilemap, tilebuf, cnt,wrap);
 
		}
		else	{
 
				lcd_tilebuf_gb_pass1_stride_mips( tilemap, tilebuf, cnt,  wrap);
 
		}
	}

	if (WX >= 160) return;
	
	base = ((R_LCDC&0x40)?0x1C00:0x1800) + (WT<<5);
	tilemap = lcd.vbank[0] + base;
	attrmap = lcd.vbank[1] + base;
	tilebuf = WND;
	cnt = ((160 - WX) >> 3) + 1;

	if (hw.cgb)
	{
		
		if (R_LCDC & 0x10) {
			if (cnt > 0) {
				lcd_tilebuf_gbc_pass2_mips(tilemap,attrmap,tilebuf,cnt,lcd_cgb_tilemap_attrs_utable);
			}
		}
		else {
			if (cnt > 0) {
				lcd_tilebuf_gbc_pass2_mips(tilemap,attrmap,tilebuf,cnt,lcd_cgb_tilemap_attrs2_utable);
			}
		}
	}
	else
	{
		if (R_LCDC & 0x10) {
 
			if (cnt>0) {
				lcd_tilebuf_gb_pass2_mips(tilemap,tilebuf,cnt);
			}
 
		}
		else {
 
			if(cnt>0) {
				lcd_tilebuf_gb_pass2_stride_mips(tilemap,tilebuf,cnt);
			}
 
		}
	}
}


void bg_scan()
{
	int cnt;
	byte *src, *dest;
	int *tile;

	if (WX <= 0) return;
	cnt = WX;
	tile = BG;
	dest = BUF;
	
	src = patpix[*(tile++)][V] + U;
	memcpy2(dest, src, 8-U);
	dest += 8-U;
	cnt -= 8-U;
 
	if (cnt>0) {
		lcd_cpy_block_mips(dest,tile,V,cnt);
	}
 
}

void wnd_scan()
{
	int cnt;
	byte *src, *dest;
	int *tile;

	if (WX >= 160) return;
	cnt = 160 - WX;
	tile = WND;
	dest = BUF + WX;
	
 
	if (cnt>0) {
 
		lcd_cpy_block_mips(dest,tile,WV,cnt);
	}
 
}
 

static inline void blendcpy(byte *dest, byte *src, byte b,int cnt)
{
#if 0
	while (cnt--) *(dest++) = *(src++) | b;
#else
	byte* dest2 = dest + cnt;
	while (dest < dest2) {
		*(dest++) = *(src++) | b;
	}
#endif
}




static inline  int priused(void *attr)
{
	un32 *a = attr;
	return (int)((a[0]|a[1]|a[2]|a[3]|a[4]|a[5]|a[6]|a[7])&0x80808080);
}

#if 0
static void zero_buf_dma(byte* dest,int cnt,int blocking_f) {
	static unsigned short buffer_to_dmem = 0;

	if (!buffer_to_dmem) {
		static const byte z_buffer[256] = {0};
		buffer_to_dmem = 1;
 
		while (DMA_FULL) {}
		DMA_LADDR = 0x0000;
		DMA_RADDR = (un32)&z_buffer[0];
		DMA_RDLEN = 256 - 1;
		while (DMA_BUSY) {}
	}


	while (DMA_FULL) {}  
    DMA_LADDR = 0x0000;
    DMA_RADDR = (un32)UncachedAddr(dest);
    DMA_WRLEN = (cnt + 8) & (~7);

	if (blocking_f) { while (DMA_BUSY) {} } 
}
#endif

 

void bg_scan_pri()
{
	register int cnt, i;
	register byte *src, *dest;

	if (WX <= 0) return;
	i = S;
	cnt = WX;
	dest = PRI;
	src = lcd.vbank[1] + ((R_LCDC&0x08)?0x1C00:0x1800) + (T<<5);

	
	if (!priused(src))
	{
		umemset(dest,0,cnt);
		return;
	}
 
	
	memset2(dest, src[i++&31]&128, 8-U);
	dest += 8-U;
	cnt -= 8-U;
	
 
	if (cnt <= 0) return;
	if (cnt >= 8) {
		if (((un32)dest)&3) {
			do {
				const byte tmp = src[i++&31]&128;
				MEMSET8(dest,tmp);
				dest += 8;
				cnt += -8;
			}while (cnt >= 8);
		} else {
			do {
				const byte tmp = src[i++&31]&128;
				const unsigned int v32 = u32_mask_table[tmp];
				*(unsigned int*)&dest[0] = v32;
				*(unsigned int*)&dest[4] = v32;
				dest += 8;
				cnt += -8;
			}while (cnt >= 8);
		}
	}


	if (0 == cnt) return;

	const byte rv = src[i&31]&128;
	while (cnt--) {
			*(dest++) = rv;
	}
 

}

void wnd_scan_pri()
{
	register int cnt;
	register byte *src, *dest;

	if (WX >= 160) return;
	cnt = 160 - WX;
	dest = PRI + WX;
	src = lcd.vbank[1] + ((R_LCDC&0x40)?0x1C00:0x1800) + (WT<<5);
	
	if (!priused(src))
	{
		 umemset(dest,0,cnt);
		return;
	}
 
 
	if (cnt >= 8) {
		if (((un32)dest)&3) {
			do {
				const byte tmp = *(src++)&128;
				MEMSET8(dest,tmp);
				dest += 8;
				cnt += -8;
			}while (cnt >= 8);
		} else {
			do {
				const byte tmp = *(src++)&128;
				const unsigned int v32 = u32_mask_table[tmp];
				*(unsigned int*)&dest[0] = v32;
				*(unsigned int*)&dest[4] = v32;
				dest += 8;
				cnt += -8;
			}while (cnt >= 8);
		}
	}

	if (0 == cnt) return;


	const byte rv = *(src)&128;
	while (cnt--) {
			*(dest++) = rv;
	}
}
 

void bg_scan_color()
{
	int cnt;
	byte *src, *dest;
	int *tile;

	if (WX <= 0) return;
	cnt = WX;
	tile = BG;
	dest = BUF;
	
	src = patpix[*(tile++)][V] + U;
	blendcpy(dest, src, *(tile++), 8-U);
	dest += 8-U;
	cnt -= 8-U;	
 
	if (cnt<1){return;}
	lcd_blendcpy_block_mips(dest,tile,V,cnt);
 
}
 

void wnd_scan_color()
{
	int cnt;
	byte *src, *dest;
	int *tile;

	if (WX >= 160) return;
	cnt = 160 - WX;
	tile = WND;
	dest = BUF + WX;

	if (cnt<1){return;}
	lcd_blendcpy_block_mips(dest,tile,WV,cnt);
}


void spr_count() /*UNUSED*/
{
	int i;
	struct obj *o;
	
	NS = 0;
	if (!(R_LCDC & 0x02)) return;

	o = lcd.oam.obj;

	for (i = 40; i; i--, o++)
	{
		if (L >= o->y || L + 16 < o->y)
			continue;
		if (L + 8 >= o->y && !(R_LCDC & 0x04))
			continue;
		if (++NS == 10) break;
	}

}

static int cmp_qs(const void* a, const void* b) {
	const struct vissprite* pa = (const struct vissprite*)a;
	const struct vissprite* pb = (const struct vissprite*)b;

	if (pa->x > pb->x) {
		return 1;
	} else if (pa->x == pb->x) {
		return 0;
	}
	return -1;
}

 
void spr_enum()
{
	//int i, j;
	//register struct obj *o;
	//struct vissprite ts[10];
	//register int v, pat;
	//int l, x;

	NS = 0;
	if (!(R_LCDC & 0x02)) return;

	//o = lcd.oam.obj;
 
#if 0
	for (i = 40; i; i--, o++)
	{
		if (L >= o->y || L + 16 < o->y)
			continue;
		if (L + 8 >= o->y && !(R_LCDC & 0x04))  
			continue;
		VS[NS].x = (int)o->x - 8;
		v = L - (int)o->y + 16;
		if (hw.cgb)
		{
			pat = o->pat | (((int)o->flags & 0x60) << 5)
				| (((int)o->flags & 0x08) << 6);
			VS[NS].pal = 32 + ((o->flags & 0x07) << 2);
		}
		else
		{
			pat = o->pat | (((int)o->flags & 0x60) << 5);
			VS[NS].pal = 32 + ((o->flags & 0x10) >> 2);
		}
		VS[NS].pri = (o->flags & 0x80) >> 7;
		if ((R_LCDC & 0x04)) 
		{
			pat &= ~1;
			if (v >= 8)
			{
				v -= 8;
				pat++;
			}
			if (o->flags & 0x40) pat ^= 1;
		}
		VS[NS].buf = patpix[pat][v];
		if (++NS == 10) break;
	}
#else
		/*My version ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
		const int * lut_base = (hw.cgb) ? lcd_gb_cgb_obj_flags_table : &lcd_gb_cgb_obj_flags_table[(256 * 3)];
		if (R_LCDC & 0x04) { //Different cases to eliminate some branches
			NS = lcd_spr_enum_mips_R_LCDC_case(lcd.oam.obj,VS,lut_base,L);
		} else {
			NS = lcd_spr_enum_mips(lcd.oam.obj,VS,lut_base,L);
		}
#endif
	if (!sprsort || hw.cgb) return;

	
#if 0
	/* not quite optimal but it finally works!  */
	for (i = 0; (i < NS) ; i++)
	{
		l = 0;
		x = VS[0].x;
		for (j = 1; j < NS; j++)
		{
			if (VS[j].x < x)
			{
				l = j;
				x = VS[j].x;
			}
		}
		
		ts[i] = VS[l];
		VS[l].x = 160;
	}

	memcpy2(VS, ts, sizeof VS);
#else
	/*Improved sorting ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
	extern void stack_based_quicksort (void *const pbase, size_t total_elems, size_t size,size_t stride,
	   int (*cmp)(const void*,const void*));

	if (0 == NS) { return; }
#if 0
	for (i = 0,j = NS;i < j;++i) {
		sprite_sort_buffer[i].x = VS[i].x;
		sprite_sort_buffer[i].index = i;
	}
	stack_based_quicksort (sprite_sort_buffer,NS,sizeof(sprite_sort_buffer[0]),cmp_qs2);
	for (i = 0,j = NS;i < j;++i) {
		VS[i] = VS[sprite_sort_buffer[i].index];
	}
#else
	/*
		struct __attribute__ ((aligned (16)))  vissprite
		{
			byte *buf;	:0
			int x;		:4
			byte pal : 8, pri : 9,pad[6]; 
		};
	*/
	 stack_based_quicksort (VS,NS,sizeof(VS[0]),10,cmp_qs);
#endif
#endif
}
 

void spr_scan()
{
	int i, x;
	byte pal, b, ns = NS;
	byte *src, *dest, *bg, *pri=0;
	register struct vissprite *vs;
	
 
	extern void util_cpy_lcd_buf_mips(void* dst,const void* src) ;
 

	if (!ns) return;
  
	int bg_cpy = 0;

	

	vs = &VS[ns-1];
	
	for (; ns; ns--, vs--)
	{
		x = vs->x;
		if (x >= 160) continue;
		if (x <= -8) continue;
		if (x < 0)
		{
			src = vs->buf - x;
			dest = BUF;
			i = 8 + x;
		}
		else
		{
			src = vs->buf;
			dest = BUF + x;
			if (x > 152) i = 160 - x;
			else i = 8;
		}
		pal = vs->pal;
		if (vs->pri)
		{
			if (!bg_cpy) { util_cpy_lcd_buf_mips(bgdup,BUF); bg_cpy = 1;}
			bg = bgdup + (dest - BUF);
			lcd_scan_color_pri_mips(dest,src,pri,bg,i,pal);
		}
		else if (hw.cgb)
		{
			if (!bg_cpy) { util_cpy_lcd_buf_mips(bgdup,BUF); bg_cpy = 1;}
			bg = bgdup + (dest - BUF);
			pri = PRI + (dest - BUF);
			lcd_scan_color_cgb_mips(dest,src,pri,bg,i,pal);
		}
		else {
 
			//Simple..
			register byte* pa= dest;
			register const byte* pb = &src[0],*pc=&src[i];
			if (pb < pc) {
				do {
					b = *pb;
					if (b) { *pa = pal | b;}	
					++pb,++pa;
				} while (pb < pc);
			}
		}
	}
}





void lcd_init() {
	
 

		while (lcd_scale * 160 > fb.w || lcd_scale * 144 > fb.h) {lcd_scale--;}

		s_vdest = fb.ptr + ((fb.w*fb.pelsize)>>1)
			- (80*fb.pelsize) * lcd_scale
			+ ((fb.h>>1) - 72*lcd_scale) * fb.pitch;
 
	vdest_stride = fb.pitch * lcd_scale;
	lcd_begin();
}

void lcd_begin()
{
	extern void lcd_new_pass();
	lcd_new_pass();
	vdest = s_vdest;
	WY = R_WY;
}

void lcd_refreshline()
{
	 
	int i;
	byte*dest;
	 /*byte scalebuf[160*4*4], *dest;*/
	
	if (!fb.enabled) return;
	
	if (!(R_LCDC & 0x80))
		return; /* should not happen... */
	
	updatepatpix();

	L = R_LY;
	X = R_SCX;
	Y = (R_SCY + L) & 0xff;
	S = X >> 3;
	T = Y >> 3;
	U = X & 7;
	V = Y & 7;
	
	WX = R_WX - 7;
	if (WY>L || WY<0 || WY>143 || WX<-7 || WX>159 || !(R_LCDC&0x20))
		WX = 160;
	WT = (L - WY) >> 3;
	WV = (L - WY) & 7;

	spr_enum();

	tilebuf();
	if (hw.cgb)
	{
		bg_scan_color();
		wnd_scan_color();
		if (NS)
		{
			bg_scan_pri();
			wnd_scan_pri();
		}
	}
	else
	{
		bg_scan();
		wnd_scan();
 
		lcd_recolor_mips(BUF+WX, 0x04, 160-WX);
	}
	spr_scan();


	dest = vdest;
 	extern void lcd_refresh_line_2_pal4_mips(void* dest,void* src,void* pal) ;
	switch (lcd_scale)
	{
	case 0:
	case 1:
		switch (fb.pelsize)
		{
		case 1:
			refresh_1(dest, BUF, PAL1, 160);
			break;
		case 2:
			//refresh_2((un16*)dest, BUF, PAL2, 160);
			
			lcd_refresh_line_2_mips(dest,BUF,PAL2);
			break;
		case 3:
			refresh_3(dest, BUF, PAL4, 160);
			break;
		case 4:
			refresh_4((un32*)dest, BUF, PAL4, 160);
			break;
		}
		break;
	case 2:
		switch (fb.pelsize)
		{
		case 1:
			refresh_2((un16*)dest, BUF, PAL2, 160);
			break;
		case 2:
			lcd_refresh_line_4_mips(dest,BUF,PAL4);
			//refresh_4((un32*)dest, BUF, PAL4, 160);
			break;
		case 3:
			refresh_3_2x(dest, BUF, PAL4, 160);
			break;
		case 4:
			refresh_4_2x((un32*)dest, BUF, PAL4, 160);
			break;
		}
		break;
	case 3:
		switch (fb.pelsize)
		{
		case 1:
			refresh_3(dest, BUF, PAL4, 160);
			break;
		case 2:
			refresh_2_3x((un16*)dest, BUF, PAL2, 160);
			break;
		case 3:
			refresh_3_3x(dest, BUF, PAL4, 160);
			break;
		case 4:
			refresh_4_3x((un32*)dest, BUF, PAL4, 160);
			break;
		}
		break;
	case 4:
		switch (fb.pelsize)
		{
		case 1:
			refresh_4((un32*)dest, BUF, PAL4, 160);
			break;
		case 2:
			refresh_4_2x((un32*)dest, BUF, PAL4, 160);
			break;
		case 3:
			refresh_3_4x(dest, BUF, PAL4, 160);
			break;
		case 4:
			refresh_4_4x((un32*)dest, BUF, PAL4, 160);
			break;
		}
		break;
	default:
		break;
	}
 
	vdest += vdest_stride;
 
}

static inline void updatepalette(int i)
{
	int c;// , r, g, b , y, u, v, rr, gg;

	c = (lcd.pal[i<<1] | ((int)lcd.pal[(i<<1)|1] << 8)) & 0x7FFF;
	
	if (fb.pelsize==4) {
		PAL4[i] = lcd_pal_r8g8b8_components[c]; /*Use precomputed r8g8b8 pal4 - (Dimitrios Vlachos : https://github.com/DimitrisVlachos) ;D*/
		return;
	}
	c = lcd_pal_rdp_components[c];
	PAL4[i] = (c  << 16) | c;
	PAL2[i] = c;
}

void pal_write(int i, byte b)
{
	if (lcd.pal[i] == b) return;
	lcd.pal[i] = b;
	updatepalette(i>>1);
}

void pal_write_dmg(int i, int mapnum, byte d)
{
	int j;
	int *cmap = dmg_pal[mapnum];
	int c, r, g, b;

	if (hw.cgb) return;

	/* if (mapnum >= 2) d = 0xe4; */
	for (j = 0; j < 8; j += 2)
	{
		c = cmap[(d >> j) & 3];
		r = (c & 0xf8) >> 3;
		g = (c & 0xf800) >> 6;
		b = (c & 0xf80000) >> 9;
		c = r|g|b;
		/* FIXME - handle directly without faking cgb */
		pal_write(i+j, c & 0xff);
		pal_write(i+j+1, c >> 8);
	}
}

void vram_write(int a, byte b)
{
	lcd.vbank[R_VBK&1][a] = b;
	if (a >= 0x1800) return;

	/*Add entry to list ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))*/
	const unsigned int offs = ((R_VBK&1)<<9)+(a>>4);
 
	if (patdirty[offs] == 0) {
		vram_list[vram_list_ptr++] = (unsigned short)offs;
		patdirty[offs] = 1;
	}
 
}

void vram_write_range(int a,int cnt,const byte* data)
{
	/*Add entry to list ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))*/
	int i;
	for (i = 0;i < cnt;++i,++a) {
		
		lcd.vbank[R_VBK&1][a] = *(data++);
		if (a >= 0x1800) return;
		const unsigned int offs = ((R_VBK&1)<<9)+(a>>4);
	 
		if (patdirty[offs] == 0) {
			vram_list[vram_list_ptr++] = (unsigned short)offs;
			patdirty[offs] = 1;
		}
	}
 
}

void vram_dirty()
{	
	unsigned int i;

	/*Initialize the new vram indice list ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))*/
	for (i = 0;i < 384;++i) {
		vram_list[vram_list_ptr++] = i;
		patdirty[i] = 1;
	}

	for (i = 384;i < 512;++i) {
		patdirty[i] = 1;
	}

	for (i = 512;i < 896;++i) {
		vram_list[vram_list_ptr++] = i;
		patdirty[i] = 1;
	}

	for (i = 896;i < 1024;++i) {
		patdirty[i] = 1;
	}
}

void pal_dirty()
{
	int i;
	if (!hw.cgb)
	{
		pal_write_dmg(0, 0, R_BGP);
		pal_write_dmg(8, 1, R_BGP);
		pal_write_dmg(64, 2, R_OBP0);
		pal_write_dmg(72, 3, R_OBP1);
	}
	for (i = 0; i < 64; i++)
		updatepalette(i);
}

void lcd_reset()
{
	memset(&lcd, 0, sizeof lcd);
	lcd_begin();
	vram_dirty();
	pal_dirty();
}
