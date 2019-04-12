/*New faster implementations for HwDMA operations ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/


#include "mm.h"
#include "../umem/umem.h"
#include "../emu_core/gnuboy.h"
#include "../emu_core/defs.h"
#include "../emu_core/hw.h"
#include "../emu_core/regs.h"
#include "../emu_core/mem.h"
#include "../emu_core/rtc.h"
#include "../emu_core/lcd.h"
#include "../emu_core/sound.h"
extern struct mbc mbc;
extern struct rom rom;
extern struct ram ram;




void* mem_map_write_ptr(int a)
{
	int n;
	unsigned char ha = (a>>12) & 0xE;
	
	/* printf("write to 0x%04X: 0x%02X\n", a, b); */
	switch (ha)
	{
	case 0x0: 
	case 0x1: 
	case 0x2:
	case 0x4:
	case 0x6:
		return NULL;
	case 0x8:
		return NULL;
	case 0xA:
		if (!mbc.enableram) return NULL;
		if (rtc.sel&8)
		{
			 
			return NULL;
		}
		return &ram.sbank[mbc.rambank][a & 0x1FFF];
	case 0xC:
		if ((a & 0xF000) == 0xC000)
		{
			return &ram.ibank[0][a & 0x0FFF];
		}
		n = R_SVBK & 0x07;
		return &ram.ibank[n?n:1][a & 0x0FFF];
	case 0xE:
		if (a < 0xFE00)
		{
			return mem_map_write_ptr(a & 0xDFFF);
		}
		if ((a & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) break; */
			if (a < 0xFEA0) { return &lcd.oam.mem[a & 0xFF]; }
			return NULL;
		}
		/* return writehi(a & 0xFF, b); */
		if (a >= 0xFF10 && a <= 0xFF3F)
		{
			return NULL;
		}
		if ((a & 0xFF80) == 0xFF80 && a != 0xFFFF)
		{
			return &ram.hi[a & 0xFF];
		}

		return NULL;
	}

	return NULL;
}

void* mem_map_read_ptr(int a)
{
	int n ;
	unsigned char ha = (a>>12) & 0xE;
	
	/* printf("read %04x\n", a); */
	switch (ha)
	{
	case 0x0:
		return (un8*)&rom.bank[0][a]  ;
	case 0x1:return NULL; /* not reached */
	case 0x2:
		return (un8*)&rom.bank[0][a] ;
	case 0x4:
	case 0x6:
	 	return (un8*)&rom.bank[mbc.rombank][(a) & 0x3FFF] ;
	case 0x8:
 		return (un8*)&lcd.vbank[R_VBK&1][(a) & 0x1FFF] ;
	case 0xA:
		if (!mbc.enableram && mbc.type == MBC_HUC3){ return NULL; }
		if (!mbc.enableram) { return NULL; }
		if (rtc.sel&8) { return NULL; }
		return (un8*)&ram.sbank[mbc.rambank][(a) & 0x1FFF] ;
	case 0xC:
		if ((a & 0xF000) == 0xC000) {
 
			return (un8*)&ram.ibank[0][(a) & 0x0FFF] ;
		}
		n = R_SVBK & 0x07;
 
		return (un8*)&ram.ibank[n?n:1][(a) & 0x0FFF] ;
	case 0xE:
		if (a < 0xFE00) {
			return mem_map_read_ptr(a & 0xDFFF );
		}
		if ((a & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) return 0xFF; */
			if (a < 0xFEA0){  
				return &lcd.oam.mem[0];
			}

			return NULL;
		}
		/* return readhi(a & 0xFF); */
		if (a == 0xFFFF) {  return NULL; }
		if (a >= 0xFF10 && a <= 0xFF3F) {
			return NULL;
		}
		if ((a & 0xFF80) == 0xFF80) {
			return &ram.hi[0];
		}

		return NULL;
 
	}
	return NULL; /* not reached */
}

