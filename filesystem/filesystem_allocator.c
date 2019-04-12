
#include "filesystem.h"

#ifdef FS_REDUCE_HEAP_FRAGMENTION
/*Option to reduce heap fragmention ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)*/
#include <stdint.h>
#include <string.h>

static fs_handle_t _fs_handles[FS_MAX_HANDLES];
static uint8_t _fs_handle_flags[FS_MAX_HANDLES] = {0}; //TODO : Reduce by 1/8 by using bitflags...

fs_handle_t* _fs_alloc_handle() {
	uint32_t i;

	for (i = 0;i < FS_MAX_HANDLES;++i) {
		if (_fs_handle_flags[i] == 0) {
			_fs_handle_flags[i]=1;
			memset( &_fs_handle_flags[i] ,0,sizeof(fs_handle_t));
			return & _fs_handles[i];
		}
	}
	return 0;
}

void _fs_free_handle(fs_handle_t* f) {
	uint32_t i;

	for (i = 0;i < FS_MAX_HANDLES;++i) {
		if (& _fs_handles[i] == f) {
			_fs_handle_flags[i]=0;
			return;
		}
	}
}
#else
#include <malloc.h>

fs_handle_t* _fs_alloc_handle() {
	return malloc(sizeof(fs_handle_t));
}

void _fs_free_handle(fs_handle_t* f) {
	if (f) {
		free(f);
	}
}
#endif
