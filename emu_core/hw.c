#include <string.h>

#include "gnuboy.h"
#include "defs.h"
#include "cpu.h"
#include "hw.h"
#include "regs.h"
#include "lcd.h"
#include "mem.h"
#include "fastmem.h"
#include <libdragon.h>
#include "../umem/umem.h"
#include "../mem_map/mm.h"
struct hw __attribute__ ((aligned (16))) hw;
un8 hw_buffer[1024] __attribute__ ((aligned (16)));

extern void div_advance(int cnt);
extern void timer_advance(int cnt);
extern void sound_advance(int cnt);
/*
 * hw_interrupt changes the virtual interrupt lines included in the
 * specified mask to the values the corresponding bits in i take, and
 * in doing so, raises the appropriate bit of R_IF for any interrupt
 * lines that transition from low to high.
 */

void hw_interrupt(byte i, byte mask)
{
	byte oldif = R_IF;
	i &= 0x1F & mask;
	R_IF |= i & (hw.ilines ^ i);

	/* FIXME - is this correct? not sure the docs understand... */
	if ((R_IF & (R_IF ^ oldif) & R_IE) && cpu.ime) cpu.halt = 0;
	/* if ((i & (hw.ilines ^ i) & R_IE) && cpu.ime) cpu.halt = 0; */
	/* if ((i & R_IE) && cpu.ime) cpu.halt = 0; */
	
	hw.ilines &= ~mask;
	hw.ilines |= i;
}


/*
 * hw_dma performs plain old memory-to-oam dma, the original dmg
 * dma. Although on the hardware it takes a good deal of time, the cpu
 * continues running during this mode of dma, so no special tricks to
 * stall the cpu are necessary.
 */

void hw_dma(byte b)
{
	
	addr a;

	a = ((addr)b) << 8;

	{
		//Faster version ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)
		const byte *p = mbc.rmap[a>>12];

		if (p) {
			const addr a2 = a + 160;
			//Simple hash
			if ((mbc.rmap[a2>>12]) && ((a>>12) == (a2>>12))) {
				register byte* w = lcd.oam.mem;
				register const byte* r = &p[a];
				register const byte* r2 = &p[a2];

				const un32 rm = (un32)r & 3 , wm = (un32)w & 3;
				if( (!rm) && (!wm) ) {
					for (;r < r2;r += 16,w += 16) {
						*(un32*)&w[0] = *(un32*)&r[0];
						*(un32*)&w[4] = *(un32*)&r[4];
						*(un32*)&w[8] = *(un32*)&r[8];
						*(un32*)&w[12] = *(un32*)&r[12];
					}
				} else {
					//160 / 16 reps
					for (;r < r2;r += 16,w += 16) {
						w[0] = r[0];
						w[1] = r[1];
						w[2] = r[2];
						w[3] = r[3];
						w[4] = r[4];
						w[5] = r[5];
						w[6] = r[6];
						w[7] = r[7];
						w[8] = r[8];
						w[9] = r[9];
						w[10] = r[10];
						w[11] = r[11];
						w[12] = r[12];
						w[13] = r[13];
						w[14] = r[14];
						w[15] = r[15];
					}
				}
	
				return;
			}
		}
		//Fallback but won't happen
	}

	{
		//Wont reach ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)
	 
		register byte* w = lcd.oam.mem;
		register byte* w2 = &w[160];
		for (;w < w2;w += 8) {
			w[0] = readb(a++);
			w[1] = readb(a++);
			w[2] = readb(a++);
			w[3] = readb(a++);
			w[4] = readb(a++);
			w[5] = readb(a++);
			w[6] = readb(a++);
			w[7] = readb(a++);
		}
	}
}

/* COMMENT A: NOTE this is partially superceded by COMMENT B below.
 * Beware was pretty sure that this HDMA implementation was incorrect, as when
 * he used it in bgb, it broke Pokemon Crystal (J). I tested it with this and
 * it seems to work fine, so until I find any problems with it, it's staying.
 * (Lord Nightmare)
 */


/* COMMENT B:
Timings for GDMA by /dalias/ by all means are accurate within few cycles, given
whatever we feed these values in takes a time to run expressed in double-speed
machine cycles as an argument. Whatever DMA-related issues remain, they are
likely in how intervals are applied and not in how they are calculated.

Had to replace cpu_timers() in GDMA section, don't know how but it was breaking
Shantae. Figured out the same thing happens to Pokemon Crystal and HDMA so I
also got HDMA init interval in place. Ghostbusters got fixed somehow somewhere
along the way.

Not sure how to be about now-missing lcd update. Note that DMA transfer now
completes immediately from LCDC perspective, which is far from accurate. Any
way we better get other bits straight to make sure they don't interfere before
trying to fix DMA any further.
*/


