/*
	Mips (heavily-!)optimized routines  ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos) 
*/
#include "../lcd_tables/u32_mask.h"
#include "../lcd_tables/lcd_upix_flags.h"
  
 


void lcd_flush_vram_list_mips(int list_size,unsigned char* vram) { 
	/*The asm version of my vram indice list algorithm.. */

	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".set	noat\n"
		".text	\n"
		".align 4\n"

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"


		"addu	$t9,$0,%1\n"		//vram
		"la		$s1,patpix\n"		//patpix
		"la		$t0,patdirty\n"		//dirty flags
		"la		$t1,vram_list\n"	//vram list
		"addu	$t2,%0,%0\n"		//vram list end
		"addu	$t2,$t1,$t2\n"
		"la		$a2,lcd_upix_flags_vr0\n"
		"la		$a3,lcd_upix_flags_vr1\n"

		"0:\n"
		"lhu	$t3,0($t1)\n"	//$t3 = base
		"addu	$t4,$t0,$t3\n"	//dirty flag
		"sb		$0,($t4)\n"

		"addu	$t5,$0,$0\n"	//subloop 1 ptr
		"addiu	$s0,$0,7\n"		//subloop 1 iters		
		"sll	$t6,$t3,4\n"	//base << 4	
		"addu	$t6,$t9,$t6\n"	//&vram[base]

		"1:\n"
		"lbu	$t7,0($t6)\n"	//vram[a]
		"lbu	$t8,1($t6)\n"	//vram[a+1]		
		
		"addiu	$s2,$t3,1024\n" //tmp2 = &patpix[base+1024][j][0]; 
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s2,$t5\n"
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s1,$s2\n"	

#if 1
		//Use precomputed flags  
		"addu	$t7,$a2,$t7\n"
		"addu	$t8,$a3,$t8\n"
		"lbu	$s3,0($t7)\n"	//pair 1
		"lbu	$s4,0($t8)\n"
		"lbu	$s5,256($t7)\n"	//pair 2
		"lbu	$s6,256($t8)\n"
		"lbu	$s7,512($t7)\n"	//pair 3
		"lbu	$a0,512($t8)\n"
		"addu	$s3,$s3,$s4\n"
		"addu	$s5,$s5,$s6\n"
		"addu	$s7,$s7,$a0\n"
		"sb		$s3,0($s2)\n"
		"sb		$s5,1($s2)\n"
		"sb		$s7,2($s2)\n"
		"lbu	$s3,768($t7)\n"	//pair 4
		"lbu	$s4,768($t8)\n"
		"lbu	$s5,1024($t7)\n"	//pair 5
		"lbu	$s6,1024($t8)\n"
		"lbu	$s7,1280($t7)\n"	//pair 6
		"lbu	$a0,1280($t8)\n"
		"addu	$s3,$s3,$s4\n"
		"addu	$s5,$s5,$s6\n"
		"addu	$s7,$s7,$a0\n"
		"sb		$s3,3($s2)\n"
		"sb		$s5,4($s2)\n"
		"sb		$s7,5($s2)\n"
		"lbu	$s3,1536($t7)\n"	//pair 7
		"lbu	$s4,1536($t8)\n"
		"lbu	$s5,1792($t7)\n"	//pair 8
		"lbu	$s6,1792($t8)\n"
		"addu	$s3,$s3,$s4\n"
		"addu	$s5,$s5,$s6\n"
		"sb		$s3,6($s2)\n"
		"sb		$s5,7($s2)\n"
#else
		//Or play with bits
		"andi	$s3,$t7,1\n"	//bit0
		"andi	$s4,$t8,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,0($s2)\n"
		"srl	$s3,$t7,1\n"	//bit1
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,1\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,1($s2)\n"
		"srl	$s3,$t7,2\n"	//bit2
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,2\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,2($s2)\n"
		"srl	$s3,$t7,3\n"	//bit3
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,3\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,3($s2)\n"
		"srl	$s3,$t7,4\n"	//bit4
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,4\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,4($s2)\n"
		"srl	$s3,$t7,5\n"	//bit5
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,5\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,5($s2)\n"
		"srl	$s3,$t7,6\n"	//bit6
		"andi	$s3,$s3,1\n"	
		"srl	$s4,$t8,6\n"
		"andi	$s4,$s4,1\n"
		"sll	$s4,$s4,1\n"
		"or		$s4,$s4,$s3\n"
		"sb		$s4,6($s2)\n"
		"srl	$t7,$t7,7\n"	//bit7
		"srl	$t8,$t8,7\n"
		"sll	$t8,$t8,1\n"
		"or		$t8,$t8,$t7\n"
		"sb		$t8,7($s2)\n"
#endif

		"addu	$s2,$0,$t3\n"  //tmp = &patpix[base][j][0]; 
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s2,$t5\n"
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s1,$s2\n"	
		"addiu	$s3,$t3,1024\n" //tmp2 = &patpix[base+1024][j][0];   
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s3,$t5\n"
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s1,$s3\n"	
 
		"lbu	$s4,7($s3)\n"
		"lbu	$s5,6($s3)\n"
		"lbu	$s6,5($s3)\n"
		"lbu	$s7,4($s3)\n"
		"lbu	$a0,3($s3)\n"
		"lbu	$a1,2($s3)\n"
		"sb		$s4,0($s2)\n"
		"sb		$s5,1($s2)\n"
		"sb		$s6,2($s2)\n"
		"sb		$s7,3($s2)\n"
		"sb		$a0,4($s2)\n"
		"sb		$a1,5($s2)\n"
		"lbu	$s4,1($s3)\n"	//ran out of regs
		"lbu	$s5,0($s3)\n"
		"sb		$s4,6($s2)\n"
		"sb		$s5,7($s2)\n"

		"addiu	$t6,$t6,2\n"	//vram=base<<4 | j+j
		"bne	$t5,$s0,1b\n"
		"addi	$t5,$t5,1\n"

		"addu	$t5,$0,$0\n"	//subloop 2 ptr
		"addiu	$s0,$0,7\n"		//subloop 2 iters	
		
		"2:\n"
		"addiu	$s4,$0,7\n"		//7-j
		"sub	$s4,$s4,$t5\n"

		"addu	$s2,$0,$t3\n" //tmp = &patpix[base][sj][0];
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s2,$s4\n"
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s1,$s2\n"	
		"addiu	$s3,$t3,2048\n" //tmp2 = &patpix[base+2048][j][0];
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s3,$t5\n"
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s1,$s3\n"	

#if 0
		"lbu	$s4,0($s2)\n"
		"lbu	$s5,1($s2)\n"
		"lbu	$s6,2($s2)\n"
		"lbu	$s7,3($s2)\n"
		"sb		$s4,0($s3)\n"
		"sb		$s5,1($s3)\n"
		"sb		$s6,2($s3)\n"
		"sb		$s7,3($s3)\n"
		"lbu	$s4,4($s2)\n"
		"lbu	$s5,5($s2)\n"
		"lbu	$s6,6($s2)\n"
		"lbu	$s7,7($s2)\n"
		"sb		$s4,4($s3)\n"
		"sb		$s5,5($s3)\n"
		"sb		$s6,6($s3)\n"
		"sb		$s7,7($s3)\n"
#else
		//NOTE:PATPIX addresses are always aligned!
		"lw		$s4,0($s2)\n"
		"lw		$s5,4($s2)\n"
		"sw		$s4,0($s3)\n"
		"sw		$s5,4($s3)\n"
#endif

		"addiu	$s4,$0,7\n"		//7-j
		"sub	$s4,$s4,$t5\n"

		"addiu	$s2,$t3,1024\n" //tmp = &patpix[base+1024][sj][0];
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s2,$s4\n"
		"sll	$s2,$s2,3\n"
		"addu	$s2,$s1,$s2\n"	
		"addiu	$s3,$t3,3072\n" //tmp2 = &patpix[base+3072][j][0];
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s3,$t5\n"
		"sll	$s3,$s3,3\n"
		"addu	$s3,$s1,$s3\n"	

#if 0
		"lbu	$s4,0($s2)\n"
		"lbu	$s5,1($s2)\n"
		"lbu	$s6,2($s2)\n"
		"lbu	$s7,3($s2)\n"
		"sb		$s4,0($s3)\n"
		"sb		$s5,1($s3)\n"
		"sb		$s6,2($s3)\n"
		"sb		$s7,3($s3)\n"
		"lbu	$s4,4($s2)\n"
		"lbu	$s5,5($s2)\n"
		"lbu	$s6,6($s2)\n"
		"lbu	$s7,7($s2)\n"
		"sb		$s4,4($s3)\n"
		"sb		$s5,5($s3)\n"
		"sb		$s6,6($s3)\n"
		"sb		$s7,7($s3)\n"
#else
		//NOTE:PATPIX addresses are always aligned!
		"lw		$s4,0($s2)\n"
		"lw		$s5,4($s2)\n"
		"sw		$s4,0($s3)\n"
		"sw		$s5,4($s3)\n"
#endif

		"bne	$t5,$s0,2b\n"
		"addi	$t5,$t5,1\n"
		
		"bne	$t1,$t2,0b\n"
		"addiu	$t1,$t1,2\n"

 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		".set	pop\n"
		: 
		: "r" (list_size) , "r"(vram)
		: "$0"
	);
}
  
