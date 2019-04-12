/*
	Peripheral support for ROMFS_BUILD ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos) (WARNING : COMPILE WITH .NO_OPT)
*/

#include "ctl.h"
#include <libdragon.h>
#include <string.h>
#include <malloc.h>
 
typedef volatile uint32_t vu32;

#define PI_STATUS_REG       *(vu32*)(0xA4600010)
#define PI_BSD_DOM1_LAT_REG *(vu32*)(0xA4600014)
#define PI_BSD_DOM1_PWD_REG *(vu32*)(0xA4600018)
#define PI_BSD_DOM1_PGS_REG *(vu32*)(0xA460001C)
#define PI_BSD_DOM1_RLS_REG *(vu32*)(0xA4600020)
#define PI_BSD_DOM2_LAT_REG *(vu32*)(0xA4600024)
#define PI_BSD_DOM2_PWD_REG *(vu32*)(0xA4600028)
#define PI_BSD_DOM2_PGS_REG *(vu32*)(0xA460002C)
#define PI_BSD_DOM2_RLS_REG *(vu32*)(0xA4600030)
#define PI_STATUS_DMA_BUSY ( 1 << 0 )
#define PI_STATUS_IO_BUSY  ( 1 << 1 )
#define PI_STATUS_ERROR    ( 1 << 2 )
#define FRAM_STATUS_REG         *(vu32*)(0xA8000000)
#define FRAM_COMMAND_REG        *(vu32*)(0xA8010000)
#define FRAM_EXECUTE_CMD        (0xD2000000)
#define FRAM_STATUS_MODE_CMD    (0xE1000000)
#define FRAM_ERASE_OFFSET_CMD   (0x4B000000)
#define FRAM_WRITE_OFFSET_CMD   (0xA5000000)
#define FRAM_ERASE_MODE_CMD     (0x78000000)
#define FRAM_WRITE_MODE_CMD     (0xB4000000)
#define FRAM_READ_MODE_CMD      (0xF0000000)
 
#ifndef ROMFS_RELATIVE_ADDR 
	#define ROMFS_RELATIVE_ADDR (uint32_t)(2U * 1024U * 1024U )
#endif

typedef struct PI_regs_s {
    /** @brief Uncached address in RAM where data should be found */
    volatile void * ram_address;
    /** @brief Address of data on peripheral */
    uint32_t pi_address;
    /** @brief How much data to read from RAM into the peripheral */
    uint32_t read_length;
    /** @brief How much data to write to RAM from the peripheral */
    uint32_t write_length;
    /** @brief Status of the PI, including DMA busy */
    uint32_t status;
} PI_regs_t;

static const uint32_t state_blocks = 1;
static const uint32_t state_block_addr[] = {
	128 * 1024
};

static uint8_t __attribute__((aligned(16))) dma_buffer[256];
static uint8_t signature[16];

extern void delay_secs(int32_t secs);
extern void die(char *fmt, ...);

static inline uint32_t get_rom_space_addr() {
	const uint32_t base = (uint32_t)0xB0000000U;
	return base + ROMFS_RELATIVE_ADDR;
}
 


 
static volatile struct PI_regs_s * const PI_regs2 = (struct PI_regs_s *)0xa4600000;
 
static volatile int dma2_err() {
    return PI_regs2->status & PI_STATUS_ERROR;
}

static volatile int dma2_busy() {
    return PI_regs2->status & (PI_STATUS_DMA_BUSY | PI_STATUS_IO_BUSY);
}

static volatile void dma2_read(void * ram_address, unsigned long pi_address, unsigned long len) {
	if (0 == len) 
		return;

	//int err;

    disable_interrupts();

    while (dma2_busy()) ;
    MEMORY_BARRIER();	
	PI_regs2->status = 0;
    MEMORY_BARRIER();
    PI_regs2->ram_address = ram_address;
    MEMORY_BARRIER();
    PI_regs2->pi_address =  pi_address;
    MEMORY_BARRIER();
    PI_regs2->write_length = len-1;
    MEMORY_BARRIER();
    while (dma2_busy()) ;
	//err = dma2_err();

    enable_interrupts();

	//if (err)die("dma2_read error");
}

static volatile void dma2_write(void * ram_address, unsigned long pi_address, unsigned long len) {
	if (0 == len)
		return;

	//int err;

    disable_interrupts();

    while (dma2_busy()) ;
    MEMORY_BARRIER();	
	PI_regs2->status = 0;
    MEMORY_BARRIER();
    PI_regs2->ram_address = ram_address;
    MEMORY_BARRIER();
    PI_regs2->pi_address =  pi_address;
    MEMORY_BARRIER();
    PI_regs2->read_length = len-1;
    MEMORY_BARRIER();
    while (dma2_busy()) ;
	//err = dma2_err();

    enable_interrupts();
	//if (err)die("dma2_write error");
}

