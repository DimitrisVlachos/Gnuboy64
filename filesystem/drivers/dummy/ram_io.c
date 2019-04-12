/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs (C)ChaN, 2010                     */
/*-----------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include <diskio.h>

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS RAM_disk_initialize (void)
{
	return 0;
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

	return 0;
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

    return 0;
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
