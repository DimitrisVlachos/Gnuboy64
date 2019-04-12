/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

extern DSTATUS RAM_disk_initialize (void);
extern DSTATUS RAM_disk_status (void);
extern DRESULT RAM_disk_read (BYTE *buff, DWORD sector, BYTE count);
extern DRESULT RAM_disk_write (const BYTE *buff, DWORD sector, BYTE count);
extern DRESULT RAM_disk_ioctl (BYTE ctrl, void *buff);

extern DSTATUS MMC_disk_initialize (void);
extern DSTATUS MMC_disk_status (void);
extern DRESULT MMC_disk_read (BYTE *buff, DWORD sector, BYTE count);
extern DRESULT MMC_disk_read_multi (BYTE *buff, DWORD sector, UINT count);
extern DRESULT MMC_disk_write (const BYTE *buff, DWORD sector, BYTE count);
extern DRESULT MMC_disk_ioctl (BYTE ctrl, void *buff);

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */

#define RAM		0			/* Save RAM as a FAT volume */
#define MMC		1			/* SD(HC) card interface */

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	if (drv == RAM)
		return RAM_disk_initialize();

	if (drv == MMC)
		return MMC_disk_initialize();

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	if (drv == RAM)
		return RAM_disk_status();

	if (drv == MMC)
		return MMC_disk_status();

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read_multi (
	BYTE drv,		 
	BYTE *buff,		 
	DWORD sector,	 
	UINT count		 
)
{
	return MMC_disk_read_multi(buff, sector, count);
}

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	if (drv == RAM)
		return RAM_disk_read(buff, sector, count);

	if (drv == MMC)
		return MMC_disk_read(buff, sector, count);

	return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	if (drv == RAM)
		return RAM_disk_write(buff, sector, count);

	if (drv == MMC)
		return MMC_disk_write(buff, sector, count);

	return RES_PARERR;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	if (drv == RAM)
		return RAM_disk_ioctl(ctrl, buff);

	if (drv == MMC)
		return MMC_disk_ioctl(ctrl, buff);

	return RES_PARERR;
}