int lcd_spr_enum_mips_R_LCDC_case(void* obj_struct_base_ptr,void* vis_sprite_struct_base_ptr,const int* lut,int l_value) { 

	int nspr = 0;
 
 
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		 
		".text	\n"
		".align 4\n"

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$t0,$0,%1\n"		//obj_struct_base_ptr
		"addiu	$t1,$t0,((40 )*4)\n"	//&obj_struct_base_ptr[40]
		"addu	$t2,$0,%2\n"		//vis_sprite_struct_base_ptr
		"addu	$t3,$0,%3\n"		//lut
		"addu	$t4,$0,%4\n"		//l_value
		"addiu	$t5,$t4,16\n"		//l_value + 16
		"addu	$t6,$0,$0\n"		//nspr
		"addiu	$t7,$0,10\n"		//nspr max
		"addiu	$s0,$t3,256*4*1\n"	//lut+256
		"addiu	$s1,$t3,256*4*2\n"	//lut+256+256
		"la		$s7,patpix\n"		//patpix
		"la		$s6,~1\n" 

		"0:\n"
		"lbu	$a0,0($t0)\n"		//y
		//if (L >= o->y || L + 16 < o->y)
		"bge	$t4,$a0,1f\n"	
		"lbu	$a1,1($t0)\n"		//x
		"blt	$t5,$a0,1f\n"
		"lbu	$a2,2($t0)\n"		//pat
		"lbu	$a3,3($t0)\n"		//flags

		// VS[NS].x = (int)o->x - 8;
		"addi	$a1,$a1,-8\n"
		
		"addu	$s5,$0,$a3\n"		//backup flags

		//v = L - (int)o->y + 16; @@t8
		"sub	$t8,$t4,$a0\n"
		
		//pat = o->pat | lut_base[o->flags];  @@t9
		"sll	$a3,$a3,2\n"
		"addu	$t9,$t3,$a3\n"
		"addu	$s3,$s1,$a3\n"
		"addu	$s2,$s0,$a3\n"
		"addi	$t8,$t8,16\n"	//t8+16

		"sw		$a1,4($t2)\n"	//VS[NS].x = a1
		"lw		$t9,($t9)\n"
		"lw		$s2,($s2)\n"
		"lw		$s3,($s3)\n"
				
		//VS[NS].pal = lut_base[256 + o->flags];
		"sb		$s2,8($t2)\n"
		
		"or		$t9,$a2,$t9\n"

		//VS[NS].pri = lut_base[256 + 256 + o->flags];
		//Moved in DL slot "sb		$s3,9($t2)\n"
		
		//IF RLCDC!!

		"addiu	$s2,$0,8\n"
		"blt	$t8,$s2,5f\n"
		"and	$t9,$t9,$s6\n"

		"addiu	$t8,$t8,-8\n"
		"addiu	$t9,$t9,1\n"

		"5:\n"
 		//if (o->flags & 0x40) pat ^= 1;
		"andi	$s2,$s5,0x40\n"
		"beq	$s2,$0,6f\n"
		"sb		$s3,9($t2)\n"
		"xori	$t9,$t9,1\n"
		"6:\n"

		//IF RLCDC!!

		//VS[NS].buf = patpix[pat][v];
		"sll	$t9,$t9,3\n"
		"addu	$t9,$t9,$t8\n"
		"sll	$t9,$t9,3\n"
		"addu	$t9,$s7,$t9\n"	
		"sw		$t9,0($t2)\n"

		//if nspr == 10
#if 0
		//-1 branch option....
		"sltu	$s3,$t6,$t7\n"
		"addiu	$t6,$t6,1\n"
		"addiu	$s2,$0,1\n"
		"sub	$s2,$s2,$s3\n"
		"sll	$s2,$s2,16\n"
		"addu	$t0,$t0,$s2\n"
#else
		"beq	$t6,$t7,2f\n"
		"addiu	$t6,$t6,1\n"
#endif
		"addiu	$t2,$t2,16\n"		//+=sizeof(vissprite)

		"1:\n"
#if 0
		"blt	$t0,$t1,0b\n"
#else
		"bne	$t0,$t1,0b\n"
#endif
		"addiu	$t0,$t0,4\n"

		"2:\n"

 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		"addu	%0,$0,$t6\n"
		".set	pop\n"
		: "=r"(nspr)
		: "r"(obj_struct_base_ptr) , "r"(vis_sprite_struct_base_ptr) , "r"(lut) , "r"(l_value)
		: "$0"
	);

	return nspr;
}

