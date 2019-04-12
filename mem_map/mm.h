#ifndef __mm_h__
#define __mm_h__
#include <stdlib.h>



 void mem_read_range(int a,int cnt,unsigned char* out);
void mem_write_range(int  a,int cnt,const unsigned char* in);
void* mem_map_write_ptr(int a);
void* mem_map_read_ptr(int a);
#endif

