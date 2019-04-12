/*
	GNUBoy N64 port by (Dimitrios Vlachos : https://github.com/DimitrisVlachos)~ (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com
*/

#ifndef MAX_SAVE_SLOTS
	#define MAX_SAVE_SLOTS (999)
#endif

#define cached_to_uncached_addr(_addr_,_type_)( (_type_*)(0x20000000 + ((unsigned int) _addr_)) )
#define uncached_to_cached_addr(_addr_,_type_)( (_type_*)(((unsigned int) _addr_) - 0x20000000) )
#define uncached_addr(x)	((void *)(((u32)(x)) | 0xA0000000))
#define align16(x)			((void *)(((u32)(x)+15) & 0xFFFFFFF0))
 

#define controller_btn_pressed(_b_) (controller.c[0]._b_)
#define controller_reset(_b_) (controller.c[0]._b_ = 0)
#define controller_toggle(_b_) (controller.c[0]._b_ ^= 1)
#define controller_btn_pressed_cmp(_b_,_r_) {\
	int32_t curr = (controller.c[0].data >> 16) ;\
	_r_ = controller_btn_pressed(_b_) && (curr != last_button_data);\
	last_button_data = curr;\
}

#define controller_begin_state()
#define controller_btn_status(_b_) (controller_btn_pressed(_b_) && ( ((controller.c[0].data >> 16)  ) != last_button_data))
#define controller_end_state() (last_button_data = (controller.c[0].data >> 16) )

 
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <libdragon.h>
#include <diskio.h>
#include <ff.h>

#include "wstring.h"
#include "types.h"
#include "filesystem/drivers/fs_drv.h"
#include "filesystem/filesystem.h"
#include "emu_core/gnuboy.h"
#include "emu_core/input.h"
#include "emu_core/loader.h"
#include "emu_core/fb.h"
#include "emu_core/hw.h"
#include "emu_core/pcm.h"
#include "emu_core/rtc.h"
#include "pcm_ring_buf.h"
#include "ctl.h"
#include "media.h"
#include "rdp_srec.h"
#include "umem/umem.h"
#include "emu_core/mem.h"
#include "emu_core/lcd.h"
#include "lcd_tables/lcd_pal4.h"
#include <stdarg.h>
 
#include "emu_core/defs.h"
#include "emu_core/regs.h"
#include "emu_core/mem.h"
 
#include "emu_core/lcd.h"
#include "emu_core/rtc.h"

#include "emu_core/sound.h"

typedef enum menu_state_t menu_state_t;
enum menu_state_t {
	STATE_BROWSER = 0,
	STATE_EMU,
};


struct fb __attribute__ ((aligned (16))) fb;
struct pcm __attribute__ ((aligned (16))) pcm;

byte g_curr_snd_driver = 0;
static uint16_t screen_info_changed = 0,screenshot_n = 0;
static uint16_t frame_buffer_w = 0,frame_buffer_h = 0,frame_buffer_bpp  = 0,screen_w = 0,screen_h = 0,screen_bpp = 0;
static uint8_t* frame_buffer = 0;
static display_context_t binded_display_ctx = 0xffff;
static struct controller_data controller;
static int32_t last_button_data = 0;
static rpd_srec_compiled_instruction_block_t* rdp_compiled_scene = 0;
static sprite_t* rdp_scene_buffer = 0;
uint16_t save_slot = 0;
extern void emu_init();
extern void emu_reset();
extern void emu_run();
static void take_screenshot(const char* path,const uint8_t* data,const int32_t width,const int32_t height,const int32_t bpp) ;
static void switch_display(int32_t hi_res,int32_t depth,int32_t clear_fb,int32_t force) ;
static void fb_init(int rdp_fb,int scale) ;

void doevents();
extern void loadstate_ptr(byte* buf,un32 buf_size);

void delay_secs(int32_t secs) {
	unsigned long long int t0,t1,t2,t3;
	const unsigned long long microsecond = 1000;
	const unsigned long long second = microsecond * 1000;
	t3  = 0;
	t0 = get_ticks();							
	t2 = TIMER_TICKS_LL(second * (unsigned long long int)secs  );
	while (t3 < t2) {
		t1 = get_ticks();
		t3 += t1 - t0;
		t0 = t1;
	}
}

