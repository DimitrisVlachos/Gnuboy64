#include "wstring.h"

void w2cstrcpy(void* dst, void* src)
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

void w2cstrcat(void* dst, void* src)
{
    int ix = 0, iy = 0;
    // find end of str in dst
    while (1)
    {
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
    }
    while (1)
    {
        *(char *)(dst + ix) = *(XCHAR *)(src + iy*2) & 0x00FF;
        if (*(char *)(dst + ix) == 0)
            break;
        ix++;
        iy++;
    }
}

void c2wstrcpy(void* dst, void* src)
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

void c2wstrcat(void* dst, void* src)
{
    int ix = 0, iy = 0;
    // find end of str in dst
    while (1)
    {
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
    }
    while (1)
    {
        *(XCHAR *)(dst + ix*2) = *(char *)(src + iy) & 0x00FF;
        if (*(XCHAR *)(dst + ix*2) == (XCHAR)0)
            break;
        ix++;
        iy++;
    }
}

int wstrcmp(WCHAR* ws1, WCHAR* ws2)
{
    int ix = 0;
    while (ws1[ix] && ws2[ix] && (ws1[ix] == ws2[ix])) ix++;
    if (!ws1[ix] && ws2[ix])
        return -1; // ws1 < ws2
    if (ws1[ix] && !ws2[ix])
        return 1; // ws1 > ws2
    return (int)ws1[ix] - (int)ws2[ix];
}