int lcd_spr_enum_mips(void* obj_struct_base_ptr,void* vis_sprite_struct_base_ptr,const int* lut,int l_value) { 

	int nspr = 0;
 
 
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		 
		".text	\n"
		".align 4\n"

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$t0,$0,%1\n"		//obj_struct_base_ptr
		"addiu	$t1,$t0,((40 )*4)\n"	//&obj_struct_base_ptr[40]
		"addu	$t2,$0,%2\n"		//vis_sprite_struct_base_ptr
		"addu	$t3,$0,%3\n"		//lut
		"addu	$t4,$0,%4\n"		//l_value
		"addiu	$t5,$t4,16\n"		//l_value + 16
		"addiu	$s4,$t4,8\n"		//l_value + 8
		"addu	$t6,$0,$0\n"		//nspr
		"addiu	$t7,$0,10\n"		//nspr max
		"addiu	$s0,$t3,256*4*1\n"	//lut+256
		"addiu	$s1,$t3,256*4*2\n"	//lut+256+256
		"la		$s7,patpix\n"		//patpix
		"la		$s6,~1\n" 

		"0:\n"
		"lbu	$a0,0($t0)\n"		//y
		//if (L >= o->y || L + 16 < o->y) || (L + 8 >= o->y)  
		"bge	$t4,$a0,1f\n"	
		"lbu	$a1,1($t0)\n"		//x
		"blt	$t5,$a0,1f\n"
		"lbu	$a2,2($t0)\n"		//pat
		"bge	$s4,$a0,1f\n"
		"lbu	$a3,3($t0)\n"		//flags

		// VS[NS].x = (int)o->x - 8;
		"addi	$a1,$a1,-8\n"
		
		"addu	$s5,$0,$a3\n"		//backup flags

		//v = L - (int)o->y + 16; @@t8
		"sub	$t8,$t4,$a0\n"
		
		//pat = o->pat | lut_base[o->flags];  @@t9
		"sll	$a3,$a3,2\n"
		"addu	$t9,$t3,$a3\n"
		"addu	$s3,$s1,$a3\n"
		"addu	$s2,$s0,$a3\n"
		"addi	$t8,$t8,16\n"	//t8+16

		"sw		$a1,4($t2)\n"	//VS[NS].x = a1
		"lw		$t9,($t9)\n"
		"lw		$s2,($s2)\n"
		"lw		$s3,($s3)\n"
	
		//VS[NS].pal = lut_base[256 + o->flags];
		"sb		$s2,8($t2)\n"

		"or		$t9,$a2,$t9\n"

		//VS[NS].pri = lut_base[256 + 256 + o->flags];
		"sb		$s3,9($t2)\n"
 
		//VS[NS].buf = patpix[pat][v];
		"sll	$t9,$t9,3\n"
		"addu	$t9,$t9,$t8\n"
		"sll	$t9,$t9,3\n"
		"addu	$t9,$s7,$t9\n"	
		"sw		$t9,0($t2)\n"

		//if nspr == 10
#if 0
		//-1 branch option....
		"sltu	$s3,$t6,$t7\n"
		"addiu	$t6,$t6,1\n"
		"addiu	$s2,$0,1\n"
		"sub	$s2,$s2,$s3\n"
		"sll	$s2,$s2,16\n"
		"addu	$t0,$t0,$s2\n"
#else
		"beq	$t6,$t7,2f\n"
		"addiu	$t6,$t6,1\n"
#endif
		"addiu	$t2,$t2,16\n"		//+=sizeof(vissprite)

		"1:\n"
#if 0
		"blt	$t0,$t1,0b\n"
#else
		"bne	$t0,$t1,0b\n"
#endif
		"addiu	$t0,$t0,4\n"

		"2:\n"

 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		"addu	%0,$0,$t6\n"
		".set	pop\n"
		: "=r"(nspr)
		: "r"(obj_struct_base_ptr) , "r"(vis_sprite_struct_base_ptr) , "r"(lut) , "r"(l_value)
		: "$0"
	);

	return nspr;
}
 
void lcd_bg_scan_pri_mips(void* dest,void* src,int cnt,int i) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text	\n"
		".align 4\n"

		"addu	$t0,$0,%0\n"	//dest
		"addu	$t1,$0,%1\n"	//src
		"addu	$t2,$0,%2\n"	//cnt
		"andi	$t3,%3,31\n"	//i
		"la		$t4,u32_mask_table\n" //mask

 
		//<8
		"blt	$t2,$t5,1f\n"
		"addiu	$t5,$0,8\n"

		

		"andi	$t8,$t0,3\n"
		"bne	$t8,$0,0f\n"
		"addiu	$t7,$0,8\n"			//blk len
		
		//dest aligned
		"3:\n"

		"andi	$t5,$t3,31\n"		//i&31
 		"addu	$t5,$t1,$t5\n"		//src[i&31]
		"lw		$t6,($t6)\n"		//*src
		"addiu	$t3,$t3,1\n"		//++i
		"andi	$t6,$t6,128\n"		//*src&=128
		"sll	$t6,$t6,2\n"
		"addu	$t6,$t4,$t6\n"		//mask
		"lw		$t6,($t6)\n"

		//8x bytes
		"sw		$t6,0($t0)\n"
		"sw		$t8,4($t0)\n"
		"addiu	$t2,$t2,-8\n"		//cnt-=8
		"bge	$t2,$t7,3b\n"		
		"addiu	$t0,$t0,8\n"		//dest+=8
		"beq	$t2,$0,55f\n"
		"nop\n"
		"j	1f\n"
		"nop\n"

		//dest unaligned
		"0:\n"	
	
		"andi	$t5,$t3,31\n"		//i&31
 		"addu	$t5,$t1,$t5\n"		//src[i&31]
		"addiu	$t3,$t3,1\n"		//++i
		"lbu	$t6,($t6)\n"		//*src
		"andi	$t6,$t6,128\n"		//*src&=128

		//8x bytes
		"sb		$t6,0($t0)\n"
		"sb		$t6,1($t0)\n"
		"sb		$t6,2($t0)\n"
		"sb		$t6,3($t0)\n"
		"sb		$t6,4($t0)\n"
		"sb		$t6,5($t0)\n"
		"sb		$t6,6($t0)\n"
		"sb		$t6,7($t0)\n"

		"addiu	$t2,$t2,-8\n"		//cnt-=8
		"bge	$t2,$t7,0b\n"		
		"addiu	$t0,$t0,8\n"		//dest+=8

		"beq	$t2,$0,55f\n"
		"nop\n"

		//remainder
		"1:\n"
	 
		"andi	$t5,$t3,31\n"		//i&31
 		"addu	$t5,$t1,$t5\n"		//src[i&31]
		"addiu	$t3,$t3,1\n"		//++i
		"lbu	$t6,($t6)\n"		//*src
		"andi	$t6,$t6,128\n"		//*src&=128
		
		"2:\n"	
		"sb		$t6,0($t0)\n"
		"addiu	$t0,$t0,1\n"		//dest+=1
		"bne	$t2,$0,2b\n"		//cnt-=1
		"addiu	$t2,$t2,-1\n"

		"55:\n"


		".set	pop\n"
		: 
		: "r" (dest) , "r"(src) , "r" (cnt) , "r"(i)
		: "$0"
	);
}

void lcd_tilebuf_gbc_pass1_mips(unsigned char* tilemap,unsigned char* attrmap,int* tilebuf,int cnt,const int* attrs_utable,const int* wrap_table) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		//".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //attrmap
		"addu	$t2,$0,%2\n" //tilebuf
		"addu	$t3,$0,%3\n" //cnt	
		"addu	$t4,$0,%4\n" //attrs utable
		"addu	$t9,$0,%5\n" //wrap table


		//Unrolling this won't help much since reads are depended on previous wrap offset so stalls will be caused anyway
		"0:\n"
		"lbu	$t5,0($t1)\n"	
		"lbu	$t6,0($t0)\n"	
		"lw		$t7,0($t9)\n"	
		"sll	$t6,$t6,8\n"
		"addu	$t6,$t6,$t5\n"	
		"sll	$t6,$t6,2\n"	
		"addu	$t6,$t4,$t6\n"
		"andi	$t5,$t5,0x07\n"	
		"lw		$t6,0($t6)\n"	
		"sll	$t5,$t5,2\n"	
		"addiu	$t7,$t7,1\n"
		"addiu	$t3,$t3,-1\n"
		"addiu	$t9,$t9,4\n"
		"sw		$t6,0($t2)\n"	
		"sw		$t5,4($t2)\n"
		"add	$t0,$t0,$t7\n"
		"add	$t1,$t1,$t7\n"	
		"bne	$t3,$0,0b\n"
		"addiu	$t2,$t2,8\n"

		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(attrmap) , "r"(tilebuf),"r"(cnt),"r"(attrs_utable),"r"(wrap_table)
		: "$0"
	);
}
 
