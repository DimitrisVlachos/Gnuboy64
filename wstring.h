#ifndef _wstring_h_
#define _wstring_h_

#include <ff.h>
void w2cstrcpy(void* dst, void* src);
void w2cstrcat(void* dst, void* src);
void c2wstrcpy(void* dst, void* src);
void c2wstrcat(void* dst, void* src);
int wstrcmp(WCHAR* ws1, WCHAR* ws2);
#endif