void ctl_set_signature(const char* sig) {
	int l = strlen(sig);
	data_cache_hit_writeback_invalidate((volatile void*)signature,sizeof(signature));
	memset(signature,0,sizeof(signature));
	memcpy(signature,(const void*)sig,(l > sizeof(signature)) ? sizeof(signature) : l);
	data_cache_hit_invalidate((volatile void*)signature,sizeof(signature));
}
 
void ctl_write_sram(const void* src,int len) {
	uint8_t sig[128];
	ctl_sram_rd(sig,0,128);
	memcpy(sig,signature,sizeof(signature));
	ctl_sram_wr(signature,0,128);
	ctl_sram_wr(src,128,len);
}

void ctl_write_rtc(const void* src,int len) {
	uint8_t sig[128];
	len = (len > (128 - sizeof(signature))) ? 128 - sizeof(signature): len;
	memcpy(sig,signature,sizeof(signature));
	memset(sig + sizeof(signature),0,len);
	ctl_sram_wr(sig,0,128);
}

void ctl_write_state(const void* src,int len,int block) {
	if (block >= state_blocks)
		return;
	else if (len > 128 * 1024)
		die("write_state > 128KB");

	uint8_t sig[128];
	ctl_sram_rd(sig,0,128);
	memcpy(sig,signature,sizeof(signature));
	ctl_sram_wr(sig,0,128);
	ctl_sram_wr(src,state_block_addr[block],len);
}

void ctl_read_state(void* dst,int len,int block) {
	if (block >= state_blocks)
		return;
	else if (len > 128 * 1024)
		die("read_state > 128KB");


	uint8_t sig[128];

	ctl_sram_rd(sig,0,128);
	if ( 0 != memcmp(signature,sig,sizeof(signature)) )
		return;

	ctl_sram_rd(dst,state_block_addr[block],len);
}


int ctl_dbg_cmp_sram(const void* src,int len) {
	uint8_t sig[128];
	uint8_t* p;

	ctl_sram_rd(sig,0,128);
	if ( 0 != memcmp(signature,sig,sizeof(signature)) )
		return 0;
	
	p = malloc(len);
	if (!p)
		return 0;

	ctl_sram_rd(p,128,len);
	int r = memcmp(src,p,len);

	free(p);
	return r==0;
}

int ctl_read_sram(void* dst,int len) { //reserve 128-16 bytes for rtc
	uint8_t sig[128];

	ctl_sram_rd(sig,0,128);
	if ( 0 != memcmp(signature,sig,sizeof(signature)) ) {
		memset(dst,0,len);
		return 0;
	}

	ctl_sram_rd(dst,128,len);
	return 1;
}

int ctl_read_rtc(void* dst,int len) {	//reserve 128-16 bytes for rtc
	uint8_t sig[128];

	ctl_sram_rd(sig,0,128);
	if ( 0 != memcmp(signature,sig,sizeof(signature)) ) {
		memset(dst,0,len);
		return 0;
	}
	len = (len > (128 - sizeof(signature))) ? 128 - sizeof(signature): len;
	memcpy(dst,sig+sizeof(signature),len);
	return 1;
}

void ctl_build_initial_state_block() {
	uint8_t sig[128];

	ctl_sram_rd(sig,0,128);
	if ( 0 == memcmp(signature,sig,sizeof(signature)) )
		return;

	memset(sig+sizeof(signature),0,128-sizeof(signature));
	memcpy(sig,signature,sizeof(signature));
	ctl_sram_wr(sig,0,128);
}

void ctl_sram_wr(void* p_src,uint32_t base,uint32_t len) {
	uint32_t prg;
	const uint8_t* src = (const uint8_t*)p_src;

	vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
	vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
	vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
	vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;

    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

	data_cache_hit_invalidate((volatile void*)src,len);
	if (!((uint32_t)src & 7)) { 
        while (dma2_busy()){}
        	PI_STATUS_REG = 2;
       		dma2_write((void*)((uint32_t)(src) & 0x1FFFFFFF),0xA8000000 + base,len);
		while (dma2_busy()){}
	} else {
		for (prg = 0;prg < len;) {
			const uint32_t step = ((prg + sizeof(dma_buffer)) > len) ? len - prg : sizeof(dma_buffer);
		    memcpy(dma_buffer,src,step);
		    data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,step);
		    while (dma2_busy()){}
		    PI_STATUS_REG = 2;
		    dma2_write((void*)((uint32_t)dma_buffer & 0x1FFFFFFF),0xA8000000 + base + prg,step);
			while (dma2_busy()){}
			prg += step;
			src += step;
		}
	}

	PI_BSD_DOM2_LAT_REG = piLatReg;
	PI_BSD_DOM2_PWD_REG = piPwdReg;
	PI_BSD_DOM2_PGS_REG = piPgsReg;
	PI_BSD_DOM2_RLS_REG = piRlsReg;
}

