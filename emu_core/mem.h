#ifndef __MEM_H__
#define __MEM_H__


#include "defs.h"



#define MBC_NONE 0
#define MBC_MBC1 1
#define MBC_MBC2 2
#define MBC_MBC3 3
#define MBC_MBC5 5
#define MBC_RUMBLE 15
#define MBC_HUC1 0xC1
#define MBC_HUC3 0xC3

struct __attribute__ ((aligned (16))) mbc 
{
	int type;
	int model;
	int rombank;
	int rambank;
	int romsize;
	int ramsize;
	int enableram;
	int batt;
	byte *rmap[0x10], *wmap[0x10];
};

struct __attribute__ ((aligned (16))) rom 
{
	byte (*bank)[16384];
	char name[20];
};

struct __attribute__ ((aligned (16))) ram 
{
	byte hi[256];
	byte ibank[8][4096];
	byte (*sbank)[8192];
	int loaded;
};


extern struct mbc  __attribute__ ((aligned (16))) mbc;
extern struct rom __attribute__ ((aligned (16)))  rom;
extern struct ram  __attribute__ ((aligned (16))) ram;


void mem_updatemap();
void ioreg_write(byte r, byte b);
void mbc_write(int a, byte b);
void mem_write(int a, byte b);
byte mem_read(int a);
void mbc_reset();
void ioreg_write(byte r, byte b);
byte ioreg_read(byte r);

#define READB(a) ( mbc.rmap[(a)>>12] \
? mbc.rmap[(a)>>12][(a)] \
: mem_read((a)) )
#define READW(a) ( READB((a)) | ((word)READB((a)+1)<<8) )

#define WRITEB(a, b) ( mbc.wmap[(a)>>12] \
? ( mbc.wmap[(a)>>12][(a)] = (b) ) \
: ( mem_write((a), (b)), (b) ) )
#define WRITEW(a, w) ( WRITEB((a), (w)&0xFF), WRITEB((a)+1, (w)>>8) )


#endif
