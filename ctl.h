#ifndef _ctl_h_
#define _ctl_h_
#include <stdint.h>
void ctl_build_initial_state_block();
void ctl_set_signature(const char* sig);
 
int ctl_read_sram(void* dst,int len);
int ctl_read_rtc(void* dst,int len);
void ctl_write_sram(const void* src,int len);
void ctl_write_rtc(const void* src,int len);
uint32_t ctl_dma_rom_rd(void* p_dst,uint32_t base,uint32_t len);
uint32_t ctl_dma_rom_rd_abs(void* p_dst,uint32_t base,uint32_t len);
int ctl_dbg_cmp_sram(const void* src,int len);
void ctl_sram_wr(void* p_src,uint32_t base,uint32_t len);
void ctl_sram_rd(void* p_dst,uint32_t base,uint32_t len);
void ctl_write_state(const void* src,int len,int block);
void ctl_read_state(void* dst,int len,int block);
uint32_t ctl_rom_scan(const uint8_t* pattern,uint32_t base,uint32_t pattern_size,uint32_t rom_size);
 
#endif

