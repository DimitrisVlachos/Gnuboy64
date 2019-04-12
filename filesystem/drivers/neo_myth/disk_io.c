
#include <string.h>
#include <stdio.h>
#include "neo_2_asm.h"
#include <diskio.h>
 
typedef unsigned int u32;
typedef volatile unsigned int vu32;

#define SD_CRC_SIZE (512)
#define CACHE_SIZE (16)                   /* number sectors in cache */
#define R1_LEN (48/8)
#define R2_LEN (136/8)
#define R3_LEN (48/8)
#define R6_LEN (48/8)
#define R7_LEN (48/8)
#define INIT_RETRIES (64)
#define RESP_TIME_R (1024/2)
#define RESP_TIME_W (1024*4)
//#define DEBUG_PRINT
//#define DEBUG_RESP

/*Fast asm multiblock routines*/
extern int neo2_recv_sd_multi(unsigned char *buf, int count);
extern int neo2_recv_sd_multi_psram_no_ds(unsigned char *buf, int count);
extern int neo2_recv_sd_psram_ds1(unsigned char *buf, int count);
 
static int (*sdReadMultiBlocks)(unsigned char *buf, int count) = neo2_recv_sd_multi;

extern void neo2_pre_sd(void);
extern void neo2_post_sd(void);
#ifdef DEBUG_RESP
extern void debugText(char *msg, int x, int y, int d);
#endif
/* global variables and arrays */
unsigned short cardType;                /* b0 = block access, b1 = V2 and/or HC, b15 = funky read timing */
unsigned char pkt[6];                    /* command packet */
unsigned int num_sectors;
unsigned int sd_speed;
unsigned int sec_tags[CACHE_SIZE];
unsigned char __attribute__((aligned(16))) sec_cache[CACHE_SIZE*512 + 8];
unsigned char __attribute__((aligned(16))) sec_buf[520]; /* for uncached reads */
unsigned char __attribute__((aligned(16))) resp_buf[32];
unsigned char sd_csd[R2_LEN];
static int respTime = RESP_TIME_R;
//int DISK_IO_MODE = 0;
//extern int DAT_SWAP;
//extern u32 PSRAM_ADDR;
/*
+  Polynomial = 0x89 (2^7 + 2^3 + 1)
+  width      = 7 bit
+*/

static const unsigned char __attribute__((aligned(16))) crc7_table[256] =
{
    0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
    0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
    0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
    0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
    0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
    0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
    0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
    0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
    0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
    0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
    0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
    0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
    0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
    0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
    0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
    0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
    0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
    0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
    0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
    0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
    0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
    0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
    0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
    0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
    0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
    0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
    0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
    0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
    0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
    0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
    0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
    0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

/*-----------------------------------------------------------------------*/
/*                             support code                              */
/*-----------------------------------------------------------------------*/
 


void disk_io_set_mode(int mode,int dat_swap)
{
	extern u32 PSRAM_ADDR;

	PSRAM_ADDR = 0;

	if(mode == 0)
	{
		sdReadMultiBlocks = neo2_recv_sd_multi;
		return;
	}
	else
	{
		switch(dat_swap)
		{
			case 0: sdReadMultiBlocks = neo2_recv_sd_multi_psram_no_ds; break;
			case 1: sdReadMultiBlocks = neo2_recv_sd_psram_ds1; break;
		}
	}
}

inline void sd_delay(int cnt)
{
	//cnt <<= ((cnt == 1600)<< 1) << 2;//init cnt?

    for(int ix=0; ix<cnt; ix++)
        asm("\tnop\n"::);
}

inline void wrMmcCmdBit( unsigned int bit )
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( (bit&1)<<13 ) |      // cmd value
        ( 1<<12 ) |            // d3 value
        ( 1<<11 ) |            // d2 value
        ( 1<<10 ) |            // d1 value
        ( 1<<9 ));             // d0 value
    data = *addr;
    sd_delay(sd_speed);
}