void hw_hdma()
{
	int cnt;
	addr sa;
	int da;

	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = 16;

	/*Faster version ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
	un8* direct_read_ptr = mem_map_read_ptr(sa);
	un8* direct_write_ptr =  mem_map_write_ptr(da);

	if (direct_read_ptr && direct_write_ptr) {
		umemcpy(direct_write_ptr,direct_read_ptr,cnt);
		sa += cnt;
		da += cnt;
	} else { //don't care for more opts here
		while (cnt--)
			writeb(da++, readb(sa++));
	}	
	/* SEE COMMENT B ABOVE */
	/* NOTE: lcdc_trans() after a call to hw_hdma() will advance lcdc for us */
	div_advance(16 << cpu.speed);
	timer_advance(16<< cpu.speed);
	sound_advance(16);
	
	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5--;
	hw.hdma--;
}


void hw_hdma_cmd(byte c)
{
	int cnt;
	addr sa;
	int da;
	
	int advance;

	/* Begin or cancel HDMA */
	if ((hw.hdma|c) & 0x80)
	{
		hw.hdma = c;
		R_HDMA5 = c & 0x7f;
		
		/* SEE COMMENT B ABOVE */
		advance = 460 >> cpu.speed;
		div_advance(advance << cpu.speed);
		timer_advance(advance << cpu.speed);
		sound_advance(advance);
		
		/* FIXME: according to docs, hdma should not be started during hblank
		(Extreme Ghostbusters game does, but it also works without this line) */
		if ((R_STAT&0x03) == 0x00) hw_hdma(); /* SEE COMMENT A ABOVE */
		return;
	}
	
	
	/* Perform GDMA */
	sa = ((addr)R_HDMA1 << 8) | (R_HDMA2&0xf0);
	da = 0x8000 | ((int)(R_HDMA3&0x1f) << 8) | (R_HDMA4&0xf0);
	cnt = ((int)c)+1;
	
	/* SEE COMMENT B ABOVE */
	/*cpu_timers((460>>cpu.speed)+cnt*16);*/ /*dalias*/
	advance = (460 >> cpu.speed) + (cnt << 4);
	div_advance(advance << cpu.speed);
	timer_advance(advance << cpu.speed);
	sound_advance(advance);
		
	cnt <<= 4;
 
#if 1
	/*Faster version ~ (Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
	

	un8* direct_read_ptr = mem_map_read_ptr(sa);
	un8* direct_write_ptr =  mem_map_write_ptr(da);

	if (direct_read_ptr && direct_write_ptr) {
		umemcpy(direct_write_ptr,direct_read_ptr,cnt);
		sa += cnt;
		da += cnt;
	} else if (direct_read_ptr) {
		sa += cnt;
		while (cnt--)
			writeb(da++, *(direct_read_ptr++));
	} else if (direct_write_ptr) {
		da += cnt;
		while (cnt--)
			*(direct_write_ptr++) = readb(sa++);
	} else {
		int i,j;
		for (i = 0;i < cnt;) {
			j = ((i + sizeof(hw_buffer)) > cnt) ? cnt - i : sizeof(hw_buffer);
			mem_read_range(sa,j,(hw_buffer));
	 		//data_cache_hit_writeback_invalidate(hw_buffer,sizeof(hw_buffer));
			mem_write_range(da,j,hw_buffer);
			sa += j;
			da += j;
			i += j;
		}
	}
#else
	while (cnt--)
		writeb(da++, readb(sa++));
#endif

	R_HDMA1 = sa >> 8;
	R_HDMA2 = sa & 0xF0;
	R_HDMA3 = 0x1F & (da >> 8);
	R_HDMA4 = da & 0xF0;
	R_HDMA5 = 0xFF;
}


/*
 * pad_refresh updates the P1 register from the pad states, generating
 * the appropriate interrupts (by quickly raising and lowering the
 * interrupt line) if a transition has been made.
 */

void pad_refresh()
{
	byte oldp1;
	oldp1 = R_P1;
	R_P1 &= 0x30;
	R_P1 |= 0xc0;
	if (!(R_P1 & 0x10))
		R_P1 |= (hw.pad & 0x0F);
	if (!(R_P1 & 0x20))
		R_P1 |= (hw.pad >> 4);
	R_P1 ^= 0x0F;
	if (oldp1 & ~R_P1 & 0x0F)
	{
		hw_interrupt(IF_PAD, IF_PAD);
		hw_interrupt(0, IF_PAD);
	}
}


/*
 * These simple functions just update the state of a button on the
 * pad.
 */

static byte pad_force_update = 0;

void pad_begin() {
	pad_force_update = 0;
}

void pad_end() {
	if (pad_force_update) {		
		pad_refresh();
		pad_force_update = 0;
	}
}

void pad_press(byte k)
{
	if (hw.pad & k)
		return;
	hw.pad |= k;
	pad_force_update = 1;//pad_refresh();
}

void pad_release(byte k)
{
	if (!(hw.pad & k))
		return;
	hw.pad &= ~k;
	pad_force_update = 1;//pad_refresh();
}

void pad_set(byte k, int st)
{
	st ? pad_press(k) : pad_release(k);
}

void hw_reset()
{
	hw.ilines = hw.pad = 0;

	memset(ram.hi, 0, sizeof ram.hi);

	R_P1 = 0xFF;
	R_LCDC = 0x91;
	R_BGP = 0xFC;
	R_OBP0 = 0xFF;
	R_OBP1 = 0xFF;
	R_SVBK = 0x01;
	R_HDMA5 = 0xFF;
	R_VBK = 0xFE;
}
