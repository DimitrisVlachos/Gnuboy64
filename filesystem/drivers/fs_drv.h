
#ifndef __fs_drv__
#define __fs_drv__

extern int fs_drv_init(const char* args,int argc);
extern int fs_drv_close();
extern int fs_drv_sync();
extern int fs_drv_set_sdc_speed(int fast);
extern void fs_drv_cpy_file_to_extended_mem(const char* path);
extern unsigned char* fs_drv_get_extended_mem();
#endif

