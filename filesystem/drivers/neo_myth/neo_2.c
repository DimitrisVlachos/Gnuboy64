#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <n64sys.h>
#include "neo_2_asm.h"


typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile uint64_t vu64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef uint64_t u64;

// N64 hardware defines
#define PI_STATUS_REG       *(vu32*)0xA4600010
#define PI_BSD_DOM1_LAT_REG *(vu32*)0xA4600014
#define PI_BSD_DOM1_PWD_REG *(vu32*)0xA4600018
#define PI_BSD_DOM1_PGS_REG *(vu32*)0xA460001C
#define PI_BSD_DOM1_RLS_REG *(vu32*)0xA4600020
#define PI_BSD_DOM2_LAT_REG *(vu32*)0xA4600024
#define PI_BSD_DOM2_PWD_REG *(vu32*)0xA4600028
#define PI_BSD_DOM2_PGS_REG *(vu32*)0xA460002C
#define PI_BSD_DOM2_RLS_REG *(vu32*)0xA4600030

// V1.2-A hardware
#define MYTH_IO_BASE 0xA8040000
#define SAVE_IO   *(vu32*)(MYTH_IO_BASE | 0x00*2) // 0x00000000 = ext card save, 0x000F000F = off
#define CIC_IO    *(vu32*)(MYTH_IO_BASE | 0x04*2) // 0 = ext card CIC, 1 = 6101, 2 = 6102, 3 = 6103, 5 = 6105, 6 = 6106
#define ROM_BANK  *(vu32*)(MYTH_IO_BASE | 0x0C*2) // b3-0 = gba card A25-22 (8MB granularity)
#define ROM_SIZE  *(vu32*)(MYTH_IO_BASE | 0x08*2) // b3-0 = gba card A25-22 = masked N64 A25-22
#define RUN_IO    *(vu32*)(MYTH_IO_BASE | 0x20*2) // 0xFFFFFFFF = lock IO for game
#define INT_IO    *(vu32*)(MYTH_IO_BASE | 0x24*2) // 0xFFFFFFFF = enable multi-card mode
#define NEO_IO    *(vu32*)(MYTH_IO_BASE | 0x28*2) // 0xFFFFFFFF = 16 bit mode - read long from 0xB2000000 to 0xB3FFFFFF returns word
#define ROMC_IO   *(vu32*)(MYTH_IO_BASE | 0x30*2) // 0xFFFFFFFF = run card
#define ROMSW_IO  *(vu32*)(MYTH_IO_BASE | 0x34*2) // 0x00000000 = n64 menu at 0xB0000000 to 0xB1FFFFFF and gba card at 0xB2000000 to 0xB3FFFFFF
#define SRAM2C_I0 *(vu32*)(MYTH_IO_BASE | 0x36*2) // 0xFFFFFFFF = gba sram at 0xA8000000 to 0xA803FFFF (only when SAVE_IO = 5 or 6)
#define RST_IO    *(vu32*)(MYTH_IO_BASE | 0x38*2) // 0x00000000 = RESET to game, 0xFFFFFFFF = RESET to menu
#define CIC_EN    *(vu32*)(MYTH_IO_BASE | 0x3C*2) // 0x00000000 = CIC use default, 0xFFFFFFFF = CIC open
#define CPID_IO   *(vu32*)(MYTH_IO_BASE | 0x40*2) // 0x00000000 = CPLD ID off, 0xFFFFFFFF = CPLD ID one (0x81 = V1.2, 0x82 = V2.0, 0x83 = V3.0)
// V2.0 hardware
#define SRAM2C_IO *(vu32*)(MYTH_IO_BASE | 0x2C*2) // 0xFFFFFFFF = gba sram at 0xA8000000 to 0xA803FFFF (only when SAVE_IO = 5 or 6)

#define NEO2_RTC_OFFS 0x4000
#define NEO2_GTC_OFFS 0x4800
#define NEO2_CACHE_OFFS 0xC000          // offset of NEO2 inner cache into SRAM

#define SD_OFF 0x0000
#define SD_ON  0x0480

static u32 neo_mode = SD_OFF;

static u8 __attribute__((aligned(16))) dmaBuf[128*1024];

extern unsigned int gBootCic;
extern unsigned int gCardTypeCmd;
extern unsigned int gPsramCmd;
extern short int gCardType;
extern unsigned int gCpldVers;          /* 0x81 = V1.2 hardware, 0x82 = V2.0 hardware, 0x83 = V3.0 hardware */

extern unsigned int sd_speed;
extern unsigned int fast_flag;

//hack around get_cic since we don't need the module loading stuff in the player :P
static inline uint32_t get_cic(const volatile unsigned char* r) { return 0; }
//extern int get_cic(unsigned char *buffer);
u32 PSRAM_ADDR = 0;
int DAT_SWAP = 0;

void neo2_cycle_sd(void);



void hw_delay() 
{
   asm volatile
    (
        ".set   push\n"
        ".set   noreorder\n"
        "lui    $t0,0xb000\n"
        "lw     $zero,($t0)\n"
        "jr     $ra\n"
        "nop\n"
        ".set pop\n"
    );

}

#define _neo_asic_op(cmd) *(vu32 *)(0xB2000000 | (cmd<<1))