void show_error(const char* s)
{
	graphics_fill_screen(binded_display_ctx, 0);
	graphics_draw_text( binded_display_ctx,(screen_w/2) - ( (strlen("CRITICAL ERROR")<<3) >> 1 ),(screen_h/2) - 8,"CRITICAL ERROR");
	graphics_draw_text( binded_display_ctx,(screen_w/2) - ( (strlen(s)<<3) >> 1 ),(screen_h/2) + 8,s);
	graphics_draw_text( binded_display_ctx,(screen_w/2) - ( (strlen("PRESS Z TO REBOOT")<<3) >> 1 ),(screen_h/2) +98,"PRESS Z TO REBOOT");
	graphics_draw_text( binded_display_ctx,(screen_w/2) - ( (strlen("PRESS Cr TO TAKE SCREENSHOT")<<3) >> 1 ),(screen_h/2) +106,"PRESS Cr TO TAKE SCREENSHOT");
	enable_interrupts();
	while(1) {
		controller_read(&controller); 
		controller_begin_state();
		if (controller_btn_pressed(Z)) {
			asm volatile ("j _start\nnop\n"); 
		} else if (controller_btn_pressed(C_right)) {
			fs_drv_sync();
			take_screenshot("gnuboy/screenshots/error.tga",frame_buffer,frame_buffer_w,frame_buffer_h,frame_buffer_bpp);
		}

		controller_end_state();
	}
}

void die(char *fmt, ...)
{
    char buf[512];
 
	switch_display(1,32,0,1);
	fb_init(0,1);

    va_list ap;
    va_start(ap,fmt);
    vsnprintf(buf,512,fmt,ap);
    va_end(ap);

	show_error(buf);
}

static void spr_to_ptr(const char* name,uint8_t** out,const uint32_t len)
{
	uint8_t tmp[256]; 
	fs_handle_t* f;
	*out = (uint8_t*)malloc(len);
	if (!*out) { return; }
	f = fs_open(name,"rb");
	if (!f) {*out = NULL; free(*out); return; }
	fs_read(f,(uint8_t*)tmp,sizeof(sprite_t)); /*TODO Should read the header info instead!*/
	fs_read(f,(*out),len);
	fs_close(f);
}

static void fb_centered() {
	const uint32_t y_center = ((frame_buffer_h >> 1)-(fb.h>>1));
	const uint32_t x_center = ((frame_buffer_w >> 1)-(fb.w>>1));
	fb.pitch = fb.pelsize * frame_buffer_w;
	fb.ptr = &frame_buffer[y_center * fb.pitch + (x_center * fb.pelsize)]; /*Uncached!*/
	data_cache_hit_writeback_invalidate(&fb,sizeof(struct fb));
}

static void fb_init(int rdp_fb,int scale) 
{
	extern int lcd_scale;
	frame_buffer_w = screen_w;
	frame_buffer_h = screen_h;
	frame_buffer_bpp = screen_bpp;
	memset(&fb,0,sizeof fb);

	if (frame_buffer_bpp == 2) { 
		fb.cc[0].r = 3;		/*Pointless to set these now ~all are precomputed*/
		fb.cc[0].l = 11;
		fb.cc[1].r = 2;
		fb.cc[1].l = 5;
		fb.cc[2].r = 3;
		fb.cc[2].l = 0;
	 
	} else {
		fb.cc[0].r = 0;		/*Pointless to set these now ~all are precomputed*/
		fb.cc[1].r = 0;
		fb.cc[2].r = 0;
		fb.cc[0].l = 24;
		fb.cc[1].l = 16;
		fb.cc[2].l = 8;
	}
	fb.enabled=1;
	lcd_scale=scale;
	fb.w=160*scale;
	fb.h=144*scale;
	fb.indexed=0;
	fb.dirty=0;
	fb.pelsize = frame_buffer_bpp;
	
	if ((rdp_fb)) {// && (2==frame_buffer_bpp)) {
		/*Init srec&buf,compile scene*/
 
		const int32_t tw=256,th=256,tsw = 5,tsh = 5;
		if (rdp_scene_buffer) { free(rdp_scene_buffer); }
		rdp_scene_buffer = (sprite_t*)malloc(sizeof(sprite_t) + (tw * th * frame_buffer_bpp) + 32);
 
		if (!rdp_scene_buffer) { show_error("rdp_scene_buffer == 0"); }

		fb.pitch = tw * fb.pelsize;
		fb.ptr =   align16(UncachedAddr((void*)rdp_scene_buffer->data));

		if (rdp_compiled_scene) { rdp_srec_destroy(rdp_compiled_scene); }
		rdp_compiled_scene = rdp_srec_create(1024,8);
		if (!rdp_compiled_scene) { show_error("rdp_compiled_scene == 0"); }
 
		memset(fb.ptr,0,tw*th*frame_buffer_bpp);
		rdp_scene_buffer->bitdepth = frame_buffer_bpp;
		rdp_scene_buffer->format = 0;
		rdp_scene_buffer->width = tw;
		rdp_scene_buffer->height = th;
		rdp_scene_buffer->hslices =rdp_scene_buffer->width>>tsw;
		rdp_scene_buffer->vslices = rdp_scene_buffer->height>>tsh;
		data_cache_hit_writeback_invalidate(rdp_scene_buffer,sizeof(sprite_t));

		rdp_srec_op_sync(rdp_compiled_scene,SYNC_PIPE);
		rdp_srec_op_enable_texture_copy(rdp_compiled_scene);
		rdp_srec_op_set_clipping(rdp_compiled_scene,0,0,screen_w,screen_h);
		rdp_srec_op_bind_framebuffer(rdp_compiled_scene,frame_buffer,frame_buffer_w,frame_buffer_h,frame_buffer_bpp);

		/*TODO HW tile scaling...*/
		
		int32_t sx = 0,sy = 0;
		rdp_srec_texture_t srec_texture;
		rdp_srec_tex_convert(rdp_scene_buffer,&srec_texture);

		const scalar scale = 1.0;
 
		for (int32_t j = 0;j < rdp_scene_buffer->vslices ;++j)
		{
		    for (int32_t i = 0;i < rdp_scene_buffer->hslices ;++i)
		    {
				
		        rdp_srec_op_sync(rdp_compiled_scene,SYNC_PIPE);
		        rdp_srec_op_load_texture_stride(rdp_compiled_scene,0,0,MIRROR_DISABLED, &srec_texture, j*srec_texture.hslices + i);
		        rdp_srec_op_draw_sprite_scaled(rdp_compiled_scene, 0,sx+((i << tsw)*scale) - (i*scale)   ,sy+((j << tsh)*scale) - (j*scale) ,scale,scale);
 
		    }
		}

		data_cache_hit_writeback_invalidate(&fb,sizeof(struct fb));
		rdp_srec_invalidate(rdp_compiled_scene);
		return;
 
	}

	fb_centered();
}

