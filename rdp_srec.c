/*
	Author : (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com 2011~2013 ,for myth player64 & GNUBOY64 port
	RDP instruction set + original rdp module + lib dragon: Shaun Taylor (http://www.dragonminded.com/n64dev/)
*/
#include "rdp_srec.h"
#include <malloc.h>
#include <string.h>

#define rdp_srec_write_bc(__inst__)\
{\
	if (inst_stream->curr_inst<inst_stream->max_inst) { \
		inst_stream->instruction[inst_stream->curr_inst] = (__inst__); \
		++inst_stream->curr_inst;\
	}\
}

uint32_t __rdp_srec_compile_ld_tex(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,
	uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite,int32_t sl,int32_t tl,int32_t sh,int32_t th );

rpd_srec_compiled_instruction_block_t* rdp_srec_create(int32_t max_instructions,int32_t max_cache_slots)
{
	rpd_srec_compiled_instruction_block_t* block;


	block = malloc(sizeof(rpd_srec_compiled_instruction_block_t));
	if(block == 0)
		return 0;

	block->cache = malloc(max_cache_slots * sizeof(rdp_srec_texture_cache_entry_t));
	if(block->cache == 0)
	{
		free(block);
		return 0;
	}

	block->instruction = malloc(max_instructions * sizeof(rdp_instruction_t));
	if(block->instruction == 0)
	{
		free(block->cache);
		free(block);
		return 0;
	}

	block->curr_inst = 0;
	block->max_inst = max_instructions;
	block->max_cache_slots = max_cache_slots;
	memset(block->instruction,0,max_instructions * sizeof(rdp_instruction_t));
	memset(block->cache,0,max_cache_slots * sizeof(rdp_srec_texture_cache_entry_t));

	//rdp_srec_invalidate(block);

	return block;
}

void rdp_srec_destroy(rpd_srec_compiled_instruction_block_t* block)
{
	if(!block)
		return;

	if(block->instruction)
		free(block->instruction);

	if(block->cache)
		free(block->cache);

	free(block);
}

void rdp_srec_invalidate(rpd_srec_compiled_instruction_block_t* block)
{
	data_cache_hit_writeback_invalidate(block,sizeof(rpd_srec_compiled_instruction_block_t));
	data_cache_hit_writeback_invalidate(block->instruction,
	((block->curr_inst) ? block->curr_inst : block->max_inst) * sizeof(rdp_instruction_t));
	data_cache_hit_writeback_invalidate(block->cache,block->max_cache_slots * sizeof(rdp_srec_texture_cache_entry_t));
}

void __rdp_execute(uint32_t* inst,uint32_t ptr)
{
	asm volatile(
		".set	push\n"
		".set	noreorder\n"
		".set	noat\n"
		"or		$t0,%0,$0\n"	
		"or		$t1,%1,$0\n"	
		"lui	$t2,0xa410\n"	
		"addi	$t3,$0,0x15\n"
		"sw		$t3,12($t2)\n"
		"addi	$t4,$0,0x600\n"
		"0:\n"
		"lw		$t3,12($t2)\n"
		"and	$t3,$t3,$t4\n"
		"bne	$t3,$0,0b\n"
		"nop\n"
		"lui	$t5,0xa000\n"
		"or		$t0,$t0,$t5\n"
		"sw		$t0,0($t2)\n"
		"sll	$t1,$t1,2\n"
		"addu	$t0,$t0,$t1\n"
		"sw		$t0,4($t2)\n"
		".set pop\n"
		: 
		: "r" (inst), "r" (ptr)
		: "$0"
	);
}

void rdp_srec_execute_block(rpd_srec_compiled_instruction_block_t* inst_stream,int32_t wait_full_sync)
{
	__rdp_execute(inst_stream->instruction,inst_stream->curr_inst);
}

void rdp_srec_op_bind_framebuffer(rpd_srec_compiled_instruction_block_t* inst_stream,void* fb,int32_t width,int32_t height,int32_t bpp)
{
    rdp_srec_write_bc( 0xFF000000 | ((bpp == 2) ? 0x00100000 : 0x00180000) | (width - 1) );
    rdp_srec_write_bc( (uint32_t)fb );
}

void rdp_srec_op_set_clipping(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t tx,uint32_t ty,uint32_t bx,uint32_t by)
{
	rdp_srec_write_bc( (0xED000000 | (tx << 14) | (ty << 2)) );
	rdp_srec_write_bc( (bx << 14) | (by << 2) );
}

void rdp_srec_op_enable_texture_copy(rpd_srec_compiled_instruction_block_t* inst_stream)
{
    rdp_srec_write_bc( 0xEFA000FF );
    rdp_srec_write_bc( 0x00004001 );
}

void rdp_srec_op_sync(rpd_srec_compiled_instruction_block_t* inst_stream,sync_t sync)
{
    switch(sync)
    {
		default:
        case SYNC_FULL:
			rdp_srec_write_bc(0xE9000000);
		break;

        case SYNC_PIPE:
			rdp_srec_write_bc(0xE7000000);
		break;

        case SYNC_TILE:
			rdp_srec_write_bc(0xE8000000);
		break;

        case SYNC_LOAD:
			rdp_srec_write_bc(0xE6000000);
		break;
    }

	rdp_srec_write_bc(0x00000000);
}

 

void rdp_srec_op_load_texture(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite)
{
	__rdp_srec_compile_ld_tex(inst_stream,texslot,texloc,mirror_enabled,sprite,0,0,sprite->width - 1,sprite->height - 1 );
}