// do a Neo Flash ASIC command
unsigned short _neo_asic_cmd(unsigned int cmd, int full)
{
    unsigned int data;

    if (full)
    {
	MEMORY_BARRIER();
        // need to switch rom bus for ASIC operations
        INT_IO = 0xFFFFFFFF;            // enable multi-card mode
	MEMORY_BARRIER();
        hw_delay();
        ROMSW_IO = 0x00000000;          // gba mapped to 0xB2000000 - 0xB3FFFFFF
	MEMORY_BARRIER();
        hw_delay();
        ROM_BANK = 0x00000000;
	MEMORY_BARRIER();
        hw_delay();
        ROM_SIZE = 0x000C000C;          // bank size = 256Mbit
	MEMORY_BARRIER();
        hw_delay();
    }

    // at this point, assumes rom bus in a mode that allows ASIC operations

	MEMORY_BARRIER();
    NEO_IO = 0xFFFFFFFF;                // 16 bit mode
	MEMORY_BARRIER();
    hw_delay();

    /* do unlocking sequence */
	MEMORY_BARRIER();
    _neo_asic_op(0x00FFD200);
	MEMORY_BARRIER();
    _neo_asic_op(0x00001500);
	MEMORY_BARRIER();
    _neo_asic_op(0x0001D200);
	MEMORY_BARRIER();
    _neo_asic_op(0x00021500);
	MEMORY_BARRIER();
    _neo_asic_op(0x00FE1500);
	MEMORY_BARRIER();
    /* do ASIC command */
    data = _neo_asic_op(cmd);
	MEMORY_BARRIER();
    NEO_IO = 0x00000000;                // 32 bit mode
	MEMORY_BARRIER();
    hw_delay();

    return (unsigned short)(data & 0x0000FFFF);
}

unsigned int neo_id_card(void)
{
    unsigned int result, temp;

	MEMORY_BARRIER();
    vu32 *sram = (vu32 *)0xA8000000;

	MEMORY_BARRIER();
    // enable ID mode
    _neo_asic_cmd(0x00903500, 1);       // ID enabled


	MEMORY_BARRIER();
    // map GBA save ram into sram space
    SAVE_IO = 0x00050005;               // save off
	MEMORY_BARRIER();


	MEMORY_BARRIER();
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0xFFFFFFFF;         // enable gba sram
    else
        SRAM2C_IO = 0xFFFFFFFF;         // enable gba sram
    hw_delay();

	MEMORY_BARRIER();


    // read ID from sram space
    temp = sram[0];
    result = (temp & 0xFF000000) | ((temp & 0x0000FF00)<<8);
    temp = sram[1];
    result |= ((temp & 0xFF000000)>>16) | ((temp & 0x0000FF00)>>8);

    // disable ID mode
    _neo_asic_cmd(0x00904900, 1);       // ID disabled


	MEMORY_BARRIER();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram

	MEMORY_BARRIER();

    hw_delay();

    return result;
}

unsigned int neo_get_cpld(void)
{
    unsigned int data;

	MEMORY_BARRIER();
    INT_IO = 0xFFFFFFFF;                // enable multi-card mode
	MEMORY_BARRIER();
    hw_delay();

	MEMORY_BARRIER();
    CPID_IO = 0xFFFFFFFF;               // enable CPLD ID read
	MEMORY_BARRIER();
    hw_delay();
	MEMORY_BARRIER();
    data = *(vu32 *)0xB0000000;
	MEMORY_BARRIER();
    hw_delay();
	MEMORY_BARRIER();
    CPID_IO = 0x00000000;               // disable CPLD ID read
	MEMORY_BARRIER();
    hw_delay();
	MEMORY_BARRIER();
    return data;
}

void neo_select_menu(void)
{
    _neo_asic_cmd(0x00370002, 1);       // menu flash enabled
    _neo_asic_cmd(0x00DA0044, 0);       // select menu flash


	MEMORY_BARRIER();
    ROMSW_IO = 0xFFFFFFFF;              // gba mapped to 0xB0000000 - 0xB3FFFFFF
	MEMORY_BARRIER();
    hw_delay();

	MEMORY_BARRIER();
    ROM_SIZE = 0x00000000;              // bank size = 1Gbit
	MEMORY_BARRIER();

    hw_delay();
}

void neo_select_game(void)
{
    _neo_asic_cmd(0x00370202, 1);       // game flash enabled
    _neo_asic_cmd(gCardTypeCmd, 0);     // select game flash
    _neo_asic_cmd(0x00EE0630, 0);       // set cr1 = enable extended address bus


	MEMORY_BARRIER();
    ROMSW_IO = 0xFFFFFFFF;              // gba mapped to 0xB0000000 - 0xB3FFFFFF
	MEMORY_BARRIER();

    hw_delay();

	MEMORY_BARRIER();
    ROM_SIZE = 0x00000000;              // bank size = 1Gbit
	MEMORY_BARRIER();

    hw_delay();
}

void neo_select_psram(void)
{
    _neo_asic_cmd(0x00E21500, 1);       // GBA CARD WE ON !
    _neo_asic_cmd(0x00372302|neo_mode, 0); // game flash and write enabled, optionally enable SD interface
    _neo_asic_cmd(gPsramCmd, 0);        // select psram
    _neo_asic_cmd(0x00EE0630, 0);       // set cr1 = enable extended address bus


	MEMORY_BARRIER();
    ROMSW_IO = 0xFFFFFFFF;              // gba mapped to 0xB0000000 - 0xB3FFFFFF
	MEMORY_BARRIER();

    hw_delay();
	MEMORY_BARRIER();
    ROM_SIZE = 0x000C000C;              // bank size = 256Mbit (largest psram available)
	MEMORY_BARRIER();

    hw_delay();
}

void neo_psram_offset(int offs)
{
    _neo_asic_cmd(0x00C40000|offs, 1);  // set gba game flash offset


	MEMORY_BARRIER();
    ROMSW_IO = 0xFFFFFFFF;              // gba mapped to 0xB0000000 - 0xB3FFFFFF
	MEMORY_BARRIER();


    hw_delay();

	MEMORY_BARRIER();
    ROM_SIZE = 0x000C000C;              // bank size = 256Mbit (largest psram available)
	MEMORY_BARRIER();


    hw_delay();


	MEMORY_BARRIER();
    NEO_IO = 0xFFFFFFFF;                // 16 bit mode
	MEMORY_BARRIER();

    hw_delay();

    hw_delay();
}

