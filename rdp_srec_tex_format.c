/*
	Author : (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com 2011~2013 ,for myth player64 & GNUBOY64 port
*/
#include "rdp_srec_tex_format.h"
#include <malloc.h>
#include <string.h>

rdp_srec_texture_t* rdp_srec_tex_create(uint16_t w,uint16_t h,uint16_t bpp,uint16_t format) {
	rdp_srec_texture_t* srec_texture;
	const uint32_t sz = (((w * h * bpp) + sizeof(rdp_srec_texture_t)) + (bpp << 1)) & (~((bpp << 1)-1));

	if ((0 == bpp) || (0 == w) || (0 == h)) { return NULL; }

	srec_texture = (rdp_srec_texture_t*)malloc(sz);
	if (!srec_texture) { return NULL; }

	srec_texture->width = w;
	srec_texture->height = h;
	srec_texture->bitdepth = bpp;
	srec_texture->format = format;
	srec_texture->tilepart = 0xffffffff;
	srec_texture->hslices = srec_texture->width >> 5;
	srec_texture->vslices = srec_texture->height >> 4;

	/*Make w/dw writes safe*/
	uint8_t* addr = ((void*)srec_texture) + sizeof(rdp_srec_texture_t); 
	if ((uint32_t)addr & (bpp-1)) {
		srec_texture->data = (void*)addr + (8 - ((uint32_t)addr & (bpp-1)));
	} else {
		srec_texture->data = (void*)addr;
	}

	data_cache_hit_writeback_invalidate(srec_texture,sizeof(rdp_srec_texture_t));
	return srec_texture;
}

static rdp_srec_texture_t* rdp_srec_tex_create_tilepart(void* in_addr,uint16_t w,uint16_t h,uint16_t bpp,uint16_t format) {
	rdp_srec_texture_t* srec_texture;
	const uint32_t sz = sizeof(rdp_srec_texture_t);

	if ((0 == bpp) || (0 == w) || (0 == h)) { return NULL; }

	srec_texture = (rdp_srec_texture_t*)malloc(sz);
	if (!srec_texture) { return NULL; }

	srec_texture->width = w;
	srec_texture->height = h;
	srec_texture->bitdepth = bpp;
	srec_texture->format = format;
	srec_texture->tilepart = 0;
	srec_texture->hslices = srec_texture->width >> 5;
	srec_texture->vslices = srec_texture->height >> 4;
	srec_texture->data = in_addr;

	return srec_texture;
}

/*Create a tile list based on texture. width must be divisible by tw , and height by th...*/
rdp_srec_texture_tile_list_t* rdp_srec_tilelist_create(rdp_srec_texture_t* texture,uint16_t tw,uint16_t th,uint16_t tx_offs,uint16_t ty_offs) {
	rdp_srec_texture_tile_list_t* srec_tl;

	
	if ((!texture) || (0 == tw) || (0 == th) || (texture->width < tw) || (texture->height < th)) { return NULL; }

	srec_tl = (rdp_srec_texture_tile_list_t*)malloc(sizeof(rdp_srec_texture_tile_list_t));
	if (!srec_tl) { return NULL; }

	srec_tl->texture = NULL; 
	srec_tl->tiles = 0;

	/*Estimate TC*/
	uint32_t twc,thc;

	twc = (((texture->width + tw) & (~(tw-1))) / tw) + 1;
	thc = (((texture->height + th) & (~(th-1))) / th) + 1;

	srec_tl->texture = (rdp_srec_texture_t**)malloc(sizeof(rdp_srec_texture_t**) * (twc * thc));
	if (!srec_tl->texture) { rdp_srec_tilelist_destroy(srec_tl); return NULL; }

	uint32_t i,j;
	for (i = 0,j = thc*twc;i < j;++i) { 
		srec_tl->texture[i] = NULL; 
	}

	data_cache_hit_writeback_invalidate(srec_tl,sizeof(rdp_srec_texture_tile_list_t));


	/*Make tiles*/
	uint16_t x,y;

	uint8_t* tdata = texture->data;
	for (y = 0;y < texture->height;y += th) {
		const uint32_t yb = ((uint32_t)y * (uint32_t)texture->width * (uint32_t)texture->bitdepth);
		for (x = 0;x < texture->width;x += tw) {
			srec_tl->texture[srec_tl->tiles] = 
			rdp_srec_tex_create_tilepart(&tdata[yb + ((uint32_t)x * (uint32_t)texture->bitdepth)],
			tw,th,texture->bitdepth,texture->format);
			if (!srec_tl->texture[srec_tl->tiles]) {
				rdp_srec_tilelist_destroy(srec_tl); 
				return NULL;
			}
			srec_tl->texture[srec_tl->tiles]->tilepart = ((uint32_t)(tx_offs+x) << 16) | (uint32_t)(ty_offs+y);
			data_cache_hit_writeback_invalidate(srec_tl->texture[srec_tl->tiles],sizeof(rdp_srec_texture_t));
			++srec_tl->tiles;
		}
	}

	data_cache_hit_writeback_invalidate(srec_tl,sizeof(rdp_srec_texture_tile_list_t));
	return srec_tl;
}

void rdp_srec_tilelist_destroy(rdp_srec_texture_tile_list_t* srec_tl) {
	if (srec_tl) { 
		if (srec_tl->texture) {	
			uint32_t i;	
			for (i = 0;i < srec_tl->tiles;++i) {
				if (srec_tl->texture[i]) { free(srec_tl->texture[i]); }
			}
			free(srec_tl->texture);
		}
		free(srec_tl); 
	}
}

void rdp_srec_tex_destroy(rdp_srec_texture_t* srec_texture) {
	if (srec_texture) { free(srec_texture); }
}

void rdp_srec_tex_convert(sprite_t* ldragon_sprite,rdp_srec_texture_t* srec_texture) {
	srec_texture->width = ldragon_sprite->width;
	srec_texture->height = ldragon_sprite->height;
	srec_texture->bitdepth = ldragon_sprite->bitdepth;
	srec_texture->format = ldragon_sprite->format;
	srec_texture->data = ldragon_sprite->data;
	srec_texture->hslices = ldragon_sprite->hslices;
	srec_texture->vslices = ldragon_sprite->vslices;
	srec_texture->tilepart = 0;
	data_cache_hit_writeback_invalidate(srec_texture,sizeof(rdp_srec_texture_t));
}

