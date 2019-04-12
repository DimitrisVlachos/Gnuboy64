
#include "../fs_drv.h"

int fs_drv_init(const char* args,int argc) {
	return 1;
}

int fs_drv_close() {
	return 1;
}

int fs_drv_set_sdc_speed(int fast) {
	return 1;
}

int fs_drv_sync() {
	return 1;
}

/*
	To use the cart hw dedicated memory feature make sure that your makefile fs flags look like this :
	FS_FLAGS = -D__FS_BUILD__=FS_N64_FATFS -DHAVE_HW_EXTENDED_MEM
*/
void fs_drv_cpy_file_to_extended_mem(const char* path) {  
	/*Stream file to  cart dedicated hw memory*/
 
}

unsigned char* fs_drv_get_extended_mem() {
	/*Return cart dedicated hw memory*/
	return 0;
}
