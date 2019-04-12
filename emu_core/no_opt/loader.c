#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "../gnuboy.h"
#include "../defs.h"
#include "../regs.h"
#include "../mem.h"
#include "../hw.h"
#include "../lcd.h"
#include "../rtc.h"
#include "../../ctl.h"
//#include "rc.h"
#include "../sound.h"
#include <libdragon.h>

static const byte mbc_table[256] =
{
	0, 1, 1, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 0, 0, 0, 0, 0, 5, 5, 5, MBC_RUMBLE, MBC_RUMBLE, MBC_RUMBLE, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, MBC_HUC3, MBC_HUC1
};

static const byte rtc_table[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static const byte batt_table[256] =
{
	0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	0
};

static const short romsize_table[256] =
{
	2, 4, 8, 16, 32, 64, 128, 256, 512,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 128, 128, 128
	/* 0, 0, 72, 80, 96  -- actual values but bad to use these! */
};

static const byte ramsize_table[256] =
{
	1, 1, 1, 4, 16,
	4 /* FIXME - what value should this be?! */
};




#if 0
static char *romfile=NULL;
static char *sramfile=NULL;
static char *rtcfile=NULL;
static char *saveprefix=NULL;
static char *savename=NULL;
static char* savedir = NULL;
#else
//Reduce heap fragmention for the port
#include "../../media.h"
#define MAX_ROM_PATH (MAX_MEDIA_DESC_LEN * 2)
 
static char sramfile[MAX_ROM_PATH];
static char rtcfile[MAX_ROM_PATH];
static char saveprefix[MAX_ROM_PATH];
static const char *savedir="/gnuboy/saves";
#endif


//static int saveslot=0;

static const int forcebatt=0, nobatt=0;
static const int forcedmg=0, gbamode=0;

static const int memfill = 0, memrand = -1;

static void initmem(void *mem, int size)
{
	char *p =(mem);
	if (memrand >= 0)
	{
		srand(memrand ? memrand : time(0));
		while(size--) *(p++) = rand();
	}
	else if (memfill >= 0)
		memset(p, memfill, size);
}

#if 0
static byte *loadfile(fs_handle_t* f, int *len)
{
	int l = 0, c = 0;
	byte *d = NULL;

	fs_seek(f, 0, FS_SEEK_END);
	l = fs_tell(f);

	fs_seek(f, 0, FS_SEEK_SET);
	d = (byte*) malloc(l);
	if (d != NULL)
	{
		c = fs_read(f,(void *) d, (size_t) l);

	}

	*len = l;
	return d;
}
 #endif

//For the n64 port we need to fetch the header info before loading the rom because we have to align rom data
//The original code does simply a realloc() which is bad for our case!! ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)
static int file_size(fs_handle_t* f)
{
	int l;

	fs_seek(f, 0, FS_SEEK_END);
	l = fs_tell(f);
	fs_seek(f, 0, FS_SEEK_SET);
	return l;
}

#if __FS_BUILD__ != FS_N64_ROMFS
static byte* load_file_n64(fs_handle_t* f,const char* romfile) {
	
	byte* res = 0;

#ifndef HAVE_HW_EXTENDED_MEM
	//If we don't have hw dedicated RAM ....
	byte header[512]; //1sector

	//Get file len
	int flen = file_size(f);

	//Fetch rom size
	fs_seek(f,0,FS_SEEK_SET);
	fs_read(f,header,512); // Read a sector
	fs_seek(f,0,FS_SEEK_SET);

	//Now get aligned size
	int rom_size = (int)romsize_table[header[0x0148]] * 16384;

	//Great now we don't need reallocs read + fill extra space but always try to allocate at least 2mb 
	//To reduce heap fragmention chances...
	res = malloc((rom_size < (2*1024*1024)) ? 2*1024*1024 : rom_size);

	//But if it failed we will use the file length & retry
	if (!res) {
		res = malloc((flen + 64) & (~63));
		if (!res) {
			die("Out of memory!");
		}
		rom_size = 0;
	}

	fs_read(f,(void*)UncachedAddr(res),flen);

	
	if (rom_size > flen) {
		rom.bank = (void*)res;
		memset(rom.bank[0]+flen, 0xff, rom_size - flen);
	}

	return res;
#else	
	//Otherwise use it all
	res = fs_drv_get_extended_mem();
	if (!res) {
		die("fs_drv_get_extended_mem fail!");
	}
	fs_drv_cpy_file_to_extended_mem(romfile);
 
	return res;
#endif
}


int rom_load(const char* romfile)
{
	fs_handle_t*f=NULL;
	byte c, *data, *header;
	

	f = fs_open(romfile, "rb");
		
		if (!f) return 1;

#if 0
	int len = 0, rlen;
	data = loadfile(f, &len);
#else
	data = load_file_n64(f,romfile);
#endif
	header = data;
	
	memset(rom.name,'\0',20);
	memcpy(rom.name, header+0x0134, 16);
	if (rom.name[14] & 0x80) rom.name[14] = 0;
	if (rom.name[15] & 0x80) rom.name[15] = 0;
	rom.name[16] = 0;

	c = header[0x0147];
	mbc.type = mbc_table[c];
	mbc.batt = (batt_table[c] && !nobatt) || forcebatt;
	rtc.batt = rtc_table[c];
	mbc.romsize = romsize_table[header[0x0148]];
	mbc.ramsize = ramsize_table[header[0x0149]];

	if (!mbc.romsize) die("unknown ROM size %02X\n", header[0x0148]);
	if (!mbc.ramsize) die("unknown SRAM size %02X\n", header[0x0149]);

#if 0
	rlen = 16384 * mbc.romsize;
	rom.bank = realloc(data, rlen);
	if (rlen > len) memset(rom.bank[0]+len, 0xff, rlen - len);
#else
	rom.bank = (void*)data;
#endif
 
	ram.sbank = malloc(8192 * mbc.ramsize);
	 
	initmem(ram.sbank, 8192 * mbc.ramsize);
	initmem(ram.ibank, 4096 * 8);

	mbc.rombank = 1;
	mbc.rambank = 0;

	c = header[0x0143];
	hw.cgb = ((c == 0x80) || (c == 0xc0)) && !forcedmg;
	hw.gba = (hw.cgb && gbamode);

	if (f) fs_close(f);

	return 0;
}

#else

int rom_load(const char* romfile)
{
 	
	byte c,  *header;
 
	header = 0;
 
	{
		uint8_t dummy[256];
		ctl_dma_rom_rd(dummy,0,256);
		uint32_t game_len = ((uint32_t)dummy[0] << 24U) | ((uint32_t)dummy[1] << 16U) | ((uint32_t)dummy[2] << 8U) | ((uint32_t)dummy[3]);
		uint32_t state_len = ((uint32_t)dummy[4] << 24U) | ((uint32_t)dummy[5] << 16U) | ((uint32_t)dummy[6] << 8U) | ((uint32_t)dummy[7]);
		header = malloc(game_len);
		if (!header)die("rom_load : out of mem!\n");
		ctl_dma_rom_rd(header,256 + state_len,game_len);
	}
 
	memset(rom.name,'\0',20);
	memcpy(rom.name, header+0x0134, 16);
	if (rom.name[14] & 0x80) rom.name[14] = 0;
	if (rom.name[15] & 0x80) rom.name[15] = 0;
	rom.name[16] = 0;
	
	c = header[0x0147];
	mbc.type = mbc_table[c];
	mbc.batt = (batt_table[c] && !nobatt) || forcebatt;
	rtc.batt = rtc_table[c];
	mbc.romsize = romsize_table[header[0x0148]];
	mbc.ramsize = ramsize_table[header[0x0149]];

	if (!mbc.romsize) die("unknown ROM size %02X\n", header[0x0148]);
	if (!mbc.ramsize) die("unknown SRAM size %02X\n", header[0x0149]);

	rom.bank = header;//malloc(rs);
	//if (!rom.bank) die("out of memory");
	//memcpy(rom.bank,header,rs);
 

	ram.sbank = malloc(8192 * mbc.ramsize);
	 
	initmem(ram.sbank, 8192 * mbc.ramsize);
	initmem(ram.ibank, 4096 * 8);

	mbc.rombank = 1;
	mbc.rambank = 0;

	c = header[0x0143];
	hw.cgb = ((c == 0x80) || (c == 0xc0)) && !forcedmg;
	hw.gba = (hw.cgb && gbamode);
 

	
	return 0;
}

#endif

int sram_test() {
	return ctl_dbg_cmp_sram(ram.sbank,8192*mbc.ramsize);
}

int sram_load()
{
#if __FS_BUILD__ != FS_N64_ROMFS
	fs_handle_t* f;

	if (!mbc.batt /*|| !sramfile || !*sramfile*/) return -1;

	/* Consider sram loaded at this point, even if file doesn't exist */
	ram.loaded = 1;

	f = fs_open(sramfile, "rb");
	if (!f) return -1;
	fs_read(f,ram.sbank, 8192*mbc.ramsize);
	fs_close(f);
#else
	if (!mbc.batt) return -1;
	ram.loaded = 1;
	if (!ctl_read_sram(ram.sbank, 8192*mbc.ramsize))
		return -1;
#endif
	return 0;
}


int sram_save()
{
#if __FS_BUILD__ != FS_N64_ROMFS
	fs_handle_t* f;

	/* If we crash before we ever loaded sram, DO NOT SAVE! */
	if (!mbc.batt /*|| !sramfile */|| !ram.loaded || !mbc.ramsize)
		return -1;
	
	f = fs_open(sramfile, "wb");
	if (!f) return -1;
	fs_write(f,ram.sbank, 8192*mbc.ramsize);
	fs_close(f);
#else
	if (!mbc.batt /*|| !sramfile */|| !ram.loaded || !mbc.ramsize)
		return -1;
	ctl_write_sram(ram.sbank, 8192*mbc.ramsize);
#endif
	return 0;
}


void state_save(int n)
{
#if __FS_BUILD__ != FS_N64_ROMFS
	fs_handle_t* f;
	char name[MAX_ROM_PATH+8];

	sprintf(name, "%s.%03d", saveprefix, n);

	data_cache_hit_writeback_invalidate(name,sizeof(name));
	if ((f = fs_open(name, "wb")))
	{
		savestate(f);
		fs_close(f);
	}
#else
	savestate(NULL);
#endif
}


void state_load(int n)
{
#if __FS_BUILD__ != FS_N64_ROMFS
	fs_handle_t* f;
	char name[MAX_ROM_PATH+8];

 
	sprintf(name, "%s.%03d", saveprefix, n);
	data_cache_hit_writeback_invalidate(name,sizeof(name));

	if ((f = fs_open(name, "rb")))
	{
		loadstate(f);
		vram_dirty();
		pal_dirty();
		sound_dirty();
		mem_updatemap();
		fs_close(f);
	}
#else
		loadstate(NULL); 
		vram_dirty();
		pal_dirty();
		sound_dirty();
		mem_updatemap();
#endif
}

void rtc_save()
{
	fs_handle_t* f;
	if (!rtc.batt) return;
	if (!(f = fs_open(rtcfile, "wb"))) return;
	rtc_save_internal(f);
	fs_close(f);
}

void rtc_load()
{
	fs_handle_t* f;
	if (!rtc.batt) return;
	if (!(f = fs_open(rtcfile, "rb"))) return;
	rtc_load_internal(f);
	fs_close(f);
}

static void cleanup()
{
	sram_save();
	rtc_save();
	/* IDEA - if error, write emergency savestate..? */
}

void loader_unload()
{
	//cleanup();
#if 0
	if (romfile) free(romfile);
	if (sramfile) free(sramfile);
	if (saveprefix) free(saveprefix);
	romfile = 
	sramfile = saveprefix = 0;
#endif
#ifndef HAVE_HW_EXTENDED_MEM
	if (rom.bank) free(rom.bank);
#endif
	if (ram.sbank) free(ram.sbank);

	rom.bank = 0;
	ram.sbank =  0;
	mbc.type = mbc.romsize = mbc.ramsize = mbc.batt = 0;
}

/* basename/dirname like function */
static char *base(char *s)
{
	char *p;
	p = (char *) strrchr(s, DIRSEP_CHAR);
	if (p) return p+1;
	return s;
}

static char *ldup(char *s)
{
	int i;
	char *n, *p;
	p = n = malloc(strlen(s) + 2);
	for (i = 0; s[i]; i++) if (isalnum(s[i])) *(p++) = tolower(s[i]);
	*p = 0;
	return n;
}



void loader_init(char *s)
{
	char name[MAX_ROM_PATH+8];//char *name=0, *p;
 
	
	//sys_checkdir(savedir, 1); /* needs to be writable */
	 
	int rr = rom_load(s);
 
	vid_settitle(rom.name);

#if 0
	if (savename && *savename)
	{
		if (savename[0] == '-' && savename[1] == 0)
			name = ldup(rom.name);
		else name = strdup(savename);
	}
	else if (romfile && *base(romfile) && strcmp(romfile, "-"))
	{
		name = strdup(base(romfile));
		p = (char *) strchr(name, '.');
		if (p) *p = 0;
	}
	else name = ldup(rom.name);
#else

	
	strcpy(name,base(s));
	int i = strlen(name) - 1;
	for ( ;i > 0;--i) {
		if (name[i] == '.') {
			name[i] = '\0';
			break;
		}
	}

	
#endif
	
#if 0
	saveprefix = malloc(strlen(savedir) + strlen(name) + 2);

	sprintf(saveprefix, "%s%s%s", savedir, DIRSEP, name);

	sramfile = malloc(strlen(saveprefix) + 5);
	strcpy(sramfile, saveprefix);
	strcat(sramfile, ".sav");

	rtcfile = malloc(strlen(saveprefix) + 5);
	strcpy(rtcfile, saveprefix);
	strcat(rtcfile, ".rtc");
#else

	sprintf(saveprefix, "%s%s%s", savedir, DIRSEP, name);
	data_cache_hit_writeback_invalidate(saveprefix,sizeof(saveprefix));
	strcpy(sramfile, saveprefix);
	strcpy(rtcfile, saveprefix);
	data_cache_hit_writeback_invalidate(sramfile,sizeof(sramfile));
	data_cache_hit_writeback_invalidate(rtcfile,sizeof(rtcfile));
	strcat(rtcfile, ".rtc");
	strcat(sramfile, ".sav");



#endif
	ctl_set_signature(rom.name);
	ctl_build_initial_state_block();
	sram_load();
	rtc_load();
	//if(name)free(name);
	//atexit(cleanup);


}
 