void neo_copyfrom_game(void *dest, int fstart, int len)
{
    neo_select_game();                  // select game flash
#if 0
    // copy data
    for (int ix=0; ix<len; ix+=4)
        *(u32 *)(dest + ix) = *(vu32 *)(0xB0000000 + fstart + ix);
#else
    if ((u32)dest & 7)
    {
        // not properly aligned - DMA sram space to buffer, then copy it
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dmaBuf & 0x1FFFFFFF), 0xB0000000 + fstart, len);
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        // copy DMA buffer to dst
        memcpy(dest, dmaBuf, len);
    }
    else
    {
        // destination is aligned - DMA sram space directly to dst
        data_cache_hit_writeback_invalidate(dest, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dest & 0x1FFFFFFF), 0xB0000000 + fstart, len);
        data_cache_hit_writeback_invalidate(dest, len);
    }
#endif
}

void neo_copyfrom_menu(void *dest, int fstart, int len)
{
    neo_select_menu();                  // select menu flash
#if 0
    // copy data
    for (int ix=0; ix<len; ix+=4)
        *(u32 *)(dest + ix) = *(vu32 *)(0xB0000000 + fstart + ix);
#else
    if ((u32)dest & 7)
    {
        // not properly aligned - DMA sram space to buffer, then copy it
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dmaBuf & 0x1FFFFFFF), 0xB0000000 + fstart, len);
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        // copy DMA buffer to dst
        memcpy(dest, dmaBuf, len);
    }
    else
    {
        // destination is aligned - DMA sram space directly to dst
        data_cache_hit_writeback_invalidate(dest, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dest & 0x1FFFFFFF), 0xB0000000 + fstart, len);
        data_cache_hit_writeback_invalidate(dest, len);
    }
#endif
}

void neo_copyto_psram(void *src, int pstart, int len)
{
    neo_select_psram();                 // psram enabled and write-enabled
    neo_xferto_psram(src,pstart,len);
}

void neo_copyfrom_psram(void *dest, int pstart, int len)
{
    neo_select_psram();                 // psram enabled and write-enabled

    // copy data
    for (int ix=0; ix<len; ix+=4)
        *(u32 *)(dest + ix) = *(vu32 *)(0xB0000000 + pstart + ix);
}

void neo_copyto_sram(void *src, int sstart, int len)
{
    u32 temp;

    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    temp = (sstart & 0x00030000) >> 13;
    _neo_asic_cmd(0x00E00000|temp, 1);  // set gba sram offset

    // map GBA save ram into sram space
    SAVE_IO = 0x00050005;               // save off
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0xFFFFFFFF;         // enable gba sram
    else
        SRAM2C_IO = 0xFFFFFFFF;         // enable gba sram
    hw_delay();
    hw_delay();

    // Init the PI for sram
    vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
    vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
    vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
    vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;

    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

    // copy src to DMA buffer
    for (int ix=0; ix<len; ix++)
        dmaBuf[ix*2 + 1] = *(u8*)(src + ix);
    // DMA buffer to sram space
    data_cache_hit_writeback_invalidate(dmaBuf, len*2);
    while (dma_busy()) ;
    PI_STATUS_REG = 2;
    dma_write((void *)((u32)dmaBuf & 0x1FFFFFF8), 0xA8000000 + (sstart & 0xFFFF)*2, len*2);

    _neo_asic_cmd(0x00E00000, 1);       // clear gba sram offset

    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();

    PI_BSD_DOM2_LAT_REG = piLatReg;
    PI_BSD_DOM2_PWD_REG = piPwdReg;
    PI_BSD_DOM2_PGS_REG = piPgsReg;
    PI_BSD_DOM2_RLS_REG = piRlsReg;
    hw_delay();
    //neo_select_menu();
}

void neo_copyfrom_sram(void *dst, int sstart, int len)
{
    u32 temp;

    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    temp = (sstart & 0x00030000) >> 13;
    _neo_asic_cmd(0x00E00000|temp, 1);  // set gba sram offset

    // map GBA save ram into sram space
    SAVE_IO = 0x00050005;               // save off
    hw_delay();
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0xFFFFFFFF;         // enable gba sram
    else
        SRAM2C_IO = 0xFFFFFFFF;         // enable gba sram
    hw_delay();
    hw_delay();

    // Init the PI for sram
    vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
    vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
    vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
    vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;
    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

    // DMA sram space to buffer
    data_cache_hit_writeback_invalidate(dmaBuf, len*2);
    while (dma_busy()) ;
    PI_STATUS_REG = 3;
    dma_read((void *)((u32)dmaBuf & 0x1FFFFFF8), 0xA8000000 + (sstart & 0xFFFF)*2, len*2);
    data_cache_hit_writeback_invalidate(dmaBuf, len*2);
    // copy DMA buffer to dst
    for (int ix=0; ix<len; ix++)
        *(u8*)(dst + ix) = dmaBuf[ix*2 + 1];

    _neo_asic_cmd(0x00E00000, 1);       // clear gba sram offset

    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();

    PI_BSD_DOM2_LAT_REG = piLatReg;
    PI_BSD_DOM2_PWD_REG = piPwdReg;
    PI_BSD_DOM2_PGS_REG = piPgsReg;
    PI_BSD_DOM2_RLS_REG = piRlsReg;
    hw_delay();
}