inline void wrMmcCmdByte( unsigned int byte )
{
    wrMmcCmdBit((byte>>7) & 1);
    wrMmcCmdBit((byte>>6) & 1);
    wrMmcCmdBit((byte>>5) & 1);
    wrMmcCmdBit((byte>>4) & 1);
    wrMmcCmdBit((byte>>3) & 1);
    wrMmcCmdBit((byte>>2) & 1);
    wrMmcCmdBit((byte>>1) & 1);
    wrMmcCmdBit((byte) & 1);
}

#if 1
inline void wrMmcDatBit( unsigned int bit )
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3 value
        ( 1<<11 ) |            // d2 value
        ( 1<<10 ) |            // d1 value
        ( (bit&1)<<9 ));       // d0 value
    data = *addr;
    sd_delay(sd_speed);
}

inline void wrMmcDatByte( unsigned int byte )
{
    wrMmcDatBit((byte>>7) & 1);
    wrMmcDatBit((byte>>6) & 1);
    wrMmcDatBit((byte>>5) & 1);
    wrMmcDatBit((byte>>4) & 1);
    wrMmcDatBit((byte>>3) & 1);
    wrMmcDatBit((byte>>2) & 1);
    wrMmcDatBit((byte>>1) & 1);
    wrMmcDatBit((byte>>0) & 1);
}
#endif

inline unsigned int rdMmcCmdBit()
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3
        ( 1<<11 ) |            // d2
        ( 1<<10 ) |            // d1
        ( 1<<9 )  |            // d0
        ( 1<<7 ));             // cmd input mode
    data = *addr;
    sd_delay(sd_speed);
    return (data>>12) & 1;
}

/*
inline unsigned char rdMmcCmdBits( int num )
{
    register unsigned char byte = 0;

    for(int i=0; i<num; i++)
        byte = (byte << 1) | rdMmcCmdBit();

    return byte;
}*/

inline unsigned char rdMmcCmdByte()
{
    register unsigned char byte;

    byte = rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();

    return byte;
}

inline unsigned char rdMmcCmdByte7()
{
    register unsigned char byte;

    byte = rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();
    byte = (byte << 1) | rdMmcCmdBit();

    return byte;
}

inline unsigned int rdMmcDatBit()
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        //( 1<<15 ) |          // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( 1<<12 ) |            // d3
        ( 1<<11 ) |            // d2
        ( 1<<10 ) |            // d1
        //( 1<<9 ) |           // d0
        //( 1<<6 ) |           // d3-d1 input (only needed in 4 bit mode)
        ( 1<<5 ));              // d0 input
    data = *addr;
    sd_delay(sd_speed);
    return (data>>8) & 1;
}

inline unsigned int rdMmcDatBit4()
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        //( 1<<16 ) |          // d1-d3 remap
        //( 1<<15 ) |          // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        //( 1<<12 ) |          // d3
        //( 1<<11 ) |          // d2
        //( 1<<10 ) |          // d1
        //( 1<<9 ) |           // d0
        ( 1<<6 ) |             // d3-d1 input (only needed in 4 bit mode)
        ( 1<<5 ));             // d0 input
    data = *addr;
    sd_delay(sd_speed);
    return (data>>8) & 15;
}

#if 1
inline void wrMmcDatBit4( unsigned char dat )
{
    unsigned int data;
    vu32 *addr = (vu32*)
       (( 0xB2000000 ) |       // GBA cart space
        ( 1<<24 ) |            // always 1 (set in GBAC_LIO)
        ( 1<<19 ) |            // clk enable
        ( 1<<18 ) |            // cs remap
        ( 1<<17 ) |            // cmd remap
        ( 1<<16 ) |            // d1-d3 remap
        ( 1<<15 ) |            // d0 remap
        ( 1<<14 ) |            // cs value (active high)
        ( 1<<13 ) |            // cmd value
        ( (dat&15)<<9 ));      // d3-d0 value
    data = *addr;
    sd_delay(sd_speed);
}

inline void wrMmcDatByte4( unsigned char val )
{
    wrMmcDatBit4((val >> 4) & 15);
    wrMmcDatBit4(val & 15);
}
#endif

// 4 to 8 bits please

inline unsigned char rdMmcDatByte4()
{
    return ( (rdMmcDatBit4() << 4) | rdMmcDatBit4() );
}