void rdp_srec_op_load_texture_stride(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite,int32_t offset)
{
	int32_t sprhs = sprite->hslices;
	int32_t sprvs = sprite->vslices;
    int32_t twidth = sprite->width / sprhs;
    int32_t theight = sprite->height / sprvs;
    int32_t sl = (offset & (sprhs-1)) * twidth;
    int32_t tl = (offset / sprhs) * theight;
    int32_t sh = sl + twidth - 1;
    int32_t th = tl + theight - 1;

    __rdp_srec_compile_ld_tex(inst_stream,texslot,texloc,mirror_enabled,sprite,sl,tl,sh,th );
}

 

void rdp_srec_op_draw_textured_rectangle_scaled(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t tx,int32_t ty,int32_t bx,int32_t by,
													scalar x_scale,scalar y_scale)
{
    uint16_t s = inst_stream->cache[texslot].s << 5;
    uint16_t t = inst_stream->cache[texslot].t << 5;
	int32_t xs,ys;
    if( tx < 0 )
    {
        s += (int32_t)((scalar)((-tx) << 5) * (1.0 / x_scale));
        tx = 0;
    }

    if( ty < 0 )
    {
        t += (int32_t)((scalar)((-ty) << 5) * (1.0 / y_scale));
        ty = 0;
    }

    xs = (int32_t)((1.0 / x_scale) * 4096.0);
    ys = (int32_t)((1.0 / y_scale) * 1024.0);

    rdp_srec_write_bc( 0xE4000000 | (bx << 14) | (by << 2) );
    rdp_srec_write_bc( ((texslot) << 24) | (tx << 14) | (ty << 2) );
    rdp_srec_write_bc( (s << 16) | t );
    rdp_srec_write_bc( (xs & 0xFFFF) << 16 | (ys & 0xFFFF) );
}
 

void rdp_srec_op_draw_textured_rectangle(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t tx,int32_t ty,int32_t bx,int32_t by )
{
    rdp_srec_op_draw_textured_rectangle_scaled(inst_stream,texslot,tx,ty,bx,by,1.0,1.0 );
}

void rdp_srec_op_draw_sprite(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t x,int32_t y )
{
    rdp_srec_op_draw_textured_rectangle_scaled(inst_stream,texslot,x,y,x + inst_stream->cache[texslot].width,y + inst_stream->cache[texslot].height,1.0,1.0 );
}

void rdp_srec_op_draw_sprite_scaled(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,int32_t x,int32_t y,scalar x_scale,scalar y_scale)
{
    int32_t new_width = (int32_t)(((scalar)inst_stream->cache[texslot].width * x_scale) + 0.5);
    int32_t new_height = (int32_t)(((scalar)inst_stream->cache[texslot].height * y_scale) + 0.5);
    
    rdp_srec_op_draw_textured_rectangle_scaled(inst_stream,texslot,x,y,x + new_width,y + new_height,x_scale,y_scale );
}

static uint32_t rdp_srec_round_to_power( uint32_t number )
{
    if( number <= 4   ) { return 4;   }		/*Maybe i should implement a map for this..*/
    if( number <= 8   ) { return 8;   }
    if( number <= 16  ) { return 16;  }
    if( number <= 32  ) { return 32;  }
    if( number <= 64  ) { return 64;  }
    if( number <= 128 ) { return 128; }

    return 256;
}

static uint32_t rdp_srec_log2( uint32_t n )
{
    switch( n )
    {
        case 4:
            return 2;
        case 8:
            return 3;
        case 16:
            return 4;
        case 32:
            return 5;
        case 64:
            return 6;
        case 128:
            return 7;
        default:
            /* Don't support more than 256 */
            return 8;
    }
}

 

uint32_t __rdp_srec_compile_ld_tex(rpd_srec_compiled_instruction_block_t* inst_stream,uint32_t texslot,
	uint32_t texloc,mirror_t mirror_enabled,rdp_srec_texture_t *sprite,int32_t sl,int32_t tl,int32_t sh,int32_t th)
{
    int32_t twidth = sh - sl + 1;
    int32_t theight = th - tl + 1;
    uint32_t real_width  = rdp_srec_round_to_power( twidth );
    uint32_t real_height = rdp_srec_round_to_power( theight );
    uint32_t wbits = rdp_srec_log2( real_width );
    uint32_t hbits = rdp_srec_log2( real_height );
    int32_t round_amount = (real_width &(8-1)) != 0;
	rdp_srec_texture_cache_entry_t* sc;

    rdp_srec_write_bc( 0xFD000000 | ((sprite->bitdepth == 2) ? 0x00100000 : 0x00180000) | (sprite->width - 1) );
    rdp_srec_write_bc(   (uint32_t)UncachedAddr(sprite->data) );
    rdp_srec_write_bc( 0xF5000000 | ((sprite->bitdepth == 2) ? 0x00100000 : 0x00180000) | 
                                       (((((real_width >> 3) + round_amount) * sprite->bitdepth) & 0x1FF) << 9) | ((texloc >> 3) & 0x1FF) );
    rdp_srec_write_bc( ((texslot) << 24) | (mirror_enabled == MIRROR_ENABLED ? 0x40100 : 0) | (hbits << 14 ) | (wbits << 4) );

    rdp_srec_write_bc( 0xF4000000 | (((sl << 2) & 0xFFF) << 12) | ((tl << 2) & 0xFFF) );
    rdp_srec_write_bc( (((sh << 2) & 0xFFF) << 12) | ((th << 2) & 0xFFF) );

	sc = &inst_stream->cache[texslot];
    sc->width = twidth - 1;
    sc->height = theight - 1;
    sc->s = sl;
    sc->t = tl;
    
    return (((real_width >> 3) + round_amount) << 3) * real_height * sprite->bitdepth;
}

