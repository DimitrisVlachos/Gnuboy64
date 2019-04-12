/*
	A non-blocking audio ring buffer implementation that replaces libdragon's AI queue / (Dimitrios Vlachos : https://github.com/DimitrisVlachos)
*/
 
#include "pcm_ring_buf.h"
#include "emu_core/pcm.h"
#include <string.h>
#include <malloc.h>

#define CALC_BUFFER(x)  ( ( ( ( x ) / 25 ) >> 3 ) << 3 )
 
/*N64 PORT newlib lacks def so use u32 for this platform*/
typedef uint32_t ptrdiff_t;


typedef struct AI_regs_s {
    /** @brief Pointer to uncached memory buffer of samples to play */
    volatile void * address;
    /** @brief Size in bytes of the buffer to be played.  Should be
     *         number of stereo samples * 2 * sizeof( uint16_t ) 
     */
    uint32_t length;
    /** @brief DMA start register.  Write a 1 to this register to start
     *         playing back an audio sample. */
    uint32_t control;
    /** @brief AI status register.  Bit 31 is the full bit, bit 30 is the busy bit. */
    uint32_t status;
    /** @brief Rate at which the buffer should be played.
     *
     * Use the following formula to calculate the value: ((2 * clockrate / frequency) + 1) / 2 - 1
     */
    uint32_t dacrate;
    /** @brief The size of a single sample in bits. */
    uint32_t samplesize;
} AI_regs_t;

extern struct  __attribute__ ((aligned (16))) pcm;

static pcm_rbuf_mode_t pcm_rbuf_mode = PCMRBUF_MODE_IMMEDIATE;
static uint8_t pcm_rbuf_handler_installed = 0;
static uint8_t* pcm_rbuf = 0;
static uint32_t pcm_rbuf_eos = 0;
static uint32_t pcm_rbuf_stride = 0;
static uint32_t pcm_rbuf_head = 0,pcm_rbuf_prev_head = 0,pcm_rbuf_int_head = 0;
static uint32_t pcm_rbuf_tail = 0;
static uint32_t pcm_rbuf_nsamples = 0;
static volatile AI_regs_t *  AI_regs = (volatile AI_regs_t *)0xa4500000;


static inline void ai_ints_off() {
	set_AI_interrupt(0);
}

static inline void ai_ints_on() {
	set_AI_interrupt(1);
}

static inline void ai_wait_rdy() {
	//while (AI_regs->status & (1 << 30)){}
	while (AI_regs->status & (1 << 31)){}
}

/*Transmit a single sample*/
static inline void xmit_sample_imm(const uint8_t* sample) {
	MEMORY_BARRIER();
	AI_regs->address = UncachedAddr(sample);
	MEMORY_BARRIER();
	AI_regs->length = (pcm_rbuf_nsamples << 2) & (~7) ;
	MEMORY_BARRIER();
	AI_regs->control = 1;
	MEMORY_BARRIER();
}

static inline void xmit_sample(const uint8_t* sample) {
	ai_wait_rdy();
	xmit_sample_imm(sample);
}

/*
	Transmit a unified ring buffer as multipart-sample
	Head/Tail must be aligned to sample.stride address
*/
static inline void xmit_unified_samples_imm(const uint8_t* head,const uint8_t* tail) {
	MEMORY_BARRIER();
	AI_regs->address = UncachedAddr(head);
	MEMORY_BARRIER();
	AI_regs->length = ((pcm_rbuf_nsamples << 2) * ((ptrdiff_t)(tail-head) / pcm_rbuf_stride)) & (~7);
	MEMORY_BARRIER();
	AI_regs->control = 1;
	MEMORY_BARRIER();
}

static inline void xmit_unified_samples(const uint8_t* head,const uint8_t* tail) {
	ai_wait_rdy();
	xmit_unified_samples_imm(head,tail);
}

/*Merge as many samples as possible but up to max allowed!*/
static inline const uint32_t merge_samples(const uint32_t head,const uint32_t tail,const uint32_t stride,const uint32_t max) {
	uint32_t rtail = head;
	for (uint32_t i = 0;i < max;++i) {
		if (rtail + stride > tail) { return tail; }
		rtail += stride;
	}
	return rtail;
}

void pcm_rbuf_set_mode(pcm_rbuf_mode_t mode) {
	disable_interrupts();
	switch (mode) {
		default:break;
		case PCMRBUF_MODE_IMMEDIATE: /*Should clean resources...*/
			pcm_rbuf_eos = 0;
			pcm_rbuf_mode = mode;
		break;
		case PCMRBUF_MODE_NON_BLOCKING:
			pcm_rbuf_eos = 0;
			pcm_rbuf_mode = mode;
		break;
	}
	enable_interrupts();
}

