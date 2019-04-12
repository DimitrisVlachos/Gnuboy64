
/*
	A basic fs wrapper ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))
	NOTE:Mounting has to take place elsewhere
*/

#ifndef __filesystem_h__
#define __filesystem_h__
#include <stdint.h>
#include <string.h>

typedef struct fs_handle_t fs_handle_t;

typedef enum fs_file_type_t fs_file_type_t;

enum fs_file_type_t {
	FSFT_DIR = 0,
	FSFT_FILE,
};

#ifndef MAX_FS_FILE_PATH 
#define MAX_FS_FILE_PATH 1024
#endif
#ifdef FS_REDUCE_HEAP_FRAGMENTION
#ifndef FS_MAX_HANDLES
#define FS_MAX_HANDLES 8
#endif
#endif

/*Standard IO (STDLIB)*/
#define FS_STD (0)

/*N64 FAT File system*/
#define FS_N64_FATFS (1)

/*N64 ROM File system*/
#define FS_N64_ROMFS (2)

/*Set default FS wrapper in MAKEFILE . */
/*Example -D__FS_BUILD__=FS_N64_FATFS */


#if __FS_BUILD__ == FS_N64_FATFS
#include <ff.h>
#define FS_SEEK_SET (1)
#define FS_SEEK_CURR (2)
#define FS_SEEK_END (3)
struct __attribute__((aligned(64))) fs_handle_t  {
	FIL ctx; 
	/*unsigned int wbuf[512*32/sizeof(unsigned int)];
	unsigned int wbuf_sec_dry_bits;*/
};

#define fs_scanf(_handle_,_s_fmt_,...) 
#define fs_printf(_handle_,_s_fmt_,...) 

#elif  __FS_BUILD__ == FS_STD
#include <stdio.h>
#define FS_SEEK_SET SEEK_SET
#define FS_SEEK_END SEEK_END
#define FS_SEEK_CURR SEEK_CUR

struct fs_handle_t {
	FILE* ctx;
};

#define fs_scanf(_handle_,_s_fmt_,...) fscanf((_handle_)->ctx,_s_fmt_,__VA_ARGS__)
#define fs_printf(_handle_,_s_fmt_,...) fprintf((_handle_)->ctx,_s_fmt_,__VA_ARGS__)

#elif __FS_BUILD__ == FS_N64_ROMFS

#define FS_SEEK_SET (1)
#define FS_SEEK_CURR (2)
#define FS_SEEK_END (3)
struct __attribute__((aligned(64))) fs_handle_t  {
	uint32_t mode;
	uint32_t ext;
	uint32_t addr;
	uint32_t len;
};

#define fs_scanf(_handle_,_s_fmt_,...)  
#define fs_printf(_handle_,_s_fmt_,...) 
#else
NO FILESYSTEM DEFINED
#endif

extern fs_handle_t* _fs_alloc_handle();
void _fs_free_handle(fs_handle_t* f);
 

fs_handle_t* fs_open(const char* path,const char* mode);
void fs_close(fs_handle_t* handle);
int fs_read(fs_handle_t* handle,void* dest,const unsigned int size);
int fs_write(fs_handle_t* handle,const void* src,const unsigned int size);
int fs_seek(fs_handle_t* handle,unsigned int offs,int mode);
int fs_eof(fs_handle_t* handle);
char* fs_gets(fs_handle_t* handle,char* buf,const unsigned int len);
unsigned int fs_tell(fs_handle_t* handle);
int fs_list_files(const char* path,int (*cb)(const char*,fs_file_type_t));
#endif


