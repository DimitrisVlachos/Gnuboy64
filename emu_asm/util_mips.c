/*
	Mips (heavily-!)optimized routines  ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos) 
*/
 
int util_find_non_zero_pos_mips(const void* src,int sz) { 
	int res = 0;
 
	/*Find non zero byte in src  */
	
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text	\n"
		".align 4\n"

		"beq	%0,$0,55f\n" 
		"addu	$t0,$0,%1\n"	//src
		"addu	$t1,$t0,%2\n"	//&src[sz]

		"addu	$t3,$0,$t0\n"	//save src
		"addiu	$t6,$0,1\n"

		//Optimized for one branch ;D
		"0:\n"
		"lbu	$t2,0($t0)\n"	//*src
		"sltu	$t7,$t0,$t1\n"	//t0<t1?
		"subu	$t5,$t6,$t7\n"	//1-(t0<t1)
		"sll	$t5,$t5,24\n"	//24^(1-(t0<t1))
		"addu	$t2,$t2,$t5\n"	//  += 24^(1-(t0<t1))
		"beq	$t2,$0,0b\n"
		"addu	$t0,$t0,$t7\n"
 
		//res
		"subu	%0,$t0,$t3\n"
		"beq	%0,$0,55f\n"
		"nop\n"
		"addiu	%0,%0,-1\n"

		"55:\n"
	
		".set	pop\n"
		: "=r"(res)
		: "r"(src) , "r"(sz)
		: "$0"
	);
	return res;
}
 

void util_cpy_lcd_buf_mips(void* dst,const void* src) { 
 
  
	asm  volatile (
		".set	push\n"
		".set	noreorder\n"
		".text	\n"
		".align 4\n"

 		"addiu	$sp,$sp,-(10 * 4)\n"
		"sw		$s0,0($sp)\n"
		"sw		$s1,4($sp)\n"
		"sw		$s2,8($sp)\n"
		"sw		$s3,12($sp)\n"
		"sw		$s4,16($sp)\n"
		"sw		$s5,20($sp)\n"
		"sw		$s6,24($sp)\n"
		"sw		$s7,28($sp)\n"
		"sw		$fp,32($sp)\n"
		"sw		$ra,36($sp)\n"


		"addu	$fp,$0,%0\n"	//src
		"addu	$ra,$0,%1\n"	//dst
		"addu	$t0,$0,%0\n"	//src
		"addu	$t1,$0,%1\n"	//dst

		"lw		$t2,0($t0)\n"
		"lw		$t3,4($t0)\n"
		"lw		$t4,8($t0)\n"
		"lw		$t5,12($t0)\n"
		"lw		$t6,16($t0)\n"
		"lw		$t7,20($t0)\n"
		"lw		$t8,24($t0)\n"
		"lw		$t9,28($t0)\n"
		"lw		$a0,32($t0)\n"
		"lw		$a1,36($t0)\n"
		"lw		$a2,40($t0)\n"
		"lw		$a3,44($t0)\n"
		"lw		$s0,48($t0)\n"
		"lw		$s1,52($t0)\n"
		"lw		$s2,56($t0)\n"
		"lw		$s3,60($t0)\n"
		"lw		$s4,64($t0)\n"
		"lw		$s5,68($t0)\n"
		"lw		$s6,72($t0)\n"
		"lw		$s7,76($t0)\n"
		"sw		$t2,0($t1)\n"
		"sw		$t3,4($t1)\n"
		"sw		$t4,8($t1)\n"
		"sw		$t5,12($t1)\n"
		"sw		$t6,16($t1)\n"
		"sw		$t7,20($t1)\n"
		"sw		$t8,24($t1)\n"
		"sw		$t9,28($t1)\n"
		"sw		$a0,32($t1)\n"
		"sw		$a1,36($t1)\n"
		"sw		$a2,40($t1)\n"
		"sw		$a3,44($t1)\n"
		"sw		$s0,48($t1)\n"
		"sw		$s1,52($t1)\n"
		"sw		$s2,56($t1)\n"
		"sw		$s3,60($t1)\n"
		"sw		$s4,64($t1)\n"
		"sw		$s5,68($t1)\n"
		"sw		$s6,72($t1)\n"
		"sw		$s7,76($t1)\n"

		"lw		$t2,80($fp)\n"
		"lw		$t3,84($fp)\n"
		"lw		$t4,88($fp)\n"
		"lw		$t5,92($fp)\n"
		"lw		$t6,96($fp)\n"
		"lw		$t7,100($fp)\n"
		"lw		$t8,104($fp)\n"
		"lw		$t9,108($fp)\n"
		"lw		$a0,112($fp)\n"
		"lw		$a1,116($fp)\n"
		"lw		$a2,120($fp)\n"
		"lw		$a3,124($fp)\n"
		"lw		$s0,128($fp)\n"
		"lw		$s1,132($fp)\n"
		"lw		$s2,136($fp)\n"
		"lw		$s3,140($fp)\n"
		"lw		$s4,144($fp)\n"
		"lw		$s5,148($fp)\n"
		"lw		$s6,152($fp)\n"
		"lw		$s7,156($fp)\n"
		"sw		$t2,80($ra)\n"
		"sw		$t3,84($ra)\n"
		"sw		$t4,88($ra)\n"
		"sw		$t5,92($ra)\n"
		"sw		$t6,96($ra)\n"
		"sw		$t7,100($ra)\n"
		"sw		$t8,104($ra)\n"
		"sw		$t9,108($ra)\n"
		"sw		$a0,112($ra)\n"
		"sw		$a1,116($ra)\n"
		"sw		$a2,120($ra)\n"
		"sw		$a3,124($ra)\n"
		"sw		$s0,128($ra)\n"
		"sw		$s1,132($ra)\n"
		"sw		$s2,136($ra)\n"
		"sw		$s3,140($ra)\n"
		"sw		$s4,144($ra)\n"
		"sw		$s5,148($ra)\n"
		"sw		$s6,152($ra)\n"
		"sw		$s7,156($ra)\n"

 		"lw		$s0,0($sp)\n"
		"lw		$s1,4($sp)\n"
		"lw		$s2,8($sp)\n"
		"lw		$s3,12($sp)\n"
		"lw		$s4,16($sp)\n"
		"lw		$s5,20($sp)\n"
		"lw		$s6,24($sp)\n"
		"lw		$s7,28($sp)\n"
		"lw		$fp,32($sp)\n"
		"lw		$ra,36($sp)\n"
		"addiu	$sp,$sp,(10 * 4)\n"

		".set	pop\n"
		:  
		: "r"(src) , "r"(dst)
		: "$0"
	);
}