void lcd_tilebuf_gbc_pass2_mips(unsigned char* tilemap,unsigned char* attrmap,int* tilebuf,int cnt,const int* attrs_utable) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //attrmap
		"addu	$t2,$0,%2\n" //tilebuf
		"addu	$t3,$0,%3\n" //cnt	
		"addu	$t4,$0,%4\n" //attrs utable

		//Unrolling this won't help much since reads are depended on previous wrap offset so stalls will be caused anyway
		"0:\n"
		"lbu	$t5,0($t1)\n"	 
		"lbu	$t6,0($t0)\n"	 
		"sll	$t6,$t6,8\n"
		"addu	$t6,$t6,$t5\n"	 
		"sll	$t6,$t6,2\n"	 
		"addu	$t6,$t4,$t6\n"	 
		"andi	$t5,$t5,0x07\n"	 
		"lw		$t6,0($t6)\n"	 
		"sll	$t5,$t5,2\n" 
		"addiu	$t3,$t3,-1\n"
		"addiu	$t0,$t0,1\n"	 
		"addiu	$t1,$t1,1\n"	 
		"sw		$t6,0($t2)\n"	 
		"sw		$t5,4($t2)\n"	
		"bne	$t3,$0,0b\n"
		"addiu	$t2,$t2,8\n"

		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(attrmap) , "r"(tilebuf),"r"(cnt),"r"(attrs_utable)
		: "$0"
	);
}

void lcd_tilebuf_gb_pass1_stride_mips(unsigned char* tilemap,int* tilebuf,int cnt,const int* wrap_table) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		//".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //tilebuf
		"addiu	$t2,%2,-1\n" //cnt	
		"addu	$t4,$0,%3\n" //wrap table

		//Unrolling this won't help much since reads are depended on previous wrap offset so stalls will be caused anyway
		"0:\n"
		"lb		$t5,0($t0)\n"		//(s8)*tilemap
		"lw		$t6,0($t4)\n"		//*wrap
		"addiu	$t5,$t5,256\n"		//stride+(s8)*tilemap
		"addiu	$t6,$t6,1\n"		//++wrap
		"sw		$t5,0($t1)\n"		//tilebuf[0]=t5
		"addiu	$t4,$t4,4\n"
		"addu	$t0,$t0,$t6\n"
		"addiu	$t1,$t1,4\n"
		"bne	$t2,$0,0b\n"
		"addiu	$t2,$t2,-1\n"


		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(tilebuf),"r"(cnt),  "r"(wrap_table)
		: "$0"
	);
}

void lcd_tilebuf_gb_pass1_mips(unsigned char* tilemap,int* tilebuf,int cnt,const int* wrap_table) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		//".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //tilebuf
		"addiu	$t2,%2,-1\n" //cnt
		"addu	$t4,$0,%3\n" //wrap table

		//Unrolling this won't help much since reads are depended on previous wrap offset so stalls will be caused anyway
		"0:\n"
		"lbu	$t5,0($t0)\n"		//(u8)*tilemap
		"lw		$t6,0($t4)\n"		//*wrap
		"addiu	$t6,$t6,1\n"		//++wrap
		"sw		$t5,0($t1)\n"		//tilebuf[0]=t5
		"addiu	$t4,$t4,4\n"
		"addu	$t0,$t0,$t6\n"
		"addiu	$t1,$t1,4\n"
		"bne	$t2,$0,0b\n"
		"addiu	$t2,$t2,-1\n"


		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(tilebuf),"r"(cnt),   "r"(wrap_table)
		: "$0"
	);
}
 
void lcd_tilebuf_gb_pass2_mips(unsigned char* tilemap,int* tilebuf,int cnt) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		//".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //tilebuf
		"addiu	$t2,%2,-1\n" //cnt

		"addiu	$t3,$0,7\n"
		"blt	$t2,$t3,1f\n"
		"nop\n"

		//Optimize some other time : Read unaligned part + read long aligned the rest part
		"0:\n"
		"lbu	$t3,0($t0)\n"	
		"lbu	$t4,1($t0)\n"	
		"lbu	$t5,2($t0)\n"	
		"lbu	$t6,3($t0)\n"	
		"lbu	$t7,4($t0)\n"	
		"lbu	$t8,5($t0)\n"	
		"lbu	$t9,6($t0)\n"
		"lbu	$a0,7($t0)\n"	
		"sw		$t3,0($t1)\n"
		"sw		$t4,4($t1)\n"
		"sw		$t5,8($t1)\n"
		"sw		$t6,12($t1)\n"
		"sw		$t7,16($t1)\n"
		"sw		$t8,20($t1)\n"
		"sw		$t9,24($t1)\n"
		"sw		$a0,28($t1)\n"	


	
		"addiu	$t0,$t0,8\n"
		"addiu	$t3,$0,8\n"
		"addiu	$t2,$t2,-8\n"
		"bgt	$t2,$t3,0b\n"
		"addiu	$t1,$t1,4*8\n"

		"beq	$t2,$0,2f\n"
		"nop\n"

		"1:\n"
		"lbu	$t5,0($t0)\n"		 
		"sw		$t5,0($t1)\n"		 
		"addiu	$t1,$t1,4\n"
		"addiu	$t0,$t0,1\n"
		"bne	$t2,$0,1b\n"
		"addiu	$t2,$t2,-1\n"

		"2:\n"
		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(tilebuf),"r"(cnt)
		: "$0"
	);
}

void lcd_tilebuf_gb_pass2_stride_mips(unsigned char* tilemap,int* tilebuf,int cnt) {
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		//".set	noat\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //tilemap
		"addu	$t1,$0,%1\n" //tilebuf
		"addiu	$t2,%2,-1\n" //cnt

		"addiu	$t3,$0,7\n"
		"blt	$t2,$t3,1f\n"
		"nop\n"
		
		//Optimize some other time : Read unaligned part + read long aligned the rest part
		"0:\n"
		"lb		$t3,0($t0)\n"	
		"lb		$t4,1($t0)\n"	
		"lb		$t5,2($t0)\n"	
		"lb		$t6,3($t0)\n"	
		"lb		$t7,4($t0)\n"	
		"lb		$t8,5($t0)\n"	
		"lb		$t9,6($t0)\n"
		"lb		$a0,7($t0)\n"	
		"addiu	$t3,$t3,256\n"
		"addiu	$t4,$t4,256\n"
		"addiu	$t5,$t5,256\n"
		"addiu	$t6,$t6,256\n"
		"addiu	$t7,$t7,256\n"
		"addiu	$t8,$t8,256\n"
		"addiu	$t9,$t9,256\n"
		"addiu	$a0,$a0,256\n"

		"sw		$t3,0($t1)\n"
		"sw		$t4,4($t1)\n"
		"sw		$t5,8($t1)\n"
		"sw		$t6,12($t1)\n"
		"sw		$t7,16($t1)\n"
		"sw		$t8,20($t1)\n"
		"sw		$t9,24($t1)\n"
		"sw		$a0,28($t1)\n"	

		"addiu	$t0,$t0,8\n"
		"addiu	$t3,$0,8\n"
		"addiu	$t2,$t2,-8\n"
		"bgt	$t2,$t3,0b\n"
		"addiu	$t1,$t1,4*8\n"


		"beq	$t2,$0,2f\n"
		"nop\n"

		"1:\n"
		"lbu	$t5,0($t0)\n"		 
		"sw		$t5,0($t1)\n"		 
		"addiu	$t1,$t1,4\n"
		"addiu	$t0,$t0,1\n"
		"bne	$t2,$0,1b\n"
		"addiu	$t2,$t2,-1\n"

		"2:\n"
		".set	pop\n"
		: 
		: "r" (tilemap) , "r"(tilebuf),"r"(cnt)
		: "$0"
	);
}

