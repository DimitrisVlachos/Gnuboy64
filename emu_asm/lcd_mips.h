
#ifndef _lcd_mips_h_
#define _lcd_mips_h_
/*
	Mips (heavily-!)optimized routines  ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos) 
*/

void lcd_flush_vram_list_mips(int list_size,unsigned char* vram);
int lcd_spr_enum_mips_R_LCDC_case(void* obj_struct_base_ptr,void* vis_sprite_struct_base_ptr,const int* lut,int l_value) ;
int lcd_spr_enum_mips(void* obj_struct_base_ptr,void* vis_sprite_struct_base_ptr,const int* lut,int l_value) ;
void lcd_tilebuf_gbc_pass1_mips(unsigned char* tilemap,unsigned char* attrmap,int* tilebuf,int cnt,const int* attrs_utable,const int* wrap_table);
void lcd_tilebuf_gbc_pass2_mips(unsigned char* tilemap,unsigned char* attrmap,int* tilebuf,int cnt,const int* attrs_utable);
void lcd_tilebuf_gb_pass1_stride_mips(unsigned char* tilemap,int* tilebuf,int cnt,const int* wrap_table) ;
void lcd_tilebuf_gb_pass1_mips(unsigned char* tilemap,int* tilebuf,int cnt,const int* wrap_table);
void lcd_tilebuf_gb_pass2_mips(unsigned char* tilemap,int* tilebuf,int cnt);
void lcd_tilebuf_gb_pass2_stride_mips(unsigned char* tilemap,int* tilebuf,int cnt);
void lcd_blendcpy_block_mips(void* dest,void* tile_buf,int vcoord,int cnt) ;
void lcd_cpy_block_mips(void* dest,void* tile_buf,int vcoord,int cnt) ;
void lcd_recolor_mips(unsigned char *buf, unsigned char fill,int cnt);
void lcd_refresh_line_2_mips(void* dest,void* src,void* pal) ;
void lcd_refresh_line_4_mips(void* dest,void* src,void* pal);
void lcd_bg_scan_pri_mips(void* dest,void* src,int cnt,int i);
void lcd_scan_color_cgb_mips(void* dst,const void* src,void* pr,void* bgr,int sz,int pal);
void lcd_scan_color_pri_mips(void* dst,const void* src,void* pr,void* bgr,int sz,int pal);
 
#endif

