/*
	A non-blocking audio ring buffer implementation that replaces libdragon's AI queue / (Dimitrios Vlachos : https://github.com/DimitrisVlachos)
*/
#ifndef _pcm_rbuf_h_
#define _pcm_rbuf_h_

#include <libdragon.h>
#include <stdint.h>
typedef enum pcm_rbuf_mode_t pcm_rbuf_mode_t;
enum pcm_rbuf_mode_t {
	PCMRBUF_MODE_IMMEDIATE = 0,
	PCMRBUF_MODE_NON_BLOCKING
};

int32_t pcm_rbuf_init(const int32_t freq,const int32_t queue_buffers,const int32_t rbuf_chunks);
void pcm_rbuf_close();
void pcm_rbuf_cycle();
void pcm_rbuf_reset();
void pcm_rbuf_set_mode(pcm_rbuf_mode_t mode);
#endif