// 1 to 8 bits please
inline unsigned char rdMmcDatByte()
{
    register unsigned char byte;

    byte = /*(byte << 1) | */rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();
    byte = (byte << 1) | rdMmcDatBit();

    return byte;
}

unsigned char crc7(register unsigned char *buf)
{
    register unsigned char crc = 0;

    crc = crc7_table[(crc) ^ buf[0]];
    crc = crc7_table[(crc << 1) ^ buf[1]];
    crc = crc7_table[(crc << 1) ^ buf[2]];
    crc = crc7_table[(crc << 1) ^ buf[3]];
    crc = crc7_table[(crc << 1) ^ buf[4]];

    return crc;
}

#ifdef DEBUG_RESP
void debugMmcResp( unsigned char cmd, unsigned char *resp )
{
    char temp[44];
    snprintf(temp, 40, "CMD%u: %02X %02X %02X %02X %02X %02X", cmd, resp[0], resp[1], resp[2], resp[3], resp[4], resp[5]);
    debugText(temp, -1, -1, 60);
}
#else
#define debugMmcResp(x, y)
#endif

#ifdef DEBUG_PRINT
void debugMmcPrint( char *str )
{
    char temp[44];
    snprintf(temp, 40, "%s", str);
    debugText(temp, -1, -1, 60);
}
#else
#define debugMmcPrint(x)
#endif

void debugPrint( char *str )
{
	#ifdef DEBUG_RESP
    char temp[44];
    static int dbgX = 0;
    static int dbgY = 0;
    snprintf(temp, 40, "%s", str);
    debugText(temp, dbgX, dbgY, 20);
    dbgY++;
    if (dbgY > 27)
    {
        dbgY = 0;
        dbgX = (dbgX + 8) % 40;
    }
	#endif
}

inline void sendMmcCmd( unsigned char cmd, unsigned int arg )
{
    pkt[0]=cmd|0x40;                    // b7 = 0 => start bit, b6 = 1 => host command
    pkt[1]=(unsigned char)(arg >> 24);
    pkt[2]=(unsigned char)(arg >> 16);
    pkt[3]=(unsigned char)(arg >> 8);
    pkt[4]=(unsigned char)(arg);
    pkt[5]= (crc7(pkt) << 1) | 1;        // b0 = 1 => stop bit
//    wrMmcCmdByte(0xFF);
//    wrMmcCmdByte(0xFF);
    wrMmcCmdByte( pkt[0] );
    wrMmcCmdByte( pkt[1] );
    wrMmcCmdByte( pkt[2] );
    wrMmcCmdByte( pkt[3] );
    wrMmcCmdByte( pkt[4] );
    wrMmcCmdByte( pkt[5] );
}

int recvMmcCmdResp( unsigned char *resp, unsigned int len, int cflag )
{
    register unsigned int i;
    register unsigned char* r = resp;

    for (i=0; i<respTime; i++)
    {
        // wait on start bit
        if(rdMmcCmdBit()==0)
        {
            *r++ = rdMmcCmdByte7();//rdMmcCmdBits(7);
            
            while(--len)
                *r++ = rdMmcCmdByte();

            if (cflag)
                wrMmcCmdByte(0xFF);     // 8 cycles to complete the operation so clock can halt

            respTime = RESP_TIME_R;
            debugMmcResp(pkt[0]&0x3F, resp);
            return 1;
        }
    }

    respTime = RESP_TIME_R;
    //debugPrint("recvMmcCmdResp() failed");
    debugMmcResp(pkt[0]&0x3F, resp);
    return 0;
}

int sdReadSingleBlock( unsigned char *buf, unsigned int addr )
{
   // int i = 8*1024;
	register unsigned char* resp = resp_buf;

    debugMmcPrint("Read starting");

    sendMmcCmd(17, addr);               // READ_SINGLE_BLOCK
    if (!(cardType & 0x8000))
    {
        if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 17))
            return 0;
    }

#if 0
    while ((rdMmcDatBit4()&1) != 0)
    {
        if (i-- <= 0)
        {
            debugMmcPrint("Timeout");
            return 0;               // timeout on start bit
        }
    }

    for (i=0; i<SD_CRC_SIZE; i++)
        buf[i] = rdMmcDatByte4();

    // Clock out CRC
    for (i=0; i<8; i++)
        rdMmcDatByte4();

    // Clock out end bit
    rdMmcDatBit4();
