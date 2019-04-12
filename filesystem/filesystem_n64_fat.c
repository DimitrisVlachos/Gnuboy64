/*
	Basic N64 FATFS wrapper ((Dimitrios Vlachos : https://github.com/DimitrisVlachos))
	NOTE:Mounting has to take place elsewhere
*/

#include "filesystem.h"

#if __FS_BUILD__ == FS_N64_FATFS


#include <string.h>
#define unc_addr(_addr) ((void *)(((unsigned long)(_addr))|0x20000000))
 
static void c2wstrcpy(void* dst,const void* src)
{
    int ix = 0;
    while (1)
    {
        *(XCHAR *)(dst + ix*2) = *(char *)(src + ix) & 0x00FF;
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
    }
}

static void w2cstrcpy(void* dst,const void* src)
{
    int ix = 0;
    while (1)
    {
        *(char *)(dst + ix) = *(XCHAR *)(src + ix*2) & 0x00FF;
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
    }
}

fs_handle_t* fs_open(const char* path,const char* mode) {
	fs_handle_t* wrap;
	XCHAR tmp[MAX_FS_FILE_PATH]; 
	UINT flags = 0;

	if (!path || !mode) { 
		return (fs_handle_t*)0;
	}

	wrap = _fs_alloc_handle();
	if (!wrap) {
		return (fs_handle_t*)0;
	}
	
	c2wstrcpy((void*)tmp,(void*)path);

#if 1
	if ((0==strcmp(mode,"r+w"))) {
		flags = FA_CREATE_ALWAYS | FA_WRITE | FA_READ;
	} else if ((mode[0] == 'r')  ) {
		flags = FA_OPEN_EXISTING | FA_READ;
	} else if ((mode[0] == 'w')  ){
		flags = FA_CREATE_ALWAYS | FA_WRITE;
	} 
#else
	while (*mode) {
		switch (*(mode++)) {
			case 'r':
				flags |= FA_OPEN_EXISTING;
				flags |= FA_READ;
			break;
			case 'w':
				flags |= /*FA_CREATE_NEW*/FA_CREATE_ALWAYS;
				flags |= FA_WRITE;
			break;
		}
	}
#endif
	
	//memset(unc_addr(&wrap->ctx),0,sizeof(fs_handle_t));
	if(f_open(&wrap->ctx,tmp,flags) != FR_OK)
	{
		_fs_free_handle(wrap);
		return (fs_handle_t*)0;
	}

	return wrap;
}

void fs_close(fs_handle_t* handle) {
	if (handle) {
		f_close(&handle->ctx);
		_fs_free_handle(handle);
	}
}

int fs_seek(fs_handle_t* handle,unsigned int offs,int mode) {
	if (!handle) {
		return -1;
	}
 
	switch (mode) {
		case FS_SEEK_SET:
			return f_lseek(&handle->ctx,offs) == FR_OK ? 0 : -1;

		case FS_SEEK_CURR:
			return f_lseek(&handle->ctx,handle->ctx.fptr + offs) == FR_OK ? 0 : -1;
		
		case FS_SEEK_END:
			return f_lseek(&handle->ctx,handle->ctx.fsize) == FR_OK ? 0 : -1;
	
	}
	return 0;
}

unsigned int fs_tell(fs_handle_t* handle) {
	if (!handle) {	
		return 0;
	}
	return handle->ctx.fptr;
}

int fs_read(fs_handle_t* handle,void* dest,const unsigned int size) {
	if (!handle) {
		return 0;
	}
	
	UINT rd;
	f_read(&handle->ctx,(dest),size,&rd);
	return (int)rd;
}

int fs_write(fs_handle_t* handle,const void* src,const unsigned int size) {
	if (!handle) {
		return 0;
	}

	UINT wr;

	f_write(&handle->ctx,src,size,&wr);

	return (int)wr;
}

int fs_eof(fs_handle_t* handle) {
	return (handle) ? handle->ctx.fptr == handle->ctx.fsize : 1;
}

char* fs_gets(fs_handle_t* handle,char* buf,const unsigned int len) {
	return (handle) ? f_gets(buf,(int)len,&handle->ctx) : (char*)0;
}

int fs_list_files(const char* path,int (*cb)(const char*,fs_file_type_t)) {
	DIR dir;
	FILINFO info;
	XCHAR root[MAX_FS_FILE_PATH];
	char conv[MAX_FS_FILE_PATH];
 	fs_file_type_t type;

	if ((!cb) || (!path)) { return 0; }

	c2wstrcpy((void*)root,(const void*)path);
	if(f_opendir(&dir,root) != FR_OK)
		return 0;

	while(1) {
        info.lfname = root;
        info.lfsize = MAX_FS_FILE_PATH-1;
        info.lfname[0] = (XCHAR)0;
        info.fname[0] = '\0';

		if(f_readdir(&dir,&info) != FR_OK)
			return 1;
        else if (!info.fname[0])
			return 1;
		else if (info.fname[0] == '.')
			continue;
		else if (info.lfname[0] == (XCHAR)'.')
			continue;

		type = (info.fattrib & AM_DIR) ? FSFT_DIR : FSFT_FILE;

		if(info.lfname[0])
		{
			w2cstrcpy( (void*)conv ,info.lfname);

			if(!cb(conv,type))
				return 1;

			continue;
		}

		if(!cb(info.fname,type))
			return 1;
	}
	return 1;
}

#endif