static void switch_display(int32_t hi_res,int32_t depth,int32_t clear_fb,int32_t force) 
{
	static uint16_t was_inited = 0;
	display_context_t disp = 0;
	int32_t new_w,new_h,new_bpp;

	new_bpp = depth>>3;
	if (hi_res) { new_w = 512; new_h = 480; }
	else { new_w = 256; new_h = 240; }
	
	if ((!force)&&(new_w == screen_w) && (new_h == screen_h) && (new_bpp == screen_bpp) && (was_inited) && (0xfff != binded_display_ctx)) {
		if (clear_fb) { graphics_fill_screen(binded_display_ctx, 0); }
		return;
	}

	if (was_inited) {
		disable_interrupts(); rdp_close(); enable_interrupts(); 
		display_close();
	}

	
	display_init((!hi_res) ?  RESOLUTION_256x240 : RESOLUTION_512x480,
	(depth == 32) ? DEPTH_32_BPP : DEPTH_16_BPP,1,GAMMA_NONE,ANTIALIAS_RESAMPLE);
	rdp_init();
	rdp_set_texture_flush(FLUSH_STRATEGY_NONE);
	
	while( !(disp = display_lock()) ) {} 
	
	if (clear_fb) { graphics_fill_screen(disp, 0); }
	display_show(disp); 

	frame_buffer = (uint8_t*)display_grab_framebuffer_ptr(disp);//non cached
	binded_display_ctx = disp;

	screen_w = new_w;
	screen_h = new_h;
	screen_bpp = new_bpp;
	was_inited = 1;
}

static int32_t init_everything()
{	
	int32_t res;
 
	rdp_scene_buffer = 0;
	rdp_compiled_scene = 0;
	init_interrupts();	
	res = fs_drv_init(0,0);
	sys_set_boot_cic(6102);
    controller_init();

 	pcm_rbuf_init(8000,2,2048 );
	
	return res;
}

static void load_game(const char* name) {
	char conv[MAX_MEDIA_DESC_LEN*2];
	const char* loading = "[......NOW LOADING......]";
 
	sprintf(conv,"gnuboy/games/%s",name);

	data_cache_hit_writeback_invalidate(conv,sizeof(conv));
	graphics_draw_text( binded_display_ctx,(screen_w>>1) - ( (strlen(loading)<<3) >> 1 ),(screen_h>>1) - (8/2),loading);
	disable_interrupts();
	fs_drv_sync();
	fs_drv_set_sdc_speed(1);
	loader_init(conv);
	fs_drv_set_sdc_speed(0);
	enable_interrupts();
}

static void unload_game() {
	 
	
	graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Saving SRAM+RTC..")<<3)>>1),(screen_h>>1)-(8/2),
	"Saving SRAM+RTC..");


	fs_drv_sync(); /*Prepare fs for sram/rtc save during unload()*/
	fs_drv_set_sdc_speed(0);
	sram_save();
	rtc_save();
	loader_unload();
	fs_drv_set_sdc_speed(1);
	if (rdp_compiled_scene) {	
		rdp_srec_destroy(rdp_compiled_scene);
		rdp_compiled_scene = 0;
	}
	if (rdp_scene_buffer) {
		free(rdp_scene_buffer);
		rdp_scene_buffer = 0;
	}
}