#else
    neo2_recv_sd_multi(buf,1);
#endif
    wrMmcCmdByte(0xFF);                 // 8 cycles to complete the operation so clock can halt

    debugMmcPrint("Read SD block");
    return 1;
}

inline int sdReadStartMulti( unsigned int addr )
{
	register unsigned char* resp = resp_buf;

    debugMmcPrint("Read starting");

    sendMmcCmd(18, addr);               // READ_MULTIPLE_BLOCK
    if (!(cardType & 0x8000))
    {
        if (!recvMmcCmdResp(resp, R1_LEN, 0) || (resp[0] != 18))
            return 0;
    }

    return 1;
}

/*
inline int sdReadMultiBlocks(BYTE* buff,BYTE count)
{
	if(DISK_IO_MODE == 0)
		return neo2_recv_sd_multi(buff,count);
	else
	{
		switch(DAT_SWAP)
		{
			case 0: return neo2_recv_sd_multi_psram_no_ds(buff,count);
			case 1: return neo2_recv_sd_psram_ds1(buff,count);
		}
	}

	return neo2_recv_sd_multi(buff,count);
}*/

/*
int sdReadMultiBlocks( BYTE *buff, BYTE count )
{
#if 0
    int i, j, k;
    for (j=0, k=0; j<count; j++, k+=512)
    {
        i = 64*1024;
        while ((rdMmcDatBit4()&1) != 0)
            if (i-- <= 0)
            {
                debugMmcPrint("Timeout");
                return 0;           // timeout on start bit
            }

        for (i=0; i<512; i+=4)
            *(u32*)&buff[i+k] = (rdMmcDatByte4()<<24)|(rdMmcDatByte4()<<16)|(rdMmcDatByte4()<<8)|rdMmcDatByte4();

        // Clock out CRC
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();
        rdMmcDatByte4();

        // Clock out end bit
        rdMmcDatBit4();
    }
    return 1;
#else
    return neo2_recv_sd_multi(buff, count);
#endif
}*/

inline int sdReadStopMulti( void )
{
	register unsigned char* resp = resp_buf;

    debugMmcPrint("Read stopping");

    sendMmcCmd(12, 0);                  // STOP_TRANSMISSION
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 12))
        return 0;

    return 1;
}

void sdCrc16(unsigned char *p_crc, unsigned char *data, int len)
{
    int i;
    unsigned char nybble;

    unsigned long long poly = 0x0001000000100001LL;
    unsigned long long crc = 0;
    unsigned long long n_crc; // This can probably be unsigned int

    // Load crc from array
    for (i = 0; i < 8; i++)
    {
        crc <<= 8;
        crc |= p_crc[i];
    }

    for (i = 0; i < (len * 2); i++)
    {
        if (i & 1)
            nybble = (data[i >> 1] & 0x0F);
        else
            nybble = (data[i >> 1] >> 4);

        n_crc = (crc >> (15 * 4));
        crc <<= 4;
        if ((nybble ^ n_crc) & 1) crc ^= (poly << 0);
        if ((nybble ^ n_crc) & 2) crc ^= (poly << 1);
        if ((nybble ^ n_crc) & 4) crc ^= (poly << 2);
        if ((nybble ^ n_crc) & 8) crc ^= (poly << 3);
    }

    // Output crc to array
    for (i = 7; i >= 0; i--)
    {
        p_crc[i] = crc;
        crc >>= 8;
    }
}