void neo_copyto_nsram(void *src, int sstart, int len)
{
    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    SAVE_IO = 0x00080008;               // SRAM 256KB
    hw_delay();
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();
    hw_delay();

    // Init the PI for sram
    vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
    vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
    vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
    vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;
    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

    if ((u32)src & 7)
    {
        // source not properly aligned, copy src to DMA buffer, then DMA it
        memcpy(dmaBuf, src, len);
        // DMA buffer to sram space
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 2;
        dma_write((void *)((u32)dmaBuf & 0x1FFFFFFF), 0xA8000000 + sstart, len);
    }
    else
    {
        // source is aligned, DMA src directly to sram space
        data_cache_hit_writeback_invalidate(src, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 2;
        dma_write((void *)((u32)src & 0x1FFFFFFF), 0xA8000000 + sstart, len);
    }

    SAVE_IO = 0x00050005;               // save off
    hw_delay();

    PI_BSD_DOM2_LAT_REG = piLatReg;
    PI_BSD_DOM2_PWD_REG = piPwdReg;
    PI_BSD_DOM2_PGS_REG = piPgsReg;
    PI_BSD_DOM2_RLS_REG = piRlsReg;
    hw_delay();
}

void neo_copyfrom_nsram(void *dst, int sstart, int len)
{
    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    SAVE_IO = 0x00080008;               // SRAM 256KB
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();

    // Init the PI for sram
    vu32 piLatReg = PI_BSD_DOM2_LAT_REG;
    vu32 piPwdReg = PI_BSD_DOM2_PWD_REG;
    vu32 piPgsReg = PI_BSD_DOM2_PGS_REG;
    vu32 piRlsReg = PI_BSD_DOM2_RLS_REG;
    PI_BSD_DOM2_LAT_REG = 0x00000005;
    PI_BSD_DOM2_PWD_REG = 0x0000000C;
    PI_BSD_DOM2_PGS_REG = 0x0000000D;
    PI_BSD_DOM2_RLS_REG = 0x00000002;

    if ((u32)dst & 7)
    {
        // not properly aligned - DMA sram space to buffer, then copy it
        data_cache_hit_writeback_invalidate(dmaBuf, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dmaBuf & 0x1FFFFFFF), 0xA8000000 + sstart, len);
        // copy DMA buffer to dst
        memcpy(dst, dmaBuf, len);
    }
    else
    {
        // destination is aligned - DMA sram space directly to dst
        data_cache_hit_writeback_invalidate(dst, len);
        while (dma_busy()) ;
        PI_STATUS_REG = 3;
        dma_read((void *)((u32)dst & 0x1FFFFFFF), 0xA8000000 + sstart, len);
    }

    SAVE_IO = 0x00050005;               // save off
    hw_delay();

    PI_BSD_DOM2_LAT_REG = piLatReg;
    PI_BSD_DOM2_PWD_REG = piPwdReg;
    PI_BSD_DOM2_PGS_REG = piPgsReg;
    PI_BSD_DOM2_RLS_REG = piRlsReg;
    hw_delay();
}

void neo_copyto_eeprom(void *src, int sstart, int len, int mode)
{
#if 0
    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    SAVE_IO = (mode<<16) | mode;        // set EEPROM mode
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();

    for (int ix=0; ix<len; ix+=8)
    {
        unsigned long long data;
        memcpy((void*)&data, (void*)((u32)src + ix), 8);
        eeprom_write((sstart + ix)>>3, data);
    }

    SAVE_IO = 0x00050005;               // save off
    hw_delay();

    //neo_select_menu();
#endif
}

void neo_copyfrom_eeprom(void *dst, int sstart, int len, int mode)
{
#if 0
    neo2_cycle_sd();
    //neo_select_menu();
    neo_select_game();
    hw_delay();

    SAVE_IO = (mode<<16) | mode;        // set EEPROM mode
    hw_delay();
    if ((gCpldVers & 0x0F) == 1)
        SRAM2C_I0 = 0x00000000;         // disable gba sram
    else
        SRAM2C_IO = 0x00000000;         // disable gba sram
    hw_delay();

    for (int ix=0; ix<len; ix+=8)
    {
        unsigned long long data;
        data = eeprom_read((sstart + ix)>>3);
        memcpy((void*)((u32)dst + ix), (void*)&data, 8);
    }

    SAVE_IO = 0x00050005;               // save off
    hw_delay();

    //neo_select_menu();
#endif
}

void neo_get_rtc(unsigned char *rtc)
{
}

void neo2_enable_sd(void)
{
	MEMORY_BARRIER();
    neo_mode = SD_ON;
	MEMORY_BARRIER();
    neo_select_psram();

	MEMORY_BARRIER();
    NEO_IO = 0xFFFFFFFF;                // 16 bit mode
	MEMORY_BARRIER();
    hw_delay();
}

void neo2_disable_sd(void)
{
	MEMORY_BARRIER();
    neo_mode = SD_OFF;
	MEMORY_BARRIER();

    neo_select_psram();

	MEMORY_BARRIER();
    NEO_IO = 0x00000000;                // 32 bit mode
	MEMORY_BARRIER();

    hw_delay();
}

void neo2_pre_sd(void)
{
  /*  if (!sd_speed && fast_flag)
    {
        // set the PI for myth sd
        PI_BSD_DOM1_LAT_REG = 0x00000000;
        PI_BSD_DOM1_RLS_REG = 0x00000000;
        PI_BSD_DOM1_PWD_REG = 0x00000003;
        PI_BSD_DOM1_PGS_REG = 0x00000000;
    }*/


    if (!sd_speed && fast_flag)
    {
        // set the PI for myth sd
	MEMORY_BARRIER();
        PI_BSD_DOM1_LAT_REG = 0x00000003; // fasest safe speed = 3/2/3/7
	MEMORY_BARRIER();
        PI_BSD_DOM1_RLS_REG = 0x00000002;
	MEMORY_BARRIER();
        PI_BSD_DOM1_PWD_REG = 0x00000003;
	MEMORY_BARRIER();
        PI_BSD_DOM1_PGS_REG = 0x00000007;
	MEMORY_BARRIER();
    }

}