static void do_menu() 
{
	uint8_t* bg_mem = NULL;
	media_entry_t* me = 0;
	media_res_t me_res = MEDIA_RES_MAJOR_UPDATE_NEEDED;
	int32_t start=0,sstart=0,end=0,selected=0,ybase=0,se=0,sorted_list=1;
	uint32_t curr_color=0;
	const uint32_t text_fg_color = graphics_make_color(0xff,0xff,0xff,0xff);
	const uint32_t text_hl_color = graphics_make_color(0x00,0x00,0x00,0x80); 
	const int32_t entries_per_page = 32;
	const int32_t max_entries = 4096;

 

	switch_display(1,32,0,0);
	fb_init(0,1);
	const uint32_t bg_len = frame_buffer_w*frame_buffer_h*frame_buffer_bpp;
	spr_to_ptr("gnuboy/res/bg512480.spr",&bg_mem,bg_len);
	if (!bg_mem) { show_error("Could not load BG!"); }


	media_init(max_entries,entries_per_page);
	media_import_list("gnuboy/games",sorted_list);
	media_load_state();
	 
	while (1) {
		if (me_res != MEDIA_RES_IDLE) {
			if (me_res == MEDIA_RES_MAJOR_UPDATE_NEEDED) { 
				disable_interrupts();
				umemcpy(frame_buffer,bg_mem,bg_len);
				enable_interrupts();
			}
			me_res = MEDIA_RES_IDLE;
			curr_color = 0;
			media_get_partition(&me,&start,&end,&selected);

			sstart = start;
			ybase = 100;//((frame_buffer_h>>1)) - ((entries_per_page<<3) >> 1);
			for (se = 0;start < end;++start,ybase += 8 + 2,++se) {
				if (se /*(end-start)*/== selected) {
					curr_color = text_hl_color;
					graphics_set_color(text_hl_color,0);
				} else if (text_fg_color != curr_color) {
					curr_color = text_fg_color;
					graphics_set_color(text_fg_color,0);
				}
				graphics_draw_text( binded_display_ctx,(screen_w>>1) - ( (me[start].name_len<<3) >> 1 ), ybase,me[start].name);
			}
			graphics_set_color(text_fg_color,0); //reset for rest draw text calls
		}

		controller_read(&controller); 
		controller_begin_state();
		if (controller_btn_status(start)) {
			umemcpy(frame_buffer,bg_mem,bg_len);
			graphics_draw_text( binded_display_ctx,(screen_w>>1) - ( (strlen("Fetching+Sorting...")<<3) >> 1 ), screen_h>>1,
			"Fetching+Sorting...");
			if (sorted_list) { media_save_state(); } else { media_dirty_state(); }
			sorted_list = 1;
			media_close();
			media_init(max_entries,entries_per_page);
			fs_drv_sync();
			media_import_list("gnuboy/games",1);
			media_load_state();
			me_res = MEDIA_RES_MAJOR_UPDATE_NEEDED;
			continue;
		} else if (controller_btn_status(A) && end) {
			umemcpy(frame_buffer,bg_mem,bg_len);
			free(bg_mem);
			media_save_state();
			media_close();
			switch_display(0,16,1,0);
			fb_init(0,1);	
			load_game(me[sstart + selected].name);
			return;
		}  else if (controller_btn_status(B) && end) {
			umemcpy(frame_buffer,bg_mem,bg_len);
			free(bg_mem);
			media_save_state();
			media_close();
			switch_display(1,16,1,1);
			fb_init(0,2);	
			load_game(me[sstart + selected].name);

			return;
		}  else if (controller_btn_status(up)) {
			me_res = media_prev_item();
		} else if (controller_btn_status(down)) {
			me_res = media_next_item();
		} else if (controller_btn_status(R)) {
			me_res = media_next_page();
		} else if (controller_btn_status(L)) {
			me_res = media_prev_page();
		} else if (controller_btn_status(C_right)) {
			char conv[64];
			disable_interrupts();
			fs_drv_sync();
			//fs_drv_set_sdc_speed(0);
			sprintf(conv,"gnuboy/screenshots/menu_%d.tga",screenshot_n++);
			take_screenshot(conv,frame_buffer,frame_buffer_w,frame_buffer_h,frame_buffer_bpp);
			//fs_drv_set_sdc_speed(1);
			enable_interrupts();
		}
		controller_end_state();

	}
}

