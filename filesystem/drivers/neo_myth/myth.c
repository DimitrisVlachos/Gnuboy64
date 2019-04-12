
#include "../fs_drv.h"
 
#include <diskio.h>
#include <ff.h>
#include <string.h>

#define sd_get_mode() fast_flag
#define sd_set_mode(_mode_) fast_flag = (_mode_)
#define sd_set_type(_type_) cardType = (_type_)

enum
{
	SD_FAST = 1,
	SD_SLOW = 0		
};

enum
{
	SD_FUNKY =  0x8000,		/*2^15 (b15 on) -> funky timing*/
	SD_NORMAL = 0x0000
};
 
/*													STANDARD NEO2 MODULE STUFF BEGIN									*/
unsigned int gBootCic;
unsigned int gCpldVers;                 /* 0x81 = V1.2 hardware, 0x82 = V2.0 hardware, 0x83 = V3.0 hardware */
unsigned int gCardTypeCmd;
unsigned int gPsramCmd;
short int gCardType;                    /* 0x0000 = newer flash, 0x0101 = new flash, 0x0202 = old flash */
unsigned int gCardID;                   /* should be 0x34169624 for Neo2 flash carts */
short int gCardOkay;                    /* 0 = okay, -1 = err */
short int gCursorX;                     /* range is 0 to 63 (only 0 to 39 onscreen) */
short int gCursorY;                     /* range is 0 to 31 (only 0 to 27 onscreen) */
extern short int cardType;         			/* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */
extern unsigned int num_sectors;        /* number of sectors on SD card (or 0) */
extern unsigned char *sd_csd;          			 /* card specific data */
short int gSdDetected = 0;              /* 0 - not detected, 1 - detected */
unsigned int fast_flag = 0;             /* 0 - use normal cart timing for SD read, 1 - use fast timing */
extern void disk_io_set_mode(int mode,int dat_swap);
extern void neo2_enable_sd(void);
extern void neo2_disable_sd(void);
extern DSTATUS MMC_disk_initialize(void);
extern void neo_select_menu(void);
extern void neo_select_game(void);
extern void neo_select_psram(void);
extern void neo_psram_offset(int offset);
extern unsigned int neo_id_card(void);
extern unsigned int neo_get_cpld(void);
/*													STANDARD NEO2 MODULE STUFF END   									*/

FATFS fat_filesys; 

static int mount_card();

static unsigned short slow_card = 0;

static void c2wstrcpy(void* dst, void* src)
{
    int ix = 0;
    while (1)
    {
        *(XCHAR *)(dst + ix*2) = *(char *)(src + ix) & 0x00FF;
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
    }
}

static int file_exists(const char* path)
{
	XCHAR s[256];
	FIL f;

	c2wstrcpy(s,(void*)path);

	if(f_open(&f,s,FA_OPEN_EXISTING | FA_READ) != FR_OK)
		return 0;

	f_close(&f);
	return 1;
}

#define UncachedAddr(_addr) ((void *)(((unsigned long)(_addr))|0x20000000))
void fs_drv_cpy_file_to_extended_mem(const char* path) { /*Won't work */
	extern void neo_copyto_psram(void *src, int pstart, int len);
	XCHAR s[256];
	unsigned char local_buf[512 * 8]; //8sectors
	FIL f;

	c2wstrcpy(s,(void*)path);
	if(f_open(&f,s,FA_OPEN_EXISTING | FA_READ) != FR_OK)
		return;

	unsigned int i = 0,j = f.fsize,rd;
	
	neo_select_psram(); 
	neo_psram_offset(0);

	unsigned int psram_base = 0;
	for (;i < j;) {
		rd = ((i + sizeof(local_buf)) > j) ? j - i :  sizeof(local_buf);
		i += rd;
		UINT _rd;
		f_read(&f,local_buf,rd,&_rd);
	
		rd = (rd + 4) & (~3);
		neo_copyto_psram( (local_buf),psram_base,rd);
		psram_base += rd;
		
	}
	f_close(&f);
}

unsigned char* fs_drv_get_extended_mem() { 
	neo_select_psram(); 
	neo_psram_offset(0);
 
	return (unsigned char*)(0xb0000000); /*16MB bank*/
}

int fs_drv_init(const char* args,int argc) {
 
	slow_card = 0;
    gBootCic = 6102;
    gCpldVers = neo_get_cpld();         // get Myth CPLD ID
    gCardID = neo_id_card();            // check for Neo Flash card
    neo_select_menu();                  // enable menu flash in cart space
    gCardType = *(volatile unsigned int *)(0xB01FFFF0) >> 16;
		
    switch(gCardType & 0x00FF)
    {
        case 0:
        gCardTypeCmd = 0x00DAAE44;      // set iosr = enable game flash for newest cards
        gPsramCmd = 0x00DAAF44;         // set iosr for Neo2-SD psram
        break;

        case 1:
        gCardTypeCmd = 0x00DA8E44;      // set iosr = enable game flash for new cards
        gPsramCmd = 0x00DA674E;         // set iosr for Neo2-Pro psram
        break;

        case 2:
        gCardTypeCmd = 0x00DA0E44;      // set iosr = enable game flash for old cards
        gPsramCmd = 0x00DA0F44;         // set iosr for psram
        break;

        default:
        gCardTypeCmd = 0x00DAAE44;      // set iosr = enable game flash for newest cards
        gPsramCmd = 0x00DAAF44;         // set iosr for psram
        break;
    }

	disk_io_set_mode(0,0);
    neo2_enable_sd();

	return mount_card();
}

int fs_drv_set_sdc_speed(int fast) {

	if (slow_card) {
		
		return SD_SLOW;
	}

	int prev = sd_get_mode();
	if (fast) {
		sd_set_mode(SD_FAST);
	} else {
		sd_set_mode(SD_SLOW);
	}
	return prev;
}

int fs_drv_close() {
	neo2_disable_sd();
	return 1;
}

int fs_drv_sync() {  /*Myth64 loses sync for no reason:/ */
	return fs_drv_init(0,0); 
}

static void set_sdc_speed() {
	if (file_exists("menu/n64/.fast")) {
		slow_card = 0;
		sd_set_mode(SD_FAST); 
		return;
	}
	slow_card = 1;
	sd_set_mode(SD_SLOW); 
}

static int mount_card()
{
	DIR dir;
	XCHAR root[32];

	sd_set_mode(SD_SLOW);
	sd_set_type(SD_NORMAL);
	f_mount(1, NULL);
	sd_set_type(SD_NORMAL);

	if(MMC_disk_initialize() == STA_NODISK) {
		
		sd_set_type(SD_NORMAL);
		f_mount(1, NULL);
		sd_set_type(SD_NORMAL);
		if(MMC_disk_initialize() == STA_NODISK) { return 0; }
	}

	sd_set_mode(SD_SLOW);
	if(f_mount(1,&fat_filesys))
		return 0;

	f_chdrive(1);
	c2wstrcpy(root,"/");
	sd_set_mode(SD_SLOW);

	if(f_opendir(&dir,root)) {
		sd_set_type(SD_FUNKY);
		if (MMC_disk_initialize() == STA_NODISK)
			return 0;
	}

	
	set_sdc_speed();
	return 1;
}