void neo2_post_sd(void)
{
    if (!sd_speed && fast_flag)
    {
	MEMORY_BARRIER();
        // restore the PI for rom
        PI_BSD_DOM1_LAT_REG = 0x00000040;
	MEMORY_BARRIER();
        PI_BSD_DOM1_RLS_REG = 0x00000003;
	MEMORY_BARRIER();
        PI_BSD_DOM1_PWD_REG = 0x00000012;
	MEMORY_BARRIER();
        PI_BSD_DOM1_PGS_REG = 0x00000007;
	MEMORY_BARRIER();
    }
}

void neo2_cycle_sd(void)
{
    hw_delay();

    if(neo_mode == SD_OFF)
    {
        neo2_enable_sd();
        hw_delay();
        hw_delay();
        neo2_disable_sd();
    }
    else
    {
        neo2_disable_sd();
        hw_delay();
        hw_delay();
        neo2_enable_sd();
    }

    hw_delay();
}

/*
int neo2_recv_sd_multi(unsigned char *buf, int count)
{
    int res;

    asm(".set push\n"
        ".set noreorder\n\t"
        "lui $15,0xB30E\n\t"            // $15 = 0xB30E0000
        "ori $14,%1,0\n\t"              // $14 = buf
        "ori $12,%2,0\n"                // $12 = count

        "oloop:\n\t"
        "lui $11,0x0001\n"              // $11 = timeout = 64 * 1024

        "tloop:\n\t"
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "andi $2,$2,0x0100\n\t"         // eqv of (data>>8)&0x01
        "beq $2,$0,getsect\n\t"         // start bit detected
        "nop\n\t"
        "addiu $11,$11,-1\n\t"
        "bne $11,$0,tloop\n\t"          // not timed out
        "nop\n\t"
        "beq $11,$0,exit\n\t"           // timeout
        "ori %0,$0,0\n"                 // res = FALSE

        "getsect:\n\t"
        "ori $13,$0,128\n"              // $13 = long count

        "gsloop:\n\t"
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4 => -a-- -a--
        "lui $10,0xF000\n\t"            // $10 = mask = 0xF0000000
        "sll $2,$2,4\n\t"               // a--- a---

        "lw $3,0x6060($15)\n\t"         // rdMmcDatBit4 => -b-- -b--
        "and $2,$2,$10\n\t"             // a000 0000
        "lui $10,0x0F00\n\t"            // $10 = mask = 0x0F000000
        "and $3,$3,$10\n\t"             // 0b00 0000

        "lw $4,0x6060($15)\n\t"         // rdMmcDatBit4 => -c-- -c--
        "lui $10,0x00F0\n\t"            // $10 = mask = 0x00F00000
        "or $11,$3,$2\n\t"              // $11 = ab00 0000
        "srl $4,$4,4\n\t"               // --c- --c-

        "lw $5,0x6060($15)\n\t"         // rdMmcDatBit4 => -d-- -d--
        "and $4,$4,$10\n\t"             // 00c0 0000
        "lui $10,0x000F\n\t"            // $10 = mask = 0x000F0000
        "srl $5,$5,8\n\t"               // ---d ---d
        "or $11,$11,$4\n\t"             // $11 = abc0 0000

        "lw $6,0x6060($15)\n\t"         // rdMmcDatBit4 => -e-- -e--
        "and $5,$5,$10\n\t"             // 000d 0000
        "ori $10,$0,0xF000\n\t"         // $10 = mask = 0x0000F000
        "sll $6,$6,4\n\t"               // e--- e---
        "or $11,$11,$5\n\t"             // $11 = abcd 0000

        "lw $7,0x6060($15)\n\t"         // rdMmcDatBit4 => -f-- -f--
        "and $6,$6,$10\n\t"             // 0000 e000
        "ori $10,$0,0x0F00\n\t"         // $10 = mask = 0x00000F00
        "or $11,$11,$6\n\t"             // $11 = abcd e000
        "and $7,$7,$10\n\t"             // 0000 0f00

        "lw $8,0x6060($15)\n\t"         // rdMmcDatBit4 => -g-- -g--
        "ori $10,$0,0x00F0\n\t"         // $10 = mask = 0x000000F0
        "or $11,$11,$7\n\t"             // $11 = abcd ef00
        "srl $8,$8,4\n\t"               // --g- --g-

        "lw $9,0x6060($15)\n\t"         // rdMmcDatBit4 => -h-- -h--
        "and $8,$8,$10\n\t"             // 0000 00g0
        "ori $10,$0,0x000F\n\t"         // $10 = mask = 0x000000F
        "or $11,$11,$8\n\t"             // $11 = abcd efg0

        "srl $9,$9,8\n\t"               // ---h ---h
        "and $9,$9,$10\n\t"             // 0000 000h
        "or $11,$11,$9\n\t"             // $11 = abcd efgh

        "sw $11,0($14)\n\t"             // save sector data
        "addiu $13,$13,-1\n\t"
        "bne $13,$0,gsloop\n\t"
        "addiu $14,$14,4\n\t"           // inc buffer pointer

        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4 - just toss checksum bytes
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4
        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4

        "lw $2,0x6060($15)\n\t"         // rdMmcDatBit4 - clock out end bit

        "addiu $12,$12,-1\n\t"          // count--
        "bne $12,$0,oloop\n\t"          // next sector
        "nop\n\t"

        "ori %0,$0,1\n"                 // res = TRUE

        "exit:\n"
        ".set pop\n"
        : "=r" (res)                    // output
        : "r" (buf), "r" (count)        // inputs
        : "$0" );                       // clobbered

    return res;
}
*/

//----------------------------------------------------------------------
//
//----------------------------------------------------------------------

#define CIC_6101 1
#define CIC_6102 2
#define CIC_6103 3
#define CIC_6104 4
#define CIC_6105 5
#define CIC_6106 6