static void intro() {
	switch_display(1,32,0,1);
	fb_init(0,1);

	uint8_t* bg_mem = NULL; 
	const uint32_t bg_len = frame_buffer_w*frame_buffer_h*frame_buffer_bpp;
	spr_to_ptr("gnuboy/res/intro.spr",&bg_mem,bg_len);
	if (!bg_mem) { return; } //show_error("Could not load INTRO!"); }
	
	umemcpy(frame_buffer,bg_mem,bg_len);
	free(bg_mem);

 
	delay_secs(3);
}

void browse_rom_data(uint32_t base_offset) {
	
	switch_display(1,32,1,1);
	fb_init(0,1);

	graphics_fill_screen(binded_display_ctx, 0);

	uint32_t upd = 1,disp_mode = 0; //0 = hex
	uint8_t buf[256];
	char conv[32];

	while (1) {
		controller_read(&controller); 
		controller_begin_state();

		if (controller_btn_status(R)) //INC by 1MB
			base_offset += 1 * 1024 * 1024,upd = 1;
		else if (controller_btn_status(L)) //DEC by 1MB
			base_offset -= 1 * 1024 * 1024,upd = 1;
		else if (controller_btn_status(C_up)) //INC by 1B
			base_offset += 1 ,upd = 1;
		else if (controller_btn_status(C_down)) //DEC by 1B
			base_offset -= 1 ,upd = 1;
		else if (controller_btn_status(C_left)) //disp = hex
			disp_mode = 0 ,upd = 1;
		else if (controller_btn_status(C_right)) //disp = C
			disp_mode = 1 ,upd = 1;
		else if (controller_btn_status(Z))  
			return;
		if (upd) {
			 
			 
			upd = 0;
			graphics_fill_screen(binded_display_ctx, 0);
			ctl_dma_rom_rd_abs(buf,base_offset,sizeof(buf));
			uint32_t i = 0,y = 64,sx = 56,x = sx;
			const uint32_t ls = screen_w - sx ;
			

			sprintf(conv,"LOC : 0x%08x |MODE=%s| R=+1M,L=-1M,Cu/u=+/-,Cl/r=disp,Z=Exit",base_offset,(disp_mode==0)?"H":"C");
  			graphics_draw_text(binded_display_ctx,sx,56,conv);
			for (;i < sizeof(buf);++i) {
				sprintf(conv,(disp_mode==0) ? "%02x" : "%c",buf[i]);
				graphics_draw_text(binded_display_ctx,x,y,conv);
				x += 8;
				if (x >= ls) {
					y += 8;	
					x = sx;
				}
			}
			 
		}

		controller_end_state();
	}

	
}


int main()
{
	init_everything();

#if __FS_BUILD__ != FS_N64_ROMFS
	menu_state_t state;
	intro();
	state = STATE_BROWSER;
	do {
		switch (state) {
			case STATE_EMU: {
				if (0xffff == binded_display_ctx) { /*Reached here unitialized -- ie quick load*/
					switch_display(0,32,1,1);
					fb_init(0,1);	
				}

				graphics_fill_screen(binded_display_ctx, 0);

				screen_info_changed = 1;
				save_slot = 0;
				
				emu_init();
				emu_reset();
				emu_run();
				disable_interrupts(); 
				graphics_fill_screen(binded_display_ctx, 0);
				unload_game();
				state = STATE_BROWSER;
				enable_interrupts();
				break;
			}

			default:
			case STATE_BROWSER: {
				do_menu();
				state = STATE_EMU;
				break;
			}
		}
	} while(1);
#else


	switch_display(1,16,1,1);
	fb_init(0,1);	
	graphics_fill_screen(binded_display_ctx, 0);

	//
	graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Official GNUBOYx86 port by (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on((Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com)")<<3)>>1),(screen_h>>1)-(8/2),
			"Official GNUBOYx86 port by (Dimitrios Vlachos : https://github.com/DimitrisVlachos)on((Dimitrios Vlachos : https://github.com/DimitrisVlachos)on1988@gmail.com)");
	graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Visit : http://code.google.com/p/gnuboy/")<<3)>>1),(screen_h>>1)+32,
			"Visit : http://code.google.com/p/gnuboy/");
#if 0
	{
		graphics_fill_screen(binded_display_ctx, 0);
		char dummy[128]={'\0'};
		char tmp[20] = {'\0'};
		ctl_dma_rom_rd(dummy,0,128);
		sprintf(tmp,"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		dummy[0],dummy[1],dummy[2],dummy[3],dummy[4],dummy[5],dummy[6],dummy[7],dummy[8],dummy[9],dummy[10],dummy[11],dummy[12],dummy[13],
		dummy[14],dummy[15]);
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen(tmp)<<3)>>1),(screen_h>>1)-(8/2),
			tmp);

		char pat[8];
		pat[0] = 'G';
		pat[1] = 'N';
		pat[2] = 'U';
		pat[3] = 'B';
		pat[4] = 'O';
		pat[5] = 'Y';
		pat[6] = '6';
		pat[7] = '4';

		uint32_t po = ctl_rom_scan(pat,0,sizeof(pat),1*1024*1024);
		sprintf(tmp,"pat = 0x%08x",po);
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen(tmp)<<3)>>1),(screen_h>>1)+16,
			tmp);

		if (pat) {
			ctl_dma_rom_rd(dummy,po,sizeof(pat));
			sprintf(tmp,"PAT=%c%c%c%c%c%c%c%c",dummy[0],dummy[1],dummy[2],dummy[3],dummy[4],dummy[5],dummy[6],dummy[7]);
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen(tmp)<<3)>>1),(screen_h>>1)+32,
			tmp);
		}
		delay_secs(5);
	}