void pcm_rbuf_int() {
	if (PCMRBUF_MODE_IMMEDIATE == pcm_rbuf_mode) { return; }
	/*Dont touch write buffer*/
	const uint32_t cut =  (pcm_rbuf_eos) ? pcm_rbuf_tail : pcm_rbuf_head;
 
	
#if 1
	/*Send as many as possible to fill the 2 available dma channels...*/
	while ( (pcm_rbuf_int_head < cut) && (!(AI_regs->status & (1 << 31))) ) {
		xmit_sample_imm(&pcm_rbuf[pcm_rbuf_int_head]);
		pcm_rbuf_int_head += pcm_rbuf_stride;
		 
	}
#else
	if (!(AI_regs->status & (1 << 31))) { /*Merge full chain*/
		xmit_unified_samples_imm(&pcm_rbuf[pcm_rbuf_int_head],&pcm_rbuf[cut]);
		pcm_rbuf_int_head += (ptrdiff_t)&pcm_rbuf[cut] - (ptrdiff_t)&pcm_rbuf[pcm_rbuf_int_head];
	}
#endif
	/*Flush ring buffer edge if pending */
	if ( (pcm_rbuf_eos) && (pcm_rbuf_int_head >= cut) ) {
		pcm_rbuf_int_head = 0;
		pcm_rbuf_eos = 0;
	}
}

int32_t pcm_rbuf_init(const int32_t freq,const int32_t queue_buffers,const int32_t rbuf_chunks) {
 
	audio_init_ex(freq,queue_buffers,CALC_BUFFER(freq) >> 2,AS_FM_STREAM,pcm_rbuf_int);

	pcm_rbuf_stride = audio_get_buffer_length() * sizeof(short) * 2;
	pcm_rbuf_tail = pcm_rbuf_stride * rbuf_chunks;
	pcm_rbuf_prev_head = pcm_rbuf_head = 0;
	pcm_rbuf_int_head = 0 ;
	pcm_rbuf = (uint8_t*)malloc(pcm_rbuf_tail );
	if (!pcm_rbuf) {
		return 0;
	}
	pcm_rbuf_eos = 0;
	memset(&pcm,0,sizeof pcm);
 
	pcm_rbuf_nsamples = audio_get_num_samples();
	pcm.hz = audio_get_frequency();
	pcm.stereo = 1;
	pcm.len = pcm_rbuf_stride ;
	pcm.buf = UncachedAddr(pcm_rbuf);
	pcm.pos = 0;
	
	memset(UncachedAddr(pcm_rbuf),0,pcm_rbuf_tail );

	/*Hack to add pcm rbuf's callback to the ld's internal linked list of callbacks..*/
	if (!pcm_rbuf_handler_installed) {
 		disable_interrupts();
		ai_ints_off();
		register_VI_handler(pcm_rbuf_int); /*int type doesn't matter :)*/
		enable_interrupts();
		pcm_rbuf_handler_installed = 1;
	}

	pcm_rbuf_set_mode(PCMRBUF_MODE_IMMEDIATE);
	return 1;
}


void pcm_rbuf_reset() {	
	disable_interrupts(); 
	pcm_rbuf_eos = 0;
	pcm_rbuf_int_head = 0 ;
	pcm_rbuf_prev_head = pcm_rbuf_head = 0;
	pcm.buf = UncachedAddr(pcm_rbuf);
	pcm.pos = 0;
	enable_interrupts();
}

void pcm_rbuf_cycle() {
	
	if (pcm.pos < pcm.len) {
		return;
	}

	switch (pcm_rbuf_mode) {
		case PCMRBUF_MODE_IMMEDIATE:
			xmit_sample(&pcm_rbuf[pcm_rbuf_head]);
			pcm.pos = 0;  
			pcm_rbuf_prev_head = pcm_rbuf_head;
			pcm_rbuf_head = (pcm_rbuf_head + pcm_rbuf_stride) % pcm_rbuf_tail;  
			pcm.buf = UncachedAddr(&pcm_rbuf[pcm_rbuf_head]);
		return;
		case PCMRBUF_MODE_NON_BLOCKING:
			disable_interrupts(); 
			pcm.pos = 0;  
			pcm_rbuf_prev_head = pcm_rbuf_head;
			pcm_rbuf_head += pcm_rbuf_stride;  
			if (pcm_rbuf_head >= pcm_rbuf_tail ) {
				pcm_rbuf_head = 0;
				pcm_rbuf_eos = 1;
			}
			pcm.buf = UncachedAddr(&pcm_rbuf[pcm_rbuf_head]);
			enable_interrupts();
		return;
	}
}

void pcm_rbuf_close() {
	if (pcm_rbuf) {
		free(pcm_rbuf);
		pcm_rbuf = 0;
	}
	pcm_rbuf_head = pcm_rbuf_tail = pcm_rbuf_stride = pcm_rbuf_prev_head = 0;
	audio_close();
}