void lcd_blendcpy_block_mips(void* dest,void* tile_buf,int vcoord,int cnt) { 

	/*	GBC mode : bg&wnd(just pass the corresponding V coord).
		Note cnt>0 always and we only do the intense blockcpy part.
		Reads/Writes are reduced by 80% if src/dst are aligned(in other words: the loop may run 80~160% faster most of the time).
	*/
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //dest
		"addu	$t1,$0,%1\n" //tilebuf
		"addu	$t2,$0,%2\n" //vcoord
		"addu	$t3,$t0,%3\n" //dest+cnt
		"la		$t4,patpix\n" //patpix  3d array
		//XXX t5 is reserved for src

		//Check for alignment
		"andi	$v0,$t0,3\n"
		"beq	$v0,$0,3f\n"
		"nop\n"

		//DST unaligned
		"0:\n"
		"subu	$t8,$t3,$t0\n"
		"li		$t6,8\n"
		"blt	$t8,$t6,20f\n"
		"nop\n"

		"lw		$t6,0($t1)\n"  
		"lw		$t7,4($t1)\n"  
		"addu	$t1,$t1,8\n"  
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n" 
		"sll	$t5,$t5,3\n"  
		"addu	$t5,$t4,$t5\n"


		//src aligned 
		"andi	$t6,$t5,3\n"
		"beq	$t6,$0,2f\n"
		"nop\n"

		//Unaligned all
		"1:\n"
		"lbu	$t6,0($t5)\n"		//1
		"lbu	$t8,1($t5)\n"		//2
		"lbu	$t9,2($t5)\n"		//3
		"lbu	$a0,3($t5)\n"		//4
		"lbu	$a1,4($t5)\n"		//5
		"lbu	$a2,5($t5)\n"		//6
		"lbu	$a3,6($t5)\n"		//7
		"lbu	$v0,7($t5)\n"		//8
		"or		$t6,$t6,$t7\n"
		"or		$t8,$t8,$t7\n"
		"or		$t9,$t9,$t7\n"
		"or		$a0,$a0,$t7\n"
		"or		$a1,$a1,$t7\n"
		"or		$a2,$a2,$t7\n"
		"or		$a3,$a3,$t7\n"
		"or		$v0,$v0,$t7\n"
		"sb		$t6,0($t0)\n"
		"sb		$t8,1($t0)\n"
		"sb		$t9,2($t0)\n"
		"sb		$a0,3($t0)\n"
		"sb		$a1,4($t0)\n"
		"sb		$a2,5($t0)\n"
		"sb		$a3,6($t0)\n"
		"sb		$v0,7($t0)\n"
		
		
		"j 0b\n"
		"addiu	$t0,$t0,8\n"

		"j	20f\n"
		"nop\n"

		//SRC aligned
		"2:\n"
		"la		$t8,u32_mask_table\n"
		"sll	$t7,$t7,2\n"
		"addu	$t8,$t8,$t7\n"
		"lw		$v0,0($t5)\n"
		"lw		$t7,($t8)\n"
		"lw		$v1,4($t5)\n"
		"or		$v0,$v0,$t7\n"
		"or		$v1,$v1,$t7\n"
		"srl	$t6,$v0,24\n"
		"srl	$t8,$v0,16\n"
		"srl	$t9,$v0,8\n"
 		"sb		$t6,0($t0)\n"
 		"sb		$t8,1($t0)\n"
 		"sb		$t9,2($t0)\n"
 		"sb		$v0,3($t0)\n"
		"srl	$t6,$v1,24\n"
		"srl	$t8,$v1,16\n"
		"srl	$t9,$v1,8\n"
 		"sb		$t6,4($t0)\n"
 		"sb		$t8,5($t0)\n"
 		"sb		$t9,6($t0)\n"
 		"sb		$v1,7($t0)\n"

		
		"j 0b\n"
		"addiu	$t0,$t0,8\n"

		"j	20f\n"
		"nop\n"


		//DST aligned
		"3:\n"
		"subu	$t8,$t3,$t0\n"
		"li		$t6,8\n"
		"blt	$t8,$t6,20f\n"
		"nop\n"

		"lw		$t6,0($t1)\n"  
		"lw		$t7,4($t1)\n" 
		"la		$t8,u32_mask_table\n"
		"sll	$t7,$t7,2\n"
		"addu	$t8,$t8,$t7\n"
		"lw		$t7,($t8)\n"
		"addu	$t1,$t1,8\n"  
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n" 
		"sll	$t5,$t5,3\n"  
		"addu	$t5,$t4,$t5\n"

		"andi	$t6,$t5,3\n"
		"beq	$t6,$0,5f\n"
		"nop\n"

		//Unaligned src	~reduce writes 
		"4:\n"
		"lbu	$t6,0($t5)\n"		//1
		"lbu	$t8,1($t5)\n"		//2
		"lbu	$t9,2($t5)\n"		//3
		"lbu	$a0,3($t5)\n"		//4
		"lbu	$a1,4($t5)\n"		//5
		"lbu	$a2,5($t5)\n"		//6
		"lbu	$a3,6($t5)\n"		//7
		"lbu	$v0,7($t5)\n"		//8
		"sll	$t6,$t6,24\n"
		"sll	$t8,$t8,16\n"
		"sll	$t9,$t9,8\n"
		"sll	$a1,$a1,24\n"
		"sll	$a2,$a2,16\n"
		"sll	$a3,$a3,8\n"
		"addu	$t6,$t6,$t8\n"
		"addu	$t6,$t6,$t9\n"
		"addu	$t6,$t6,$a0\n"
		"addu	$a1,$a1,$a2\n"
		"addu	$a1,$a1,$a3\n"
		"addu	$a1,$a1,$v0\n"
		"or		$t6,$t6,$t7\n"
		"or		$a1,$a1,$t7\n"
		"sw		$t6,0($t0)\n"
		"sw		$a1,4($t0)\n"

		
		"j 3b\n"
		"addiu	$t0,$t0,8\n"

		//SRC&DST aligned	~reduce both reads&writes by 80%  ~=160% faster
		"5:\n"
		"lw		$t6,0($t5)\n"
		"lw		$t8,4($t5)\n"
		"or		$t6,$t6,$t7\n"
		"or		$t8,$t8,$t7\n"
		"sw		$t6,0($t0)\n"
		"sw		$t8,4($t0)\n"

		
		"j 3b\n"
		"addiu	$t0,$t0,8\n"

		//Small block 
		"20:\n"
		"bge	$t0,$t3,22f\n"
		"nop\n"

		"lw		$t6,0($t1)\n" 
		"lw		$t7,4($t1)\n" 
		"addu	$t1,$t1,8\n"
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n"
		"sll	$t5,$t5,3\n" 
		"addu	$t5,$t4,$t5\n"

		"21:\n"
		"lbu	$t6,($t5)\n"
		"or		$t6,$t6,$t7\n"
		"sb		$t6,($t0)\n"
		"addiu	$t0,$t0,1\n"
		"bne	$t0,$t3,21b\n"
		"addiu	$t5,$t5,1\n"

		"22:\n"

		".set	pop\n"
		: 
		: "r" (dest), "r" (tile_buf) , "r"(vcoord) , "r" (cnt)
		: "$0"
	);
}