#endif
	//delay_secs(3);

	//browse_rom_data(0);

	switch_display(0,16,1,1);
	fb_init(0,1);	
	graphics_fill_screen(binded_display_ctx, 0);

	disable_interrupts();
	loader_init("blah.gb");
	enable_interrupts();
	screen_info_changed = 1;
	save_slot = 0;
	emu_init();
	emu_reset();
	graphics_fill_screen(binded_display_ctx, 0);


	{
		disable_interrupts();
		uint8_t dummy[256];
		ctl_dma_rom_rd(dummy,0,256);
		uint32_t state_len = ((uint32_t)dummy[4] << 24U) | ((uint32_t)dummy[5] << 16U) | ((uint32_t)dummy[6] << 8U) | ((uint32_t)dummy[7]);
		if (state_len) {
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Loading state..")<<3)>>1),(screen_h>>1)-(8/2),
			"Loading state..");
			delay_secs(1);
 

			uint8_t* state = malloc(state_len);
			if (!state) die("Unable to allocate %u for state",state_len);
			ctl_dma_rom_rd(state,256  ,state_len);
			loadstate_ptr(state,state_len);
			vram_dirty();
			pal_dirty();
			sound_dirty();
			mem_updatemap();
			free(state);
		}
		enable_interrupts();
	}

	#if 1
	{
		extern int lcd_scale;
		int32_t x = ((screen_w>>1) - ((160*lcd_scale)>>1))    ;
		int32_t y = ((screen_h>>1) - ((144*lcd_scale)>>1))  + (144*lcd_scale) + 4;

		char dummy[128]={'\0'};
 		char tmp[256];
		ctl_dma_rom_rd(dummy,0,128);
		sprintf(tmp,"%02x %02x %02x %02x %02x %02x %02x %02x",
		dummy[0],dummy[1],dummy[2],dummy[3],dummy[4],dummy[5],dummy[6],dummy[7]);
		graphics_draw_text(binded_display_ctx,x,y,tmp);
		sprintf(tmp,"%c %c %c %c %c %c %c %c",dummy[8],dummy[9],dummy[10],dummy[11],dummy[12],dummy[13],
		dummy[14],dummy[15]);
		graphics_draw_text(binded_display_ctx,x,y+8,tmp);
	}
	#endif

	emu_run();

#endif
	//Dont bother cleaning up memory.There's nowhere to return to  
	while(1){}
    return 0;
}


 

/**/
 
static void info_print() {
#if MAX_SAVE_SLOTS != 0
 	char buf[64];
	extern int lcd_scale;
	if (!screen_info_changed) {
		return;
	}
	
	

	if (3 != lcd_scale) {
 
		screen_info_changed = 0;
		int32_t x = ((screen_w>>1) - ((160*lcd_scale)>>1))    ;
		int32_t y = ((screen_h>>1) - ((144*lcd_scale)>>1))  + (144*lcd_scale) + 4;
 
		graphics_fill_screen(binded_display_ctx, 0); //Slow but who cares ;D
		sprintf(buf,"Save slot:%d ",save_slot);
		graphics_draw_text(binded_display_ctx,x,y,buf);
	}
#else


#endif
}



