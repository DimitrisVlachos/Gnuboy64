
/*
	Basic STDIO fs wrapper ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))
	NOTE:Mounting has to take place elsewhere
*/
#include "filesystem.h"

#if __FS_BUILD__ == FS_STD

#include <malloc.h>

fs_handle_t* fs_open(const char* path,const char* mode) {
	fs_handle_t* wrap;

	wrap = malloc(sizeof(fs_handle_t)); 
	if (!wrap) {
		return NULL;
	}
	wrap->ctx = fopen(path,mode);
	if (!wrap->ctx) {
		free(wrap);
		return NULL;
	}
	return wrap;
}

void fs_close(fs_handle_t* handle) {
	if (handle) {
		if (handle->ctx) {
			fclose(handle->ctx);
		}
		free(handle);
	}
}

int fs_seek(fs_handle_t* handle,unsigned int offs,int mode) {
	return fseek(handle->ctx,offs,mode);
}

unsigned int fs_tell(fs_handle_t* handle) {
	return ftell(handle->ctx);
}

int fs_read(fs_handle_t* handle,void* dest,const unsigned int size) {
	return fread(dest,1,size,handle->ctx);
}

int fs_write(fs_handle_t* handle,const void* src,const unsigned int size) {
	return fwrite(src,1,size,handle->ctx);
}

int fs_eof(fs_handle_t* handle) {
	return feof(handle->ctx);
}

char* fs_gets(fs_handle_t* handle,char* buf,const unsigned int len) {
	return fgets(buf,len,handle->ctx);
}

int fs_list_files(const char* path,int (*cb)(const char*,fs_file_type_t)) {
	if ((!cb) || (!path)) { return 0; }
	return 0;
}
#endif