void mem_write_range(int  a,int cnt,const unsigned char* in) {
	int n,i;
	unsigned char ha = (a>>12) & 0xE;
	
	/* printf("write to 0x%04X: 0x%02X\n", a, b); */
	switch (ha)
	{
	case 0x0: for (i = 0;i < cnt;++i) { mbc_write(a++, *(in++)); } return;
	case 0x1: return;
	case 0x2:
	case 0x4:
	case 0x6:
		for (i = 0;i < cnt;++i) { mbc_write(a++, *(in++)); }
		return;
	case 0x8:
		/* if ((R_STAT & 0x03) == 0x03) break; */
#if 1
		vram_write_range(a& 0x1FFF,cnt,in);
#else
		for (i = 0;i < cnt;++i) { vram_write((a++) & 0x1FFF, *(in++)); }
#endif
		return;
	case 0xA:
		if (!mbc.enableram) return;
		if (rtc.sel&8)
		{
			for (i = 0;i < cnt;++i) { rtc_write( *(in++)); }
			return;
		}
		
		//ram.sbank[mbc.rambank][a & 0x1FFF] = b;
		umemcpy(&ram.sbank[mbc.rambank][a & 0x1FFF],in,cnt);
		return;
	case 0xC:
		if ((a & 0xF000) == 0xC000)
		{
			//ram.ibank[0][a & 0x0FFF] = b;
			umemcpy(&ram.ibank[0][a & 0x0FFF],in,cnt);
			return;
		}
		n = R_SVBK & 0x07;
		//ram.ibank[n?n:1][a & 0x0FFF] = b;
		umemcpy(&ram.ibank[n?n:1][a & 0x0FFF],in,cnt);
		return;
	case 0xE:
		if (a < 0xFE00)
		{
			mem_write_range(a & 0xDFFF,cnt,in);
			return;
		}
		if ((a & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) break; */
			if (a < 0xFEA0) {
				if (((a&0xff)+cnt) <= 256) {
					umemcpy(&lcd.oam.mem[a & 0xFF],in,cnt);
					return;
				}
				for (i = 0;i < cnt;++i) {
					lcd.oam.mem[(a++) & 0xFF] = *(in++);
				}
			}
			return;
		}
		/* return writehi(a & 0xFF, b); */
		if (a >= 0xFF10 && a <= 0xFF3F)
		{
			for (i = 0;i < cnt;++i) {
				sound_write((a++) & 0xFF, *(in++));
			}
			return;
		}
		if ((a & 0xFF80) == 0xFF80 && a != 0xFFFF)
		{
			if (((a&0xff)+cnt) <= 256) {
				umemcpy(&ram.hi[a & 0xFF],in,cnt);
				return;
			}
			for (i = 0;i < cnt;++i) {
				ram.hi[(a++) & 0xFF] = *(in++);
			}
			return;
		}

		for (i = 0;i < cnt;++i) {
			ioreg_write((a++) & 0xFF, *(in++));
		}
		return;
	}
}

void mem_read_range(int a,int cnt,unsigned char* out)
{
	int n,i;
	unsigned char ha = (a>>12) & 0xE;
	
	/* printf("read %04x\n", a); */
	switch (ha)
	{
	case 0x0:
		umemcpy(out,(un8*)&rom.bank[0][a],cnt);
	return;
	case 0x1:return ; /* not reached */
	case 0x2:
		umemcpy(out,(un8*)&rom.bank[0][a],cnt);
		return;
	case 0x4:
	case 0x6:
	 	umemcpy(out,(un8*)&rom.bank[mbc.rombank][(a) & 0x3FFF],cnt);
		return;
	case 0x8:
 		umemcpy(out,(un8*)&lcd.vbank[R_VBK&1][(a) & 0x1FFF],cnt);
		return;
	case 0xA:
		if (!mbc.enableram && mbc.type == MBC_HUC3){ umemset(out,0x01,cnt); return; }
		if (!mbc.enableram) { umemset(out,0xff,cnt); return; }
		if (rtc.sel&8) { umemset(out,rtc.regs[rtc.sel&7],cnt); return; }
		umemcpy(out,(un8*)&ram.sbank[mbc.rambank][(a) & 0x1FFF],cnt);
		return ;
	case 0xC:
		if ((a & 0xF000) == 0xC000) {
 
			umemcpy(out,(un8*)&ram.ibank[0][(a) & 0x0FFF],cnt);
			return ;
		}
		n = R_SVBK & 0x07;
 
		umemcpy(out,(un8*)&ram.ibank[n?n:1][(a) & 0x0FFF],cnt);
		return;
	case 0xE:
		if (a < 0xFE00) {
			mem_read_range(a & 0xDFFF,cnt,out);
			return;
		}
		if ((a & 0xFF00) == 0xFE00)
		{
			/* if (R_STAT & 0x02) return 0xFF; */
			if (a < 0xFEA0){  
	
				if (((a&0xff)+cnt) <= 256) {
					umemcpy(out,&lcd.oam.mem[0],cnt);
					return;	
				}
				register const un8* rd = lcd.oam.mem;
				for (i = 0;i < cnt;++i) {
					*(out++) = rd[(a++) & 0xFF]; 
				}
				return;
			}
			umemset(out,0xff,cnt);
			return;
		}
		/* return readhi(a & 0xFF); */
		if (a == 0xFFFF) {  umemset(out,REG(0xff),cnt); return; }
		if (a >= 0xFF10 && a <= 0xFF3F) {

			for (i = 0;i < cnt;++i) {
				*(out++) = sound_read((a++)&0xff );  
			}
			return;
		}
		if ((a & 0xFF80) == 0xFF80) {
			if (((a&0xff)+cnt) <= 256) {
				umemcpy(out,&ram.hi[0],cnt);
				return;	
			}
			register const un8* rd = ram.hi;
			for (i = 0;i < cnt;++i) {
				*(out++) = rd[(a++) & 0xFF];  
			}

			return ;
		}
			for (i = 0;i < cnt;++i) {
				*(out++) = ioreg_read((a++)&0xff);  
			}
			return;
 
	}
	return ; /* not reached */
}
 