void lcd_cpy_block_mips(void* dest,void* tile_buf,int vcoord,int cnt) { 

	/*	GB mode : bg&wnd(just pass the corresponding V coord).
		Note cnt>0 always and we only do the intense blockcpy part.
		Reads/Writes are reduced by 80% if src/dst are aligned(in other words: the loop may run 80~160% faster most of the time).
	*/
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text\n"
		".align 4\n"

		"addu	$t0,$0,%0\n" //dest
		"addu	$t1,$0,%1\n" //tilebuf
		"addu	$t2,$0,%2\n" //vcoord
		"addu	$t3,$t0,%3\n" //dest+cnt
		"la		$t4,patpix\n" //patpix  3d array
		//XXX t5 is reserved for src

		//Check for alignment
		"andi	$v0,$t0,3\n"
		"beq	$v0,$0,3f\n"
		"nop\n"

		//DST unaligned
		"0:\n"
		"subu	$t8,$t3,$t0\n"
		"li		$t6,8\n"
		"blt	$t8,$t6,20f\n"
		"nop\n"

		"lw		$t6,0($t1)\n"   
		"addu	$t1,$t1,4\n"  
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n" 
		"sll	$t5,$t5,3\n"  
		"addu	$t5,$t4,$t5\n"


		//src aligned 
		"andi	$t6,$t5,3\n"
		"beq	$t6,$0,2f\n"
		"nop\n"

		//Unaligned all
		"1:\n"
		"lbu	$t6,0($t5)\n"		//1
		"lbu	$t8,1($t5)\n"		//2
		"lbu	$t9,2($t5)\n"		//3
		"lbu	$a0,3($t5)\n"		//4
		"lbu	$a1,4($t5)\n"		//5
		"lbu	$a2,5($t5)\n"		//6
		"lbu	$a3,6($t5)\n"		//7
		"lbu	$v0,7($t5)\n"		//8
		"sb		$t6,0($t0)\n"
		"sb		$t8,1($t0)\n"
		"sb		$t9,2($t0)\n"
		"sb		$a0,3($t0)\n"
		"sb		$a1,4($t0)\n"
		"sb		$a2,5($t0)\n"
		"sb		$a3,6($t0)\n"
		"sb		$v0,7($t0)\n"
		
		
		"j 0b\n"
		"addiu	$t0,$t0,8\n"

		"j	20f\n"
		"nop\n"

		//SRC aligned
		"2:\n"
		"lw		$v0,0($t5)\n"
		"lw		$v1,4($t5)\n"

		"srl	$t6,$v0,24\n"
		"srl	$t8,$v0,16\n"
		"srl	$t9,$v0,8\n"
 		"sb		$t6,0($t0)\n"
 		"sb		$t8,1($t0)\n"
 		"sb		$t9,2($t0)\n"
 		"sb		$v0,3($t0)\n"
		"srl	$t6,$v1,24\n"
		"srl	$t8,$v1,16\n"
		"srl	$t9,$v1,8\n"
 		"sb		$t6,4($t0)\n"
 		"sb		$t8,5($t0)\n"
 		"sb		$t9,6($t0)\n"
 		"sb		$v1,7($t0)\n"

		
		"j 0b\n"
		"addiu	$t0,$t0,8\n"

		"j	20f\n"
		"nop\n"


		//DST aligned
		"3:\n"
		"subu	$t8,$t3,$t0\n"
		"li		$t6,8\n"
		"blt	$t8,$t6,20f\n"
		"nop\n"

		"lw		$t6,0($t1)\n"  
		"addu	$t1,$t1,4\n"  
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n" 
		"sll	$t5,$t5,3\n"  
		"addu	$t5,$t4,$t5\n"

		"andi	$t6,$t5,3\n"
		"beq	$t6,$0,5f\n"
		"nop\n"

		//Unaligned src	~reduce writes 
		"4:\n"
		"lbu	$t6,0($t5)\n"		//1
		"lbu	$t8,1($t5)\n"		//2
		"lbu	$t9,2($t5)\n"		//3
		"lbu	$a0,3($t5)\n"		//4
		"lbu	$a1,4($t5)\n"		//5
		"lbu	$a2,5($t5)\n"		//6
		"lbu	$a3,6($t5)\n"		//7
		"lbu	$v0,7($t5)\n"		//8
		"sll	$t6,$t6,24\n"
		"sll	$t8,$t8,16\n"
		"sll	$t9,$t9,8\n"
		"sll	$a1,$a1,24\n"
		"sll	$a2,$a2,16\n"
		"sll	$a3,$a3,8\n"
		"addu	$t6,$t6,$t8\n"
		"addu	$t6,$t6,$t9\n"
		"addu	$t6,$t6,$a0\n"
		"addu	$a1,$a1,$a2\n"
		"addu	$a1,$a1,$a3\n"
		"addu	$a1,$a1,$v0\n"

		"sw		$t6,0($t0)\n"
		"sw		$a1,4($t0)\n"

		
		"j 3b\n"
		"addiu	$t0,$t0,8\n"

		//SRC&DST aligned	~reduce both reads&writes by 80%  ~=160% faster
		"5:\n"
		"lw		$t6,0($t5)\n"
		"lw		$t8,4($t5)\n"
		"sw		$t6,0($t0)\n"
		"sw		$t8,4($t0)\n"

		
		"j 3b\n"
		"addiu	$t0,$t0,8\n"

		//Small block 
		"20:\n"
		"bge	$t0,$t3,22f\n"
		"nop\n"

		"lw		$t6,0($t1)\n" 
		"addu	$t1,$t1,4\n"
		"sll	$t6,$t6,3\n" 
		"addu	$t5,$t6,$t2\n"
		"sll	$t5,$t5,3\n" 
		"addu	$t5,$t4,$t5\n"

		"21:\n"
		"lbu	$t6,($t5)\n"
		"sb		$t6,($t0)\n"
		"addiu	$t0,$t0,1\n"
		"bne	$t0,$t3,21b\n"
		"addiu	$t5,$t5,1\n"

		"22:\n"

		".set	pop\n"
		: 
		: "r" (dest), "r" (tile_buf) , "r"(vcoord) , "r" (cnt)
		: "$0"
	);
}

