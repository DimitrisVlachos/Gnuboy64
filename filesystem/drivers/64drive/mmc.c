
#include "diskio.h"

DSTATUS MMC_disk_initialize (void) {
	return 0;
}

DSTATUS MMC_disk_status (void) {
	return 0;
}

DRESULT MMC_disk_read (BYTE *buff, DWORD sector, BYTE count) {
	return 0;
}

DRESULT MMC_disk_read_multi (BYTE *buff, DWORD sector, UINT count) {
	return 0;
}

DRESULT MMC_disk_write (const BYTE *buff, DWORD sector, BYTE count) {
	return 0;
}

DRESULT MMC_disk_ioctl (BYTE ctrl, void *buff) {
	return 0;
}