int sendSdWriteBlock4( unsigned char *buf, unsigned char *crcbuf )
{
    int i;

    //for (i = 0; i < 20; i++)
    wrMmcDatByte4(0xFF);                // minimum 2 P bits
    wrMmcDatBit4(0);                    // write start bit

    // write out data and crc bytes
    for (i=0; i<SD_CRC_SIZE; i++)
        wrMmcDatByte4( buf[i] );

    for (i=0; i<8; i++)
        wrMmcDatByte4( crcbuf[i] );

    wrMmcDatBit4(15);                   // write end bit

    rdMmcDatByte4();                    // clock out two bits on D0

#if 1
    // spec says the CRC response from the card appears now...
    //   start bit, three CRC response bits, end bit
    // this is followed by a busy response while the block is written
    //   start bit, variable number of 0 bits, end bit

    if( rdMmcDatBit4() & 1 )
    {
        debugPrint("No start bit after write data block.");
        return 0;
    }
    else
    {
        unsigned char crc_stat = 0;

        for(i=0; i<3; i++)
        {
            crc_stat <<= 1;
            crc_stat |= (rdMmcDatBit4() & 1);   // read crc status on D0
        }
        rdMmcDatBit4();                 // end bit

        // If CRC Status is not positive (010b) return error
        if(crc_stat!=2)
        {
            debugPrint("CRC error on write data block.");
            return 0;
        }

        // at this point the card is definetly cooperating so wait for the start bit
        while( rdMmcDatBit4() & 1 );

        // Do not time out? Writes take an unpredictable amount of time to complete.
        for(i=(1024*128);i>0;i--)
        {
            if((rdMmcDatBit4()&1) != 0 )
                break;
        }
        // check for busy timeout
        if(i==0)
        {
            debugPrint("Write data block busy timeout.");
            return 0;
        }
    }
#else
    // ignore the spec - lets just clock out some bits and wait
    // for the busy to clear...it works well on many non working cards

    rdMmcDatByte4();                    // clock out two bits on D0
    rdMmcDatByte4();                    // clock out two bits on D0

    while(rdMmcDatBit4() & 1) ;         // wait for DAT line to go low, indicating busy writing
    while((rdMmcDatBit4()&1) == 0) ;    // wait for DAT line high, indicating finished writing block
#endif

    wrMmcCmdByte(0xFF);                 // 8 cycles to complete the operation so clock can halt
    debugMmcPrint("Write done");
    return 1;
}

int sdWriteSingleBlock( unsigned char *buf, unsigned int addr )
{
    unsigned char resp[R1_LEN];
    unsigned char crcbuf[8];

    //memset(crcbuf, 0, 8);             // crc in = 0
    *(unsigned int*)&crcbuf[0]=0;
    *(unsigned int*)&crcbuf[4]=0;

    sdCrc16(crcbuf, buf, SD_CRC_SIZE);          // Calculate CRC16

    sendMmcCmd( 24, addr );
    respTime = RESP_TIME_W;
    if (recvMmcCmdResp(resp, R1_LEN, 0) && (resp[0] == 24))
        return sendSdWriteBlock4(buf, crcbuf);

    debugMmcResp(24, resp);
    return 0;
}

int sdInit(void)
{
    int i;
    unsigned short rca;
    unsigned char resp[R2_LEN];         // R2 is largest response

	disk_io_set_mode(0,0);

    respTime = RESP_TIME_R;

    for (i=0; i<128; i++)
        wrMmcCmdBit(1);

    sendMmcCmd(0, 0);                   // GO_IDLE_STATE
    wrMmcCmdByte(0xFF);                 // 8 cycles to complete the operation so clock can halt

    sendMmcCmd(8, 0x1AA);               // SEND_IF_COND
    if (recvMmcCmdResp(resp, R7_LEN, 1))
    {
        if ((resp[0] == 8) && (resp[3] == 1) && (resp[4] == 0xAA))
            cardType |= 2;              // V2 and/or HC card
        else
            return 0;                   // unusable
    }

    for (i=0; i<INIT_RETRIES; i++)
    {
        sendMmcCmd(55, 0xFFFF);         // APP_CMD
        if (recvMmcCmdResp(resp, R1_LEN, 1) && (resp[4] & 0x20))
        {
            sendMmcCmd(41, (cardType & 2) ? 0x40300000 : 0x00300000);    // SD_SEND_OP_COND
            if (recvMmcCmdResp(resp, R3_LEN, 1) && (resp[0] == 0x3F) && (resp[1] & 0x80))
            {
                if (resp[1] & 0x40)
                    cardType |= 1;      // HC card
                if (!(resp[2] & 0x30))
                    return 0;           // unusable
                break;
            }
        }
    }
    if (i == INIT_RETRIES)
    {
        // timed out
        return 0;                       // unusable
    }

    sendMmcCmd(2, 0xFFFFFFFF);          // ALL_SEND_CID
    if (!recvMmcCmdResp(resp, R2_LEN, 1) || (resp[0] != 0x3F))
        return 0;                       // unusable

    sendMmcCmd(3, 1);                   // SEND_RELATIVE_ADDR
    if (!recvMmcCmdResp(resp, R6_LEN, 1) || (resp[0] != 3))
        return 0;                       // unusable
    rca = (resp[1]<<8) | resp[2];

#if 1
    sendMmcCmd(9, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // SEND_CSD
    recvMmcCmdResp(sd_csd, R2_LEN, 1);
#endif

    sendMmcCmd(7, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // SELECT_DESELECT_CARD
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 7))
        return 0;                       // unusable

    sendMmcCmd(55, (((rca>>8)&0xFF)<<24) | ((rca&0xFF)<<16) | 0xFFFF); // APP_CMD
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || !(resp[4] & 0x20))
        return 0;                       // unusable
    sendMmcCmd(6, 2);                   // SET_BUS_WIDTH (to 4 bits)
    if (!recvMmcCmdResp(resp, R1_LEN, 1) || (resp[0] != 6))
        return 0;                       // unusable

    sd_speed = 0;                       // data rate after init
    return 1;
}