// Simulated PIF ROM bootcode adapted from DaedalusX64 emulator
void simulate_pif_boot(u32 cic_chip)
{
    u32 ix;
    vu32 *src, *dst;
    u32 country = ((*(vu32 *)0xB000003C) >> 8) & 0xFF;
    vu64 *gGPR = (vu64 *)0xA0300000;

    /* Clear XBUS/Flush/Freeze */
    ((vu32 *)0xA4100000)[3] = 0x15;

    // clear some OS globals for cleaner boot
    *(vu32*)0xA000030C = 0;             // cold boot
    memset((void*)0xA000031C, 0, 64);   // clear app nmi buffer

    // copy the memsize for different boot loaders
    if ((cic_chip == CIC_6105) && (gBootCic != CIC_6105))
        *(vu32 *)0xA00003F0 = *(vu32 *)0xA0000318;
    else if ((cic_chip != CIC_6105) && (gBootCic == CIC_6105))
        *(vu32 *)0xA0000318 = *(vu32 *)0xA00003F0;

    // Copy low 0x1000 bytes to DMEM
    src = (vu32 *)0xB0000000;
    dst = (vu32 *)0xA4000000;
    for (ix=0; ix<(0x1000>>2); ix++)
        dst[ix] = src[ix];

    // Need to copy crap to IMEM for CIC-6105 boot.
    dst = (vu32 *)0xA4001000;

    // register values due to pif boot for CiC chip and country code, and IMEM crap

    gGPR[0]=0x0000000000000000LL;
    gGPR[6]=0xFFFFFFFFA4001F0CLL;
    gGPR[7]=0xFFFFFFFFA4001F08LL;
    gGPR[8]=0x00000000000000C0LL;
    gGPR[9]=0x0000000000000000LL;
    gGPR[10]=0x0000000000000040LL;
    gGPR[11]=0xFFFFFFFFA4000040LL;
    gGPR[16]=0x0000000000000000LL;
    gGPR[17]=0x0000000000000000LL;
    gGPR[18]=0x0000000000000000LL;
    gGPR[19]=0x0000000000000000LL;
    gGPR[21]=0x0000000000000000LL;
    gGPR[26]=0x0000000000000000LL;
    gGPR[27]=0x0000000000000000LL;
    gGPR[28]=0x0000000000000000LL;
    gGPR[29]=0xFFFFFFFFA4001FF0LL;
    gGPR[30]=0x0000000000000000LL;

    switch (country) {
        case 0x44: //Germany
        case 0x46: //french
        case 0x49: //Italian
        case 0x50: //Europe
        case 0x53: //Spanish
        case 0x55: //Australia
        case 0x58: // ????
        case 0x59: // X (PAL)
            switch (cic_chip) {
                case CIC_6102:
                    gGPR[5]=0xFFFFFFFFC0F1D859LL;
                    gGPR[14]=0x000000002DE108EALL;
                    gGPR[24]=0x0000000000000000LL;
                    break;
                case CIC_6103:
                    gGPR[5]=0xFFFFFFFFD4646273LL;
                    gGPR[14]=0x000000001AF99984LL;
                    gGPR[24]=0x0000000000000000LL;
                    break;
                case CIC_6105:
                    dst[0x04>>2] = 0xBDA807FC;
                    gGPR[5]=0xFFFFFFFFDECAAAD1LL;
                    gGPR[14]=0x000000000CF85C13LL;
                    gGPR[24]=0x0000000000000002LL;
                    break;
                case CIC_6106:
                    gGPR[5]=0xFFFFFFFFB04DC903LL;
                    gGPR[14]=0x000000001AF99984LL;
                    gGPR[24]=0x0000000000000002LL;
                    break;
            }

            gGPR[20]=0x0000000000000000LL;
            gGPR[23]=0x0000000000000006LL;
            gGPR[31]=0xFFFFFFFFA4001554LL;
            break;
        case 0x37: // 7 (Beta)
        case 0x41: // ????
        case 0x45: //USA
        case 0x4A: //Japan
        default:
            switch (cic_chip) {
                case CIC_6102:
                    gGPR[5]=0xFFFFFFFFC95973D5LL;
                    gGPR[14]=0x000000002449A366LL;
                    break;
                case CIC_6103:
                    gGPR[5]=0xFFFFFFFF95315A28LL;
                    gGPR[14]=0x000000005BACA1DFLL;
                    break;
                case CIC_6105:
                    dst[0x04>>2] = 0x8DA807FC;
                    gGPR[5]=0x000000005493FB9ALL;
                    gGPR[14]=0xFFFFFFFFC2C20384LL;
                case CIC_6106:
                    gGPR[5]=0xFFFFFFFFE067221FLL;
                    gGPR[14]=0x000000005CD2B70FLL;
                    break;
            }
            gGPR[20]=0x0000000000000001LL;
            gGPR[23]=0x0000000000000000LL;
            gGPR[24]=0x0000000000000003LL;
            gGPR[31]=0xFFFFFFFFA4001550LL;
    }

    switch (cic_chip) {
        case CIC_6101:
            gGPR[22]=0x000000000000003FLL;
            break;
        case CIC_6102:
            gGPR[1]=0x0000000000000001LL;
            gGPR[2]=0x000000000EBDA536LL;
            gGPR[3]=0x000000000EBDA536LL;
            gGPR[4]=0x000000000000A536LL;
            gGPR[12]=0xFFFFFFFFED10D0B3LL;
            gGPR[13]=0x000000001402A4CCLL;
            gGPR[15]=0x000000003103E121LL;
            gGPR[22]=0x000000000000003FLL;
            gGPR[25]=0xFFFFFFFF9DEBB54FLL;
            break;
        case CIC_6103:
            gGPR[1]=0x0000000000000001LL;
            gGPR[2]=0x0000000049A5EE96LL;
            gGPR[3]=0x0000000049A5EE96LL;
            gGPR[4]=0x000000000000EE96LL;
            gGPR[12]=0xFFFFFFFFCE9DFBF7LL;
            gGPR[13]=0xFFFFFFFFCE9DFBF7LL;
            gGPR[15]=0x0000000018B63D28LL;
            gGPR[22]=0x0000000000000078LL;
            gGPR[25]=0xFFFFFFFF825B21C9LL;
            break;
        case CIC_6105:
            dst[0x00>>2] = 0x3C0DBFC0;
            dst[0x08>>2] = 0x25AD07C0;
            dst[0x0C>>2] = 0x31080080;
            dst[0x10>>2] = 0x5500FFFC;
            dst[0x14>>2] = 0x3C0DBFC0;
            dst[0x18>>2] = 0x8DA80024;
            dst[0x1C>>2] = 0x3C0BB000;
            gGPR[1]=0x0000000000000000LL;
            gGPR[2]=0xFFFFFFFFF58B0FBFLL;
            gGPR[3]=0xFFFFFFFFF58B0FBFLL;
            gGPR[4]=0x0000000000000FBFLL;
            gGPR[12]=0xFFFFFFFF9651F81ELL;
            gGPR[13]=0x000000002D42AAC5LL;
            gGPR[15]=0x0000000056584D60LL;
            gGPR[22]=0x0000000000000091LL;
            gGPR[25]=0xFFFFFFFFCDCE565FLL;
            break;
        case CIC_6106:
            gGPR[1]=0x0000000000000000LL;
            gGPR[2]=0xFFFFFFFFA95930A4LL;
            gGPR[3]=0xFFFFFFFFA95930A4LL;
            gGPR[4]=0x00000000000030A4LL;
            gGPR[12]=0xFFFFFFFFBCB59510LL;
            gGPR[13]=0xFFFFFFFFBCB59510LL;
            gGPR[15]=0x000000007A3C07F4LL;
            gGPR[22]=0x0000000000000085LL;
            gGPR[25]=0x00000000465E3F72LL;
            break;
    }


    // set HW registers - PI_BSD_DOM1 regs, etc


    // now set MIPS registers - set CP0, and then GPRs, then jump thru gpr11 (which is 0xA400040)
    asm(".set noat\n\t"
        ".set noreorder\n\t"
        "li $8,0x34000000\n\t"
        "mtc0 $8,$12\n\t"
        "nop\n\t"
        "li $9,0x0006E463\n\t"
        "mtc0 $9,$16\n\t"
        "nop\n\t"
        "li $8,0x00005000\n\t"
        "mtc0 $8,$9\n\t"
        "nop\n\t"
        "li $9,0x0000005C\n\t"
        "mtc0 $9,$13\n\t"
        "nop\n\t"
        "li $8,0x007FFFF0\n\t"
        "mtc0 $8,$4\n\t"
        "nop\n\t"
        "li $9,0xFFFFFFFF\n\t"
        "mtc0 $9,$14\n\t"
        "nop\n\t"
        "mtc0 $9,$8\n\t"
        "nop\n\t"
        "mtc0 $9,$30\n\t"
        "nop\n\t"
        "lui $31,0xA030\n\t"
        "ld $1,0x08($31)\n\t"
        "ld $2,0x10($31)\n\t"
        "ld $3,0x18($31)\n\t"
        "ld $4,0x20($31)\n\t"
        "ld $5,0x28($31)\n\t"
        "ld $6,0x30($31)\n\t"
        "ld $7,0x38($31)\n\t"
        "ld $8,0x40($31)\n\t"
        "ld $9,0x48($31)\n\t"
        "ld $10,0x50($31)\n\t"
        "ld $11,0x58($31)\n\t"
        "ld $12,0x60($31)\n\t"
        "ld $13,0x68($31)\n\t"
        "ld $14,0x70($31)\n\t"
        "ld $15,0x78($31)\n\t"
        "ld $16,0x80($31)\n\t"
        "ld $17,0x88($31)\n\t"
        "ld $18,0x90($31)\n\t"
        "ld $19,0x98($31)\n\t"
        "ld $20,0xA0($31)\n\t"
        "ld $21,0xA8($31)\n\t"
        "ld $22,0xB0($31)\n\t"
        "ld $23,0xB8($31)\n\t"
        "ld $24,0xC0($31)\n\t"
        "ld $25,0xC8($31)\n\t"
        "ld $26,0xD0($31)\n\t"
        "ld $27,0xD8($31)\n\t"
        "ld $28,0xE0($31)\n\t"
        "ld $29,0xE8($31)\n\t"
        "ld $30,0xF0($31)\n\t"
        "ld $31,0xF8($31)\n\t"
        "jr $11\n\t"
        "nop"
        ::: "$8" );
}