static void take_screenshot(const char* path,const uint8_t* data,const int32_t width,const int32_t height,const int32_t bpp) {
	
	uint8_t buffer[512*4];


	fs_handle_t* f = fs_open(path,"wb");
	if (!f) { return; }

	//Targa format 

	uint8_t* hdr = buffer;
	memset(hdr,0,18);
	hdr[2]=2;
	hdr[12]=width&0xff;
	hdr[13]=width>>8;
	hdr[14]=height&0xff;
	hdr[15]=height>>8;
	hdr[16]= bpp<<3;
	data_cache_hit_writeback_invalidate(hdr,sizeof(hdr));

	fs_write(f,(void*)hdr,18);

	uint32_t buffer_p = 0;
	
	int32_t x,x2,y,w2 = width * bpp;

	if (4==bpp) {
		for (y = 0;y < height;++y) {
			uint8_t* baddr =&frame_buffer[(height-1-y)*w2];

			for (x = 0;x < w2;) {
				if (x + sizeof(buffer) <= w2) {
					buffer_p = sizeof(buffer);
				} else {
					buffer_p = w2 - x;
				}
				
				if (0 == buffer_p)break;
				uint8_t* pbuf = &buffer[0];
				
				for (x2 = x + buffer_p;x < x2;x+=4,pbuf += 4) {
					uint32_t c = *(uint32_t*)&baddr[x];
					//rgba->bgra(alpha is wasted space)
					pbuf[0] = (c>>8)&0xff; 
					pbuf[1] = (c>>16)&0xff; 
					pbuf[2] = (c>>24)&0xff; 
					pbuf[3] = 0xff;
				}
				
				data_cache_hit_writeback_invalidate(buffer,buffer_p);
				fs_write(f,(const void*)(buffer),buffer_p);
			}
		}
	} else {
		for (y = 0;y < height;++y) {
			uint8_t* baddr =&frame_buffer[(height-1-y)*w2];

			for (x = 0;x < w2;) {
				if ((x + (sizeof(buffer))) <= w2) {
					buffer_p = sizeof(buffer);
				} else {
					buffer_p = w2 - x;
				}
				if (0 == buffer_p)break;

				uint8_t* pbuf = &buffer[0];
				for (x2 = x + buffer_p;x < x2;x+=2,pbuf += 2) {

					uint16_t c=*(uint16_t*)&baddr[x];

					//Convert r5g5b5a1 to a1r5g5b5
					//Just remove the alpha bit by moving right in 1 place , promote b15 if it was set ... and save in LE byte order ;D
					c = ((c>>1)) | ((!!(c>>1)&1) << 15);

					pbuf[0]=c&0xff;
					pbuf[1]=c>>8;
				}

				
				data_cache_hit_writeback_invalidate(buffer,buffer_p);
				fs_write(f,(const void*)(buffer),buffer_p);
			}
		}
	}

	fs_close(f);
}

void lcd_new_pass() {
	info_print();

	if (rdp_compiled_scene) {
		rdp_attach_display(binded_display_ctx);
		rdp_srec_execute_block(rdp_compiled_scene,1);
		rdp_detach_display(); 
	}

}

void doevents()
{
	
	extern byte g_emu_running;
	extern int lcd_scale;
	controller_read(&controller); 
	controller_begin_state();

#if __FS_BUILD__ != FS_N64_ROMFS
		if (controller_btn_status(C_left)) {
			controller_end_state();
	
			disable_interrupts(); 
			screen_info_changed = 1;

			int32_t upd = 1;
			const char* scale_modes[] = {"Z = Scale(1x)","Z = Scale(2x)","Z = Scale(3x)"};
			while (1) {
				if (upd) {
					upd = 0;
					graphics_fill_screen(binded_display_ctx, 0);
					graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("A = Resume")<<3)>>1),(screen_h>>1) - (8/2),
					"A = Resume");
					graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("B = Exit")<<3)>>1),(screen_h>>1) - ((8/2)+8),
					"B = Exit");
					if (frame_buffer_w != 256) {
						graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen(scale_modes[lcd_scale-1])<<3)>>1),
						(screen_h>>1) - ((8/2)+8+8),scale_modes[lcd_scale-1]);
					}
				}
				controller_read(&controller); 
				controller_begin_state();
				if (controller_btn_status(A)) {
					g_emu_running = 1;
					break;
				} else if (controller_btn_status(B)) {
					g_emu_running = 0;
					break;
				} else if ((frame_buffer_w != 256) && controller_btn_status(Z)) {
					lcd_scale = (lcd_scale + 1) % 4;
					if (lcd_scale < 1) { lcd_scale = 1; }
					upd = 1;	
					fb_centered();
					lcd_init();
				}
				controller_end_state();
			}
			graphics_fill_screen(binded_display_ctx, 0);
			controller_end_state();
			enable_interrupts();

			return;
		} else if (controller_btn_status(C_right)) {
			char conv[64];
			disable_interrupts();
			fs_drv_sync();
			//fs_drv_set_sdc_speed(0);
			
			sprintf(conv,"gnuboy/screenshots/emu_std%d.tga",screenshot_n++);
			take_screenshot(conv,frame_buffer,frame_buffer_w,frame_buffer_h,frame_buffer_bpp);
#if 0
			if (rdp_scene_buffer)
			take_screenshot("gnuboy/screenshots/rdp_rtt.tga",UncachedAddr(rdp_scene_buffer->data),rdp_scene_buffer->width,rdp_scene_buffer->height,rdp_scene_buffer->bitdepth);
#endif
			//fs_drv_set_sdc_speed(1);
			enable_interrupts();
		} else if (controller_btn_status(C_up)) {
			uint16_t tmp = save_slot;
			save_slot += save_slot < MAX_SAVE_SLOTS;
			screen_info_changed = save_slot != tmp;
		} else if (controller_btn_status(C_down)) {
			uint16_t tmp = save_slot;
			save_slot -= save_slot > 0;
			screen_info_changed = save_slot != tmp;
		} else if (controller_btn_status(L)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Saving state..")<<3)>>1),(screen_h>>1)-(8/2),"Saving state..");
			fs_drv_sync();
			fs_drv_set_sdc_speed(0);
			state_save(save_slot);
			fs_drv_set_sdc_speed(1);
			enable_interrupts();
		} else if (controller_btn_status(R)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Loading state..")<<3)>>1),(screen_h>>1)-(8/2),
			"Loading state..");
			fs_drv_sync();
			fs_drv_set_sdc_speed(0);
			state_load(save_slot);
			fs_drv_set_sdc_speed(1);
			enable_interrupts();
		}