/*-----------------------------------------------------------------------*/
/* Initialize SD(HC) Card                                                */
/*-----------------------------------------------------------------------*/

DSTATUS MMC_disk_initialize (void)
{
    int i;
    int result;

    for (i=0; i<CACHE_SIZE; i++)
        sec_tags[i] = 0xFFFFFFFF;       // invalidate cache entry

    cardType &= 0x8000;                 // keep funky flag

    sd_speed = 1600;                    // init with slow data rate
    neo2_pre_sd();
    result = sdInit();
    neo2_post_sd();

    if (!result)
        cardType = 0xFFFF;

#ifdef DEBUG_PRINT
    if (!result)
        debugMmcPrint("Card unusable");
    else
    {
        if (cardType & 1)
            debugMmcPrint("SDHC Card ready");
        else
            debugMmcPrint("SD Card ready");
    }
#endif

    if ((sd_csd[1] & 0xC0) == 0)
    {
        // CSD type 1 - version 1.x cards, and version 2.x standard capacity
        //
        //    C_Size      - 12 bits - [73:62]
        //    C_Size_Mult -  3 bits - [49:47]
        //    Read_Bl_Len -  4 bits - [83:80]
        //
        //    Capacity (bytes) = (C_Size + 1) * ( 2 ^ (C_Size_Mult + 2)) * (2 ^ Read_Bl_Len)

        unsigned int C_Size      = ((sd_csd[7] & 0x03) << 10) | (sd_csd[8] << 2) | ((sd_csd[9] >>6) & 0x03);
        unsigned int C_Size_Mult = ((sd_csd[10] & 0x03) << 1) | ((sd_csd[11] >> 7) & 0x01);
        unsigned int Read_Bl_Len = sd_csd[6] & 0x0f;

        num_sectors = ((C_Size + 1) * (1 << (C_Size_Mult + 2)) * ((1 << Read_Bl_Len)) / 512);
    }
    else
    {
        // CSD type 2 - version 2.x high capacity
        //
        //    C_Size      - 22 bits - [69:48]
        //
        //    Capacity (bytes) = (C_Size + 1) * 1024 * 512

        num_sectors = (((sd_csd[8] & 0x3F) << 16) | (sd_csd[9] << 8) | sd_csd[10]) * 1024;
    }

    return result ? 0 : STA_NODISK;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS MMC_disk_status (void)
{
    return (cardType != 0xFFFF) ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT MMC_disk_read (
    BYTE *buff,            /* Data buffer to store read data */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count            /* Number of sectors to read (1..255) */
)
{
    unsigned int ix;

    if (count == 1)
    {
        ix = sector & (CACHE_SIZE - 1);    // really simple hash
        if (sec_tags[ix] != sector)
        {
            // sector not in cache - fetch it
            neo2_pre_sd();
            if (!sdReadSingleBlock(&sec_cache[ix << 9], sector << ((cardType & 1) ? 0 : 9)))
            {
                // read failed, retry once
                if (!sdReadSingleBlock(&sec_cache[ix << 9], sector << ((cardType & 1) ? 0 : 9)))
                {
                    // read failed
                    neo2_post_sd();
                    sec_tags[ix] = 0xFFFFFFFF;
                    //debugPrint("Read failed!");
                    return RES_ERROR;
                }
            }
            neo2_post_sd();
            sec_tags[ix] = sector;
        }

        memcpy(buff, &sec_cache[ix << 9], 512);
    }
    else
    {
        neo2_pre_sd();
        if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
        {
            // read failed, retry once
            if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!sdReadMultiBlocks(buff, count))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
        }
        else if (!sdReadMultiBlocks(buff, count))
        {
            // read failed, retry once
            if (!sdReadStartMulti(sector << ((cardType & 1) ? 0 : 9)))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!sdReadMultiBlocks(buff, count))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
        }
        sdReadStopMulti();
        neo2_post_sd();
    }

    return RES_OK;
}

