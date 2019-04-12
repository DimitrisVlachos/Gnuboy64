/*
	Author : (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com 2011~2013 ,for myth player64 & GNUBOY64 port
*/
#ifndef _rdp_srec_tex_format_h_
#define _rdp_srec_tex_format_h_
#include <libdragon.h>
#include <stdint.h>

typedef struct 
{
	uint32_t width;
	uint32_t height;
	uint32_t bitdepth;
	uint32_t format;
	uint32_t tilepart;
	uint32_t hslices;
	uint32_t vslices;
	void* data;
} __attribute__((aligned(sizeof(void*)))) rdp_srec_texture_t;

typedef struct /*tex coords + rect size*/
{
    int32_t s;		
    int32_t t;
    int32_t width;
    int32_t height;
} __attribute__((aligned(sizeof(int32_t)))) rdp_srec_texture_cache_entry_t;

typedef struct 
{
	uint16_t w;
	uint16_t h;
	uint16_t s;
	uint16_t t;
} __attribute__((aligned(sizeof(int32_t)))) rdp_srec_tile_t;

typedef struct 
{
	rdp_srec_texture_t** texture;
	uint32_t tiles;
} __attribute__((aligned(sizeof(void**)))) rdp_srec_texture_tile_list_t;

rdp_srec_texture_t* rdp_srec_tex_create(uint16_t w,uint16_t h,uint16_t bpp,uint16_t format);
/*Create a tile list based on texture. width must be divisible by tw , and height by th...*/
rdp_srec_texture_tile_list_t* rdp_srec_tilelist_create(rdp_srec_texture_t* texture,uint16_t tw,uint16_t th,uint16_t tx_offs,uint16_t ty_offs);
void  rdp_srec_tilelist_destroy(rdp_srec_texture_tile_list_t* srec_tl);
void rdp_srec_tex_destroy(rdp_srec_texture_t* srec_texture) ;
void rdp_srec_tex_convert(sprite_t* ldragon_sprite,rdp_srec_texture_t* srec_texture) ;
#endif