void lcd_recolor_mips(unsigned char *buf, unsigned char fill,int cnt) { /*GB mode*/

	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text\n"
		".align 4\n"
		//".set	noat\n"

		//<=0
		"blez %1,6f\n"
		"nop\n"

		"addu	$t0,$0,%0\n"
		"addu	$t1,$t0,%1\n"
		"addu	$t2,$0,%2\n"

		//Leq  8?
		"ble	%1,$t3,5f\n"
		"li		$t3,8\n"

		//Is addr aligned ?
		"andi	$t3,$t0,3\n"
		"bne	$t3,$0,0f\n"
		"nop\n"

		"j 2f\n"			
		"nop\n"

		//Not aligned - write unaligned part
		"0:\n"
		"li		$t4,4\n"	
		"sub	$t3,$t4,$t3\n"
		"subu	$t1,$t1,$t3\n"	//patch end offset
				
		"1:\n"	
		"lbu	$t4,0($t0)\n"
		"or		$t4,$t4,$t2\n"
		"sb		$t4,0($t0)\n"
		"addiu	$t0,$t0,1\n"
		"bne	$t3,$0,1b\n"
		"addi	$t3,$t3,-1\n"

		//Check if already done
		"beq	$t0,$t1,6f\n"
		"nop\n"

		//Addr is now aligned 
		"2:\n"

		"subu	$t4,$t1,$t0\n"
		"li		$t5,8\n"
		"ble	$t4,$t5,5f\n"
		"nop\n"

		//Load  mask (XXX $t5 = mask ~ don't touch)
		"3:\n"
		"la		$t5,u32_mask_table\n"
		"or		$t6,$0,$t2\n"
		"sll	$t6,$t6,2\n"
		"addu	$t6,$t5,$t6\n"
		"lw		$t5,($t6)\n"
		
		//Can we do 8x block copy?
		"subu	$t4,$t1,$t0\n"
		"li		$t6,(4*8)\n"
		"blt	$t4,$t6,4f\n"
		"nop\n"

		//Do 8x long copies 
		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$a0,$0,$t5\n"
		"addu	$a1,$0,$t5\n"
		"addu	$a2,$0,$t5\n"
		"addu	$a3,$0,$t5\n"
		"addu	$s4,$0,$a0\n"
		"addu	$s5,$0,$a1\n"
		"addu	$s6,$0,$a2\n"
		"addu	$s7,$0,$a3\n"
		
		"7:\n"
		"lw		$t4,0($t0)\n"		//1
		"lw		$t7,4($t0)\n"		//2
		"lw		$t8,8($t0)\n"		//3
		"lw		$t9,12($t0)\n"		//4
		"lw		$s0,16($t0)\n"		//5
		"lw		$s1,20($t0)\n"		//6
		"lw		$s2,24($t0)\n"		//7
		"lw		$s3,28($t0)\n"		//8

		"or		$t4,$t4,$t5\n"
		"or		$t7,$t7,$a0\n"
		"or		$t8,$t8,$a1\n"
		"or		$t9,$t9,$a2\n"
		"or		$s0,$s0,$s4\n"
		"or		$s1,$s1,$s5\n"
		"or		$s2,$s2,$s6\n"
		"or		$s3,$s3,$s7\n"

		"sw		$t4,0($t0)\n"
		"sw		$t7,4($t0)\n"
		"sw		$t8,8($t0)\n"
		"sw		$t9,12($t0)\n"
		"sw		$s0,16($t0)\n"
		"sw		$s1,20($t0)\n"
		"sw		$s2,24($t0)\n"
		"sw		$s3,28($t0)\n"
 
		"addiu	$t0,$t0,(4*8)\n"
		"ble	$t6,$t1,7b\n"
		"addiu	$t6,$t0,(4*8*1)\n" //check ahead

		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		//Check if already done
		"beq	$t0,$t1,6f\n"
		"nop\n"

		"subu	$t4,$t1,$t0\n"
		"li		$t6,8\n"
		"ble	$t4,$t6,5f\n"
		"nop\n"
 
		//Do 1x long copies 
		"4:\n"
		"lw		$t4,0($t0)\n"
		"or		$t4,$t4,$t5\n"
		"sw		$t4,0($t0)\n"
		"addiu	$t0,$t0,(4*1)\n"
		"ble	$t6,$t1,4b\n"
		"addiu	$t6,$t0,(4*1)\n" //check ahead

		//Check if already done
		"beq	$t0,$t1,6f\n"
		"nop\n"

		//remainder
		"5:\n"

		"lbu	$t4,0($t0)\n"
		"or		$t4,$t4,$t2\n"
		"sb		$t4,0($t0)\n"
		"bne	$t0,$t1,5b\n"
		"addiu	$t0,$t0,1\n"

		"6:\n"

		".set	pop\n"
		: 
		: "r" (buf), "r" (cnt) , "r"(fill)
		: "$0"
	);
}

void lcd_refresh_line_2_mips(void* dest,void* src,void* pal) { /*For 1xscale : 16bit mode*/
	asm volatile (
		".set	push\n"
		".set	noreorder\n"
		".set	noat\n"
		".text\n"
		".align 4\n"

 
		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$t0,$0,%0\n"
		"addu	$t1,$0,%1\n"
		"addu	$t2,$0,%2\n"
		"addu	$t4,$0,$0\n"
		"addu	$t5,$0,$0\n"
		"addu	$t6,$0,$0\n"
		"addu	$t7,$0,$0\n"
		"addiu	$t3,$t0,(((160/2) )*4)-(4*4)\n" 

		/*iters = linewidth/(sizeof(dw)/sizeof(w))/unrolls*sizeof(dw)-(sizeof(dw)*unrolls)*/
  
		"0:\n"
		"lbu	$t4,0($t1)\n"			//src[0]
		"lbu	$t5,1($t1)\n"			//src[1]
		"lbu	$a0,2($t1)\n"			//src[2]
		"lbu	$a1,3($t1)\n"			//src[3]
		"lbu	$s0,4($t1)\n"			//src[4]
		"lbu	$s1,5($t1)\n"			//src[5]
		"lbu	$s4,6($t1)\n"			//src[6]
		"lbu	$s5,7($t1)\n"			//src[7]

		"addu	$t4,$t4,$t4\n"			//src0*sizeof word
		"addu	$t5,$t5,$t5\n"			//src1*sizeof word
		"addu	$t4,$t2,$t4\n"			//src0+=paladdr
		"addu	$t5,$t2,$t5\n"			//src1+=paladdr
		"addu	$a0,$a0,$a0\n"			//src0*sizeof word
		"addu	$a1,$a1,$a1\n"			//src1*sizeof word
		"addu	$a0,$t2,$a0\n"			//src0+=paladdr
		"addu	$a1,$t2,$a1\n"			//src1+=paladdr
		"addu	$s0,$s0,$s0\n"			//src0*sizeof word
		"addu	$s1,$s1,$s1\n"			//src1*sizeof word
		"addu	$s0,$t2,$s0\n"			//src0+=paladdr
		"addu	$s1,$t2,$s1\n"			//src1+=paladdr
		"addu	$s4,$s4,$s4\n"			//src0*sizeof word
		"addu	$s5,$s5,$s5\n"			//src0*sizeof word
		"addu	$s4,$t2,$s4\n"			//src1+=paladdr
		"addu	$s5,$t2,$s5\n"			//src1+=paladdr

		"addiu	$t1,$t1,8\n"			//src+=8

		"lhu	$a2,0($a0)\n"			//*src[0]
		"lhu	$a3,0($a1)\n"			//*src[1]
		"lhu	$t6,0($t4)\n"			//*src[2]
		"lhu	$t7,0($t5)\n"			//*src[3]
		"lhu	$s2,0($s0)\n"			//*src[4]
		"lhu	$s3,0($s1)\n"			//*src[5]
		"lhu	$s6,0($s4)\n"			//*src[6]
		"lhu	$s7,0($s5)\n"			//*src[7]

		"sll	$t6,$t6,16\n"			//src0 << 16
		"sll	$a2,$a2,16\n"			//src0 << 16
		"sll	$s2,$s2,16\n"			//src0 << 16
		"sll	$s6,$s6,16\n"			//src0 << 16

		"addu	$a2,$a2,$a3\n"			//src0|=src1
		"addu	$t6,$t6,$t7\n"			//src0|=src1
		"addu	$s2,$s2,$s3\n"			//src0|=src1
		"addu	$s6,$s6,$s7\n"			//src0|=src1

		"sw		$t6,0($t0)\n"			//*u32(Dest)=src0
		"sw		$a2,4($t0)\n"			//*u32(Dest+4)=src0
		"sw		$s2,8($t0)\n"			//*u32(Dest+8)=src0
		"sw		$s6,12($t0)\n"			//*u32(Dest+12)=src0

		"bne	$t0,$t3,0b\n"
		"addiu	$t0,$t0,(4*4)\n"


 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"


		".set	pop\n"
		: 
		: "r" (dest), "r" (src) , "r"(pal)
		: "$0"
	);
}