int last_sector_before_failure = 0;

DRESULT MMC_disk_read_multi(BYTE* buff,DWORD sector,UINT count)
{
	last_sector_before_failure = 0;
	neo2_pre_sd();

    {
		sector <<= ((cardType & 1) ? 0 : 9);

        if (!sdReadStartMulti(sector))
        {
            // read failed, retry once
            if (!sdReadStartMulti(sector))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!sdReadMultiBlocks(buff,count))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
			
			sdReadStopMulti();
			neo2_post_sd();
			return RES_OK;
        }
        
		if (!sdReadMultiBlocks(buff,count))
        {
            // read failed, retry once
            if (!sdReadStartMulti(sector))
            {
                // start multiple sector read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
            if (!sdReadMultiBlocks(buff + (last_sector_before_failure << 9),count - last_sector_before_failure))
            {
                // read failed
                neo2_post_sd();
                //debugPrint("Read failed!");
                return RES_ERROR;
            }
        }
   		 sdReadStopMulti();
    }

    neo2_post_sd();
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT MMC_disk_write (
    const BYTE *buff,    /* Data to be written */
    DWORD sector,        /* Sector address (LBA) */
    BYTE count           /* Number of sectors to write (1..255) */
)
{
    unsigned int ix, iy;

    sd_speed = 1;                       // no fast access on writes
    for (ix=0; ix<count; ix++)
    {
        iy = (sector + ix) & (CACHE_SIZE - 1);    // really simple hash
        if (sec_tags[iy] == (sector + ix))
            sec_tags[iy] = 0xFFFFFFFF;

        memcpy(sec_buf, (void *)((unsigned int)buff + (ix << 9) ), 512);
        neo2_pre_sd();
        if (!sdWriteSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
        {
            // write failed, retry once
            if (!sdWriteSingleBlock(sec_buf, (sector + ix) << ((cardType & 1) ? 0 : 9)))
            {
                neo2_post_sd();
                sd_speed = 0;           // allow fast access on reads
                return RES_ERROR;
            }
        }
        neo2_post_sd();
    }

    sd_speed = 0;                       // allow fast access on reads
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT MMC_disk_ioctl (
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    switch (ctrl)
    {
        case GET_SECTOR_COUNT:
            *(unsigned int *)buff = num_sectors;
            return num_sectors ? RES_OK : RES_ERROR;
        case GET_SECTOR_SIZE:
            *(unsigned short *)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(unsigned int *)buff = 1;
            return RES_OK;
        case CTRL_SYNC:
            return RES_OK;
        case CTRL_POWER:
        case CTRL_LOCK:
        case CTRL_EJECT:
        case MMC_GET_TYPE:
        case MMC_GET_CSD:
        case MMC_GET_CID:
        case MMC_GET_OCR:
        case MMC_GET_SDSTAT:
        default:
            return RES_PARERR;
    }
}

/* 31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */
/* 15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */
DWORD get_fattime (void)
{
    return 0;
}
 