void ctl_sram_rd(void* p_dst,uint32_t base,uint32_t len) {
	uint32_t prg;
	uint8_t* dst = (uint8_t*)p_dst;

    vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
    vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
    vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
    vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;

    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

	data_cache_hit_writeback_invalidate((volatile void*)dst,len);
	if (!((uint32_t)dst & 7)) {	
		
		while (dma2_busy()){}
			PI_STATUS_REG = 3;
		    dma2_read((void*)((uint32_t) (dst) & 0x1FFFFFFF),0xA8000000 + base ,len);
		while (dma2_busy()){}
		 
	} else {
		for (prg = 0;prg < len;) {
			const uint32_t step = ((prg + sizeof(dma_buffer)) > len) ? len - prg : sizeof(dma_buffer);

		    data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,step);
		    while (dma2_busy()){}
		    PI_STATUS_REG = 3;
		    dma2_read((void*)((uint32_t)dma_buffer & 0x1FFFFFFF),0xA8000000 + base + prg,step);
			while (dma2_busy()){}
			data_cache_hit_invalidate((volatile void*)dma_buffer,step);
		    memcpy(dst, dma_buffer,step);
	 
			dst += step;
			prg += step;
		}
	}

    PI_BSD_DOM2_LAT_REG = piLatReg;
    PI_BSD_DOM2_PWD_REG = piPwdReg;
    PI_BSD_DOM2_PGS_REG = piPgsReg;
    PI_BSD_DOM2_RLS_REG = piRlsReg;
}
   

uint32_t ctl_rom_scan(const uint8_t* pattern,uint32_t base,uint32_t pattern_size,uint32_t rom_size) {
	const uint8_t* s = (const uint8_t*)(get_rom_space_addr() + base);
	const uint8_t* t = s + rom_size;

	//for mess emu test ~dont use on hw
	disable_interrupts();
	for (;s < t;++s) {

		uint32_t o = 0;
		const uint8_t* y = s,*z = pattern;
		while ((o < pattern_size) && (y < t)) {
			if (*(y++) != *(z++)) break;
			++o;
		}
		if (o == pattern_size) {
			enable_interrupts();
			return s- (const uint8_t*)(get_rom_space_addr() + base)  ;
		}
	}

	enable_interrupts();
	return 0;
}

uint32_t ctl_dma_rom_rd(void* p_dst,uint32_t base,uint32_t len) {
	uint8_t* dst = (uint8_t*)p_dst;
	uint32_t prg = 0;
	
	disable_interrupts();

	//TODO : read unaligned small block + read remainder as aligned 
	//data_cache_hit_writeback_invalidate((volatile void*)p_dst,len);
		for (;prg < len;) {
			uint32_t step = ((prg + sizeof(dma_buffer)) > len) ? len - prg : sizeof(dma_buffer);
			//Always do minimum reads of dma buffer size
    		data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,sizeof(dma_buffer));

			const unsigned long hw_addr = ((get_rom_space_addr() + base + prg) | 0x10000000) & 0x1FFFFFFF;

    		dma2_read((void *)(((uint32_t)dma_buffer) & 0x1FFFFFFF), hw_addr,sizeof(dma_buffer));
    		data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,sizeof(dma_buffer));
			memcpy(dst,dma_buffer,step);
			prg += step;
			dst += step;
		}
	

	enable_interrupts();

	return prg;
}

//base contains absolute adress within rom space
uint32_t ctl_dma_rom_rd_abs(void* p_dst,uint32_t base,uint32_t len) {
	uint8_t* dst = (uint8_t*)p_dst;
	uint32_t prg = 0;
	
	disable_interrupts();

	//TODO : read unaligned small block + read remainder as aligned 
	//data_cache_hit_writeback_invalidate((volatile void*)p_dst,len);
		for (;prg < len;) {
			uint32_t step = ((prg + sizeof(dma_buffer)) > len) ? len - prg : sizeof(dma_buffer);
			//Always do minimum reads of dma buffer size
    		data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,sizeof(dma_buffer));

			const unsigned long hw_addr = ((0xB0000000 + base + prg) | 0x10000000) & 0x1FFFFFFF;

    		dma2_read((void *)(((uint32_t)dma_buffer) & 0x1FFFFFFF), hw_addr ,sizeof(dma_buffer));
    		data_cache_hit_writeback_invalidate((volatile void*)dma_buffer,sizeof(dma_buffer));
			memcpy(dst,dma_buffer,step);
			prg += step;
			dst += step;
		}
	

	enable_interrupts();

	return prg;
}

#if 0
 
int test_sram(const uint32_t buf_sz) {
	uint8_t* buf = malloc(buf_sz);
	uint8_t* rbuf = malloc(buf_sz);

	uint32_t i;
	for (i = 0;i < buf_sz;++i) {
		rbuf[i] = 0;
		buf[i] = i & 0xff;
	}

	ctl_sram_wr(buf,0,buf_sz);
	ctl_sram_rd(rbuf,0,buf_sz);

	int r =  memcmp(buf,rbuf,buf_sz);
	
	free(buf);
	free(rbuf);
	return r == 0;
}
#endif