void lcd_refresh_line_4_mips(void* dest,void* src,void* pal) { /*For 2xscale : 16bit mode*/

	asm volatile (
		".set	push\n"
		".set	noreorder\n"
		".set	noat\n"
		".text\n"
		".align 4\n"

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$t0,$0,%0\n"
		"addu	$t1,$0,%1\n"
		"addu	$t2,$0,%2\n"
		"addiu	$t3,$t0,(160*4)-(8*4)\n" 

		"0:\n"
		/*(8x words)*/
		"lbu	$t4,0($t1)\n"			//src[0]
		"lbu	$t5,1($t1)\n"			//src[1]
		"lbu	$a0,2($t1)\n"			//src[2]
		"lbu	$a1,3($t1)\n"			//src[3]
		"lbu	$s0,4($t1)\n"			//src[4]
		"lbu	$s1,5($t1)\n"			//src[5]
		"lbu	$s2,6($t1)\n"			//src[6]
		"lbu	$s3,7($t1)\n"			//src[7]

		"sll	$t4,$t4,2\n"			//src0*sizeof dword
		"sll	$t5,$t5,2\n"			//src1*sizeof dword
		"sll	$a0,$a0,2\n"			//src0*sizeof dword
		"sll	$a1,$a1,2\n"			//src1*sizeof dword
		"sll	$s0,$s0,2\n"			//src0*sizeof dword
		"sll	$s1,$s1,2\n"			//src1*sizeof dword
		"sll	$s2,$s2,2\n"			//src1*sizeof dword
		"sll	$s3,$s3,2\n"			//src1*sizeof dword

		"addiu	$t1,$t1,8\n"			//src+=8

		"addu	$t4,$t2,$t4\n"			//src0+=paladdr
		"addu	$t5,$t2,$t5\n"			//src1+=paladdr
		"addu	$a0,$t2,$a0\n"			//src0+=paladdr
		"addu	$a1,$t2,$a1\n"			//src1+=paladdr
		"addu	$s0,$t2,$s0\n"			//src0+=paladdr
		"addu	$s1,$t2,$s1\n"			//src1+=paladdr
		"addu	$s2,$t2,$s2\n"			//src0+=paladdr
		"addu	$s3,$t2,$s3\n"			//src1+=paladdr

		"lw		$t6,0($t4)\n"			//*src[0]
		"lw		$t7,0($t5)\n"			//*src[1]
		"lw		$a2,0($a0)\n"			//*src[0]
		"lw		$a3,0($a1)\n"			//*src[1]
		"lw		$s4,0($s0)\n"			//*src[0]
		"lw		$s5,0($s1)\n"			//*src[1]
		"lw		$s6,0($s2)\n"			//*src[0]
		"lw		$s7,0($s3)\n"			//*src[1]

		"sw		$t6,0($t0)\n"			//*u32(Dest+0)=src0
		"sw 	$t7,4($t0)\n"			//*u32(Dest+4)=src1
		"sw		$a2,8($t0)\n"			//*u32(Dest+8)=src0
		"sw 	$a3,12($t0)\n"			//*u32(Dest+12)=src1
		"sw		$s4,16($t0)\n"			//*u32(Dest+16)=src0
		"sw 	$s5,20($t0)\n"			//*u32(Dest+20)=src1
		"sw		$s6,24($t0)\n"			//*u32(Dest+24)=src0
		"sw 	$s7,28($t0)\n"			//*u32(Dest+28)=src1

		"bne	$t0,$t3,0b\n"
		"addiu	$t0,$t0,4*8\n"
 
 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		".set	pop\n"
		: 
		: "r" (dest), "r" (src) , "r"(pal)
		: "$0"
	);
}


void lcd_scan_color_cgb_mips(void* dst,const void* src,void* pr,void* bgr,int sz,int pal) { 
 
 
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text	\n"
		".align 4\n"

		"ble	%4,$0,55f\n" 
		"addu	$t0,$0,%1\n"	//src
		"addu	$t1,$t0,%4\n"	//&src[sz]

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$s0,$0,%0\n"	//dst
		"addu	$s1,$0,%2\n"	//pr
		"addu	$s2,$0,%3\n"	//bgr
		"addu	$s3,$0,%5\n"	//pal
		
		
		"addiu	$t6,$0,1\n"

		//Optimized for one branch ;D
		"1:\n"
		"addu	$t3,$0,$t0\n"	//save src

		"0:\n"
		"lbu	$t2,0($t0)\n"	//*src
		"sltu	$t7,$t0,$t1\n"	//t0<t1?
		"subu	$t5,$t6,$t7\n"	//1-(t0<t1)
		"sll	$t5,$t5,24\n"	//24^(1-(t0<t1))
		"addu	$t2,$t2,$t5\n"	//  += 24^(1-(t0<t1))
		"beq	$t2,$0,0b\n"	//keep scanning for nz byte
		"addu	$t0,$t0,$t7\n"	
 
		"bne	$t5,$0,5f\n"
		"subu	$s4,$t0,$t3\n"	//src-ssrc
		"addu	$s0,$s0,$s4\n"	//dst += diff
		"addu	$s1,$s1,$s4\n"	//pr += diff
		"beq	$t2,$0,1b\n"
 		"addu	$s2,$s2,$s4\n"	//bgr += diff

		"sltu	$s4,$s4,1\n"
		"lbu	$s5,-1($s2)\n"
		"lbu	$s4,-1($s1)\n"
		"andi	$s5,$s5,3\n"
		"sltu	$s4,$s4,1\n"
		"sltu	$s5,$s5,1\n"
		"beq	$s4,$0,1b\n"
		"or		$s4,$s4,$s5\n"
		"or		$t2,$t2,$s3\n"
		"j		1b\n"
 		"sb		$t2,-1($s0)\n"

		//res
		"5:\n"
 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		"55:\n"
	
		".set	pop\n"
		: 
		: "r"(dst) , "r"(src),"r"(pr),"r"(bgr),"r"(sz),"r"(pal)
		: "$0"
	);
 
}

void lcd_scan_color_pri_mips(void* dst,const void* src,void* pr,void* bgr,int sz,int pal) { 
 
	//pri is not used just beeing lazy :p ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos)
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text	\n"
		".align 4\n"

		"beq	%4,$0,55f\n" 
		"addu	$t0,$0,%1\n"	//src
		"addu	$t1,$t0,%4\n"	//&src[sz]

		"addiu	$sp,$sp,-(8 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"

		"addu	$s0,$0,%0\n"	//dst
		"addu	$s1,$0,%2\n"	//pr
		"addu	$s2,$0,%3\n"	//bgr
		"addu	$s3,$0,%5\n"	//pal
		
		
		"addiu	$t6,$0,1\n"

		//Optimized for one branch ;D
		"1:\n"
		"addu	$t3,$0,$t0\n"	//save src

		"0:\n"
		"lbu	$t2,0($t0)\n"	//*src
		"sltu	$t7,$t0,$t1\n"	//t0<t1?
		"subu	$t5,$t6,$t7\n"	//1-(t0<t1)
		"sll	$t5,$t5,24\n"	//24^(1-(t0<t1))
		"addu	$t2,$t2,$t5\n"	//  += 24^(1-(t0<t1))
		"beq	$t2,$0,0b\n"
		"addu	$t0,$t0,$t7\n"	//keep scanning for nz byte
 
		"bne	$t5,$0,5f\n"
		"subu	$s4,$t0,$t3\n"	//src-ssrc
		"addu	$s0,$s0,$s4\n"	//dst += diff
		"beq	$t2,$0,1b\n"
 		"addu	$s2,$s2,$s4\n"	//bgr += diff

		"lbu	$s5,-1($s2)\n"
		"bne	$s5,$0,1b\n"
		"andi	$s5,$s5,3\n"

		"or		$t2,$t2,$s3\n"
		"j		1b\n"
 		"sb		$t2,-1($s0)\n"

		//res
		"5:\n"
 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"addiu	$sp,$sp,(8 * 4)\n"

		"55:\n"
	
		".set	pop\n"
		: 
		: "r"(dst) , "r"(src),"r"(pr),"r"(bgr),"r"(sz),"r"(pal)
		: "$0"
	);
 
}



 
