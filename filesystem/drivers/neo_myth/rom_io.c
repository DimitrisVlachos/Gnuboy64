/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs (C)ChaN, 2010                     */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include <diskio.h>

unsigned int rom_offset = 0;
unsigned int rom_size = 0;
unsigned char rom_sec[512];

extern void neo_copyfrom_game(unsigned char *dest, int fstart, int len);


//#define DEBUG_PRINT

#ifdef DEBUG_PRINT
extern void delay(int count);
extern void put_str(char *str, int fcolor);
extern short int gCursorX;				/* range is 0 to 63 (only 0 to 39 onscreen) */
extern short int gCursorY;				/* range is 0 to 31 (only 0 to 27 onscreen) */
void debugRomPrint( char *str )
{
	char temp[44];
	snprintf(temp, 40, "%s", str);
	gCursorX = 0;
	gCursorY++;
	if (gCursorY > 27)
		gCursorY = 0;
	put_str(temp, 0);
	delay(60);
}
#else
#define debugRomPrint(x)
#endif

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS ROM_disk_initialize (void)
{
	DSTATUS result;

    neo_copyfrom_game(rom_sec, rom_offset, 512);

    result = ((rom_sec[0x1FE] == 0x55) && (rom_sec[0x1FF] == 0xAA)) ? 0 : STA_NODISK;
    if (result)
        rom_offset = 0;

    if (result)
        debugRomPrint("Flash ROM block failed init.");
    else
        debugRomPrint("Flash ROM block passed init.");

	return result;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS ROM_disk_status (void)
{
    return rom_offset ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT ROM_disk_read (
	BYTE *buff,			/* Data buffer to store read data */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to read (1..255) */
)
{
    int ix;

    for (ix=0; ix<count; ix++)
    {
        neo_copyfrom_game(rom_sec, rom_offset + ix*512, 512);
        memcpy(&buff[ix*512], rom_sec, 512);
    }

    debugRomPrint("Read Flash ROM block");
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT ROM_disk_ioctl (
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    switch (ctrl)
    {
        case GET_SECTOR_COUNT:
            *(unsigned int *)buff = rom_size >> 9;
            return rom_size ? RES_OK : RES_ERROR;
        case GET_SECTOR_SIZE:
            *(unsigned short *)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
        case CTRL_SYNC:
        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
        default:
            return RES_PARERR;
    }
}
