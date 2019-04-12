
#ifndef __PCM_H__
#define __PCM_H__


#include "defs.h"

struct __attribute__ ((aligned (16))) pcm 
{
	int hz, len;
	int stereo;
	byte *buf;
	int pos;
};

extern struct pcm __attribute__ ((aligned (16)))pcm;


#endif


