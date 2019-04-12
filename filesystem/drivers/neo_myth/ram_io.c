/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs (C)ChaN, 2010                     */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include <diskio.h>


static unsigned char sec_buf[512];


extern void neo_copyto_nsram(void *src, int sstart, int len);
extern void neo_copyfrom_nsram(void *dst, int sstart, int len);

extern void debugText(char *msg, int x, int y, int d);
//#define DEBUG_PRINT

#ifdef DEBUG_PRINT
void debugPrint( char *str )
{
    char temp[44];
    static int dbgX = 0;
    static int dbgY = 0;
    snprintf(temp, 40, "%s", str);
    debugText(temp, dbgX, dbgY, 20);
    dbgY++;
    if (dbgY > 27)
    {
        dbgY = 0;
        dbgX = (dbgX + 8) % 40;
    }
}
#else
#define debugPrint(x)
#endif

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS RAM_disk_initialize (void)
{
	DSTATUS result;

    neo_copyfrom_nsram(sec_buf, 0, 512);

    result = ((sec_buf[0x1FE] == 0x55) && (sec_buf[0x1FF] == 0xAA)) ? 0 : STA_NODISK;

    if (result)
        debugPrint("SRAM not formatted.");
    else
        debugPrint("SRAM Formatted.");

	return result;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS RAM_disk_status (void)
{
    return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT RAM_disk_read (
	BYTE *buff,			/* Data buffer to store read data */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to read (1..255) */
)
{
    int ix = 0;

    while (count)
    {
		BYTE rlen = (count >= 16) ? 16 : count;
        neo_copyfrom_nsram((void*)&buff[ix*512], (sector + ix)*512, rlen*512);
        ix += rlen;
		count -= rlen;
    }

    debugPrint("Read SRAM block");
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT RAM_disk_write (
    const BYTE *buff,    /* Data to be written */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count           /* Number of sectors to write (1..255) */
)
{
    int ix = 0;

    while (count)
    {
		BYTE wlen = (count >= 16) ? 16 : count;
        neo_copyto_nsram((void*)&buff[ix*512], (sector + ix)*512, wlen*512);
        ix += wlen;
		count -= wlen;
    }

    debugPrint("Write SRAM block");
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT RAM_disk_ioctl (
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
    switch (ctrl)
    {
        case GET_SECTOR_COUNT:
            *(unsigned int *)buff = 511;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(unsigned short *)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(unsigned int *)buff = 1;
            return RES_OK;
        case CTRL_SYNC:
            return RES_OK;
        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
        default:
            return RES_PARERR;
    }
}