/*
Address 0x1D0000 arrangement:
every 64 bytes is a item for one game, inside the 64 bytes:
[0] : game flag, 0xFF-> game's item,  0x00->end item
[1] : high byte of rom bank
[2] : low byte of rom bank
[3] : high byte of rom size
[4] : low byte of rom size
[5] : save value, which range from 0x00~0x06
[6] : game cic value, which range from 0x00~0x06
[7] : game mode value, which range from 0x00~0x0F
[8~63] : game name string

about the word of rombank and romsize:
  0M ROM_BANK   0 0 0 0  -> 0x0000
 64M ROM_BANK   0 0 0 1  -> 0x0001
128M ROM_BANK   0 0 1 0  -> 0x0002
256M ROM_BANK   0 1 0 0  -> 0x0004
512M ROM_BANK   1 0 0 0  -> 0x0008
  1G ROM_BANK   0 0 0 0  -> 0x0000 // only when ROM SIZE also que to zero

 64M ROM_SIZE   1 1 1 1  -> 0x000F
128M ROM_SIZE   1 1 1 0  -> 0x000E
256M ROM_SIZE   1 1 0 0  -> 0x000C
512M ROM_SIZE   1 0 0 0  -> 0x0008
  1G ROM_SIZE   0 0 0 0  -> 0x0000
*/


