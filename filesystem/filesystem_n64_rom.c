/*
	Basic N64 ROMFS wrapper ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))
*/

#include "filesystem.h"

#if __FS_BUILD__ == FS_N64_ROMFS

 
fs_handle_t* fs_open(const char* path,const char* mode) {
	return NULL;
}

void fs_close(fs_handle_t* handle) {
}

int fs_seek(fs_handle_t* handle,unsigned int offs,int mode) {
	return 0;
}

unsigned int fs_tell(fs_handle_t* handle) {
	return 0;
}

int fs_read(fs_handle_t* handle,void* dest,const unsigned int size) {
	return 0;
}

int fs_write(fs_handle_t* handle,const void* src,const unsigned int size) {
	return 0;
}

int fs_eof(fs_handle_t* handle) {
	return 1;
}

char* fs_gets(fs_handle_t* handle,char* buf,const unsigned int len) {
	return NULL;
}

int fs_list_files(const char* path,int (*cb)(const char*,fs_file_type_t)) {
	if ((!cb) || (!path)) { return 0; }
	return 0;
}
#endif