#else

		if (controller_btn_pressed(C_left)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Saving SRAM+RTC..")<<3)>>1),(screen_h>>1)-(8/2),"Saving SRAM+RTC..");
			sram_save();
			rtc_save();
			enable_interrupts();
			delay_secs(1);
		} else if (controller_btn_pressed(C_right)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Loading SRAM+RTC..")<<3)>>1),(screen_h>>1)-(8/2),"Loading SRAM+RTC..");
			sram_load();
			rtc_load();
			enable_interrupts();
			delay_secs(1);
		}  else if (controller_btn_pressed(C_up)) {
			graphics_fill_screen(binded_display_ctx, 0);
			g_curr_snd_driver = (g_curr_snd_driver + 1) % 3;
			switch (g_curr_snd_driver) {
				case 0:
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("SOUND DRIVER:OFF")<<3)>>1),(screen_h>>1)-(8/2),"SOUND DRIVER:OFF");
					pcm_rbuf_set_mode(PCMRBUF_MODE_IMMEDIATE);
				break;
				case 1:
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("SOUND DRIVER:NON BLOCKING")<<3)>>1),(screen_h>>1)-(8/2),"SOUND DRIVER:NON BLOCKING");
					pcm_rbuf_set_mode(PCMRBUF_MODE_IMMEDIATE);
				break;
				case 2:
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("SOUND DRIVER:IMMEDIATE")<<3)>>1),(screen_h>>1)-(8/2),"SOUND DRIVER:IMMEDIATE");
					pcm_rbuf_set_mode(PCMRBUF_MODE_IMMEDIATE);
				break;
			}
			delay_secs(4);
			graphics_fill_screen(binded_display_ctx, 0);
		}   else if (controller_btn_status(L)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Saving state..")<<3)>>1),(screen_h>>1)-(8/2),"Saving state..");
			state_save(save_slot);
			enable_interrupts();
			delay_secs(1);
		} else if (controller_btn_status(R)) {
			disable_interrupts();
			screen_info_changed = 1;
			graphics_draw_text(binded_display_ctx,(screen_w>>1) - ((strlen("Loading state..")<<3)>>1),(screen_h>>1)-(8/2),
			"Loading state..");
			state_load(save_slot);
			enable_interrupts();
			delay_secs(1);
		}
#endif
 
	pad_begin();
		pad_set(PAD_A,controller_btn_pressed(A) != 0);
		pad_set(PAD_B,controller_btn_pressed(B) != 0);
		pad_set(PAD_START,controller_btn_pressed(start) != 0);
		pad_set(PAD_SELECT,controller_btn_pressed(Z) != 0);

		pad_set(PAD_UP,controller_btn_pressed(up) != 0);
		pad_set(PAD_DOWN,controller_btn_pressed(down) != 0);
		pad_set(PAD_LEFT,controller_btn_pressed(left) != 0);
		pad_set(PAD_RIGHT,controller_btn_pressed(right) != 0);
	pad_end(); //Handle hw ints once

	controller_end_state();
}


void vid_setpal(int i, int r, int g, int b) { 

}
 
void vid_settitle(char *title) {

}
  
void pcm_init() {
	pcm_rbuf_reset();
}

int pcm_submit() {
	if (g_curr_snd_driver)
		pcm_rbuf_cycle();

	return 1;
}

void pcm_close() {
	pcm_rbuf_reset();
}

void sys_checkdir(char *path, int wr)
{
 
}