#define N64_MENU_ENTRY  (0xB0000000 + 0x1D0000)


void neo_run_menu(void)
{
    u32 mythaware;

    neo_select_menu();                  // select menu flash

    // set myth hw for selected rom
    ROM_BANK = 0x00000000;
    hw_delay();
    ROM_SIZE = 0x000F000F;
    hw_delay();
    SAVE_IO  = 0x00080008;
    hw_delay();
    CIC_IO   = 0x00020002;
    hw_delay();
    RST_IO   = 0xFFFFFFFF;
    hw_delay();
    mythaware = !memcmp((void *)0xB0000020, "N64 Myth", 8);
    RUN_IO   = mythaware ? 0x00000000 : 0xFFFFFFFF;
    hw_delay();

    // start cart
    disable_interrupts();
    simulate_pif_boot(2);               // should never return
    enable_interrupts();
}

void neo_run_game(u8 *option, int reset)
{
    u32 rombank=0;
    u32 romsize=0;
    u32 romsave=0;
    u32 romcic=0;
    u32 rommode=0;
    u32 gamelen;
    u32 runcic;
    u32 mythaware;

    rombank = (option[1]<<8) + option[2];
    if (rombank > 16)
        rombank = rombank / 64;
    rombank += (rombank<<16);

    romsize = (option[3]<<8) + option[4];
    romsize += (romsize<<16);

    romsave = (option[5]<<16) + option[5];
    romcic  = (option[6]<<16) + option[6];
    rommode = (option[7]<<16) + option[7];

    if ((romsize & 0xFFFF) > 0x000F)
    {
        // romsize is number of Mbits in rom
        gamelen = (romsize & 0xFFFF)*128*1024;
        if (gamelen <= (8*1024*1024))
            romsize = 0x000F000F;
        else if (gamelen <= (16*1024*1024))
            romsize = 0x000E000E;
        else if (gamelen <= (32*1024*1024))
            romsize = 0x000C000C;
        else if (gamelen <= (64*1024*1024))
            romsize = 0x00080008;
        else
            romsize = 0x00000000;
    }

    neo_select_game();                  // select game flash

    // set myth hw for selected rom
    ROM_BANK = rombank;
    hw_delay();
    ROM_SIZE = romsize;
    hw_delay();
    SAVE_IO  = romsave;
    hw_delay();
    // if set to use card cic, figure out cic for simulated start
    if (romcic == 0)
        runcic = get_cic((volatile unsigned char *)0xB0000040);
    else
        runcic = romcic & 7;
    if (!reset && (runcic != 2))
        reset = 1;                      // reset back to menu since cannot reset to game in hardware
    CIC_IO   = romcic; //reset ? 0x00020002 : romcic;
    hw_delay();
    RST_IO = reset ? 0xFFFFFFFF : 0x00000000;
    hw_delay();
    mythaware = !memcmp((void *)0xB0000020, "N64 Myth", 8);
    RUN_IO   = mythaware ? 0x00000000 : 0xFFFFFFFF;
    hw_delay();

    // start cart
    disable_interrupts();
    simulate_pif_boot(runcic);          // should never return
    enable_interrupts();
}

void neo_run_psram(u8 *option, int reset)
{
    u32 romsize=0;
    u32 romsave=0;
    u32 romcic=0;
    u32 rommode=0;
    u32 gamelen;
    u32 runcic;
    u32 mythaware;

    romsize = (option[3]<<8) + option[4];
    romsize += (romsize<<16);

    romsave = (option[5]<<16) + option[5];
    romcic  = (option[6]<<16) + option[6];
    rommode = (option[7]<<16) + option[7];

    if ((romsize & 0xFFFF) > 0x000F)
    {
        // romsize is number of Mbits in rom
        gamelen = (romsize & 0xFFFF)*128*1024;
        if (gamelen <= (8*1024*1024))
            romsize = 0x000F000F;
        else if (gamelen <= (16*1024*1024))
            romsize = 0x000E000E;
        else if (gamelen <= (32*1024*1024))
            romsize = 0x000C000C;
        else if (gamelen <= (64*1024*1024))
            romsize = 0x00080008;
        else
            romsize = 0x00000000;
    }

    neo_mode = SD_OFF;                  // make sure SD card interface is disabled
    neo_select_psram();                 // select gba psram
    _neo_asic_cmd(0x00E2D200, 1);       // GBA CARD WE OFF !

    // set myth hardware for selected rom
    ROMSW_IO = 0xFFFFFFFF;              // gba mapped to 0xB0000000 - 0xB3FFFFFF
    hw_delay();
    ROM_BANK = 0x00000000;
    hw_delay();
    ROM_SIZE = romsize;
    hw_delay();
    SAVE_IO  = romsave;
    hw_delay();
    // if set to use card cic, figure out cic for simulated start
    if (romcic == 0)
        runcic = get_cic((unsigned char *)0xB0000040);
    else
        runcic = romcic & 7;
    if (!reset && (runcic != 2))
        reset = 1;                      // reset back to menu since cannot reset to game in hardware
    CIC_IO   = romcic; //reset ? 0x00020002 : romcic;
    hw_delay();
    RST_IO = reset ? 0xFFFFFFFF : 0x00000000;
    hw_delay();
    mythaware = !memcmp((void *)0xB0000020, "N64 Myth", 8);
    RUN_IO   = mythaware ? 0x00000000 : 0xFFFFFFFF;
    hw_delay();

    // start cart
    disable_interrupts();
    simulate_pif_boot(runcic);          // should never return
    enable_interrupts();
}

