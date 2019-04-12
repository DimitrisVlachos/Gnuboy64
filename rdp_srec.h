
/*
	Author : (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com 2011~2013 ,for myth player64 & GNUBOY64 port
	RDP instruction set + original rdp module + lib dragon: Shaun Taylor (http://www.dragonminded.com/n64dev/)
*/
#ifndef _rdp_srec_h_
#define _rdp_srec_h_


#include <libdragon.h>
#include <stdint.h>
#include "rdp_srec_tex_format.h"

typedef double scalar;
typedef uint32_t rdp_instruction_t;

typedef struct 
{
	rdp_instruction_t* instruction;
	rdp_srec_texture_cache_entry_t* cache;
	int32_t curr_inst;
	int32_t max_inst;
	int32_t max_cache_slots;
} __attribute__((aligned(32))) rpd_srec_compiled_instruction_block_t;
  
rpd_srec_compiled_instruction_block_t* rdp_srec_create(int32_t max_instructions,int32_t max_cache_slots);
void rdp_srec_destroy(rpd_srec_compiled_instruction_block_t* block);
void rdp_srec_invalidate(rpd_srec_compiled_instruction_block_t* block);
void rdp_srec_execute_block(rpd_srec_compiled_instruction_block_t* inst_stream,int32_t wait_full_sync);
void rdp_srec_op_bind_framebuffer(rpd_srec_compiled_instruction_block_t* inst_stream,void* fb,int32_t width,int32_t height,int32_t bpp);
void rdp_srec_op_set_clipping(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t tx,uint32_t ty,uint32_t bx,uint32_t by);
void rdp_srec_op_enable_texture_copy(rpd_srec_compiled_instruction_block_t* inst_stream);
void rdp_srec_op_sync(rpd_srec_compiled_instruction_block_t* inst_stream,sync_t sync);
void rdp_srec_op_load_texture(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite);
void rdp_srec_op_load_texture_stride(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite,int32_t offset);
void rdp_srec_op_draw_textured_rectangle_scaled(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t tx,int32_t ty,int32_t bx,int32_t by,
scalar x_scale,scalar y_scale);
void rdp_srec_op_draw_textured_rectangle(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t tx,int32_t ty,int32_t bx,int32_t by );
void rdp_srec_op_draw_sprite(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t x,int32_t y );
void rdp_srec_op_draw_sprite_scaled(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t x,int32_t y,scalar x_scale,scalar y_scale);
void rdp_srec_op_load_texture_buf_stride(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,void* data,int32_t hslices,int32_t vslices,int32_t width,int32_t height,int32_t bitdepth,int32_t offset);
void rdp_srec_op_load_texture_buf(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,
	void* data,int32_t width,int32_t height,int32_t bitdepth );
void rdp_srec_op_draw_textured_rectangle_scaled_ex(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t tx,int32_t ty,int32_t bx,int32_t by,
													scalar x_scale,scalar y_scale,uint16_t s,uint16_t t);
#endif

