
/*Interface : neo_2_asm.h*/

.section .text

.align 4
.set nomips16
.set push
.set noreorder
.set noat

/*BEGIN neo2_recv_sd_multi_psram_no_ds */
.global neo2_recv_sd_multi_psram_no_ds
.ent    neo2_recv_sd_multi_psram_no_ds
		neo2_recv_sd_multi_psram_no_ds:

		la $14,PSRAM_ADDR			/*PSRAM rel offs*/
		lw $14,0($14)
		la $15,0xB30E6060			/* $15 = 0xB30E6060*/
		ori $12,$5,0				/* $12 = count*/

		addiu $sp,$sp,-(8 * 4)
		sw $16,0($sp)
		sw $17,4($sp)
		sw $18,8($sp)
		sw $19,12($sp)
		sw $20,16($sp)
		sw $21,20($sp)
		sw $22,24($sp)
		sw $23,28($sp)

		addi $16,$0,0x04			/*shift/inc addr by 4*/
		addi $22,$0,0x08			/*shift by 8*/
		addi $23,$0,-1				/*dec by 1*/

		ori $18,$0,0xf000			/*mask = 0000 f000*/
		ori $19,$0,0x0f00			/*mask = 0000 0f00*/
		ori $20,$0,0x00f0			/*mask = 0000 00f0*/
		ori $21,$0,0x000f			/*mask = 0000 000f*/

		lui $24,0xF000				/*mask = f000 0000*/
		lui $25,0x0F00				/*mask = 0f00 0000*/
		lui $10,0x00F0				/*mask = 00f0 0000*/
		lui $17,0x000F				/*mask = 000f 0000*/
		lui $1,0xb000				/*$at = psram base ; b000 0000*/

		neo2_recv_sd_multi_psram_no_ds_oloop:
		lui $11,0x0001				/* $11 = timeout = 64 * 1024*/
		addi $8,$0,0x0100
		addi $9,$0,0x01

		neo2_recv_sd_multi_psram_no_ds_tloop:
		lw $2,($15)					/* rdMmcDatBit4*/
		and $2,$2,$8				/* eqv of (data>>8)&0x01*/
		beq $2,$0,neo2_recv_sd_multi_psram_no_ds_get_sect			/* start bit detected*/
		nop
		bne $11,$0,neo2_recv_sd_multi_psram_no_ds_tloop			/* not timed out*/
		addu $11,$11,$9
		beq $11,$0,neo2_recv_sd_multi_psram_no_ds_exit		/* timeout*/
		addu $2,$0,$0					/* res = FALSE*/

		neo2_recv_sd_multi_psram_no_ds_get_sect:
		addi $13,$0,128				/* $13 = long count*/

		neo2_recv_sd_multi_psram_no_ds_gsloop:

		lw $2,($15)					/* rdMmcDatBit4 => -a-- -a--*/
		sll $2,$2,$16				/* a--- a---*/

		lw $3,($15)					/* rdMmcDatBit4 => -b-- -b--*/
		and $2,$2,$24				/* a000 0000*/
		and $3,$3,$25				/* 0b00 0000*/

		lw $4,($15)					/* rdMmcDatBit4 => -c-- -c--*/
		or $11,$3,$2				/* $11 = ab00 0000*/
		srl $4,$4,$16				/* --c- --c-*/

		lw $5,($15)					/* rdMmcDatBit4 => -d-- -d--*/
		and $4,$4,$10				/* 00c0 0000*/
		srl $5,$5,$22				/* ---d ---d*/
		or $11,$11,$4				/* $11 = abc0 0000*/

		lw $6,($15)					/* rdMmcDatBit4 => -e-- -e--*/
		and $5,$5,$17				/* 000d 0000*/
		sll $6,$6,$16				/* e--- e---*/
		or $11,$11,$5				/* $11 = abcd 0000*/

		lw $7,($15)					/* rdMmcDatBit4 => -f-- -f--*/
		and $6,$6,$18				/* 0000 e000*/
		or $11,$11,$6				/* $11 = abcd e000*/
		and $7,$7,$19				/* 0000 0f00*/

		lw $8,($15)					/* rdMmcDatBit4 => -g-- -g--*/
		or $11,$11,$7				/* $11 = abcd ef00*/
		srl $8,$8,$16				/* --g- --g-*/

		lw $9,($15)					/* rdMmcDatBit4 => -h-- -h--*/
		and $8,$8,$20				/* 0000 00g0*/
		or $11,$11,$8				/* $11 = abcd efg0*/

		srl $9,$9,$22				/* ---h ---h*/
		and $9,$9,$21				/* 0000 000h*/
		or $11,$11,$9				/* $11 = abcd efgh*/

		addu $9,$1,$14				/*psram base + rel*/
		sw $11,0($9)				/*save sector data*/
		lbu $8,0($1)				/*dl bus*/
		addu $13,$13,$23			/*dec long_cnt*/
		
		bne $13,$0,neo2_recv_sd_multi_psram_no_ds_gsloop
		addu $14,$14,$16				/*inc psram offs*/

		lw $2,($15)					/* rdMmcDatBit4 - just toss checksum bytes */
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/

		addu $12,$12,$23			/* count--*/
		bne $12,$0,neo2_recv_sd_multi_psram_no_ds_oloop	/* next sector*/
		lw $2,($15)					/* rdMmcDatBit4 - clock out end bit*/

		addi $2,$0,1					/* res = TRUE*/

		neo2_recv_sd_multi_psram_no_ds_exit:
		lw $16,0($sp)
		lw $17,4($sp)
		lw $18,8($sp)
		lw $19,12($sp)
		lw $20,16($sp)
		lw $21,20($sp)
		lw $22,24($sp)
		lw $23,28($sp)
		
		la $8,PSRAM_ADDR
		sw $14,0($8)

	jr $ra
	addiu $sp,$sp,(8 * 4)

.end neo2_recv_sd_multi_psram_no_ds
/*END neo2_recv_sd_multi_psram_no_ds */

/*BEGIN neo2_recv_sd_psram_ds1 */
.global neo2_recv_sd_psram_ds1
.ent    neo2_recv_sd_psram_ds1
		neo2_recv_sd_psram_ds1:

		la $14,PSRAM_ADDR			/*PSRAM rel offs*/
		lw $14,0($14)
		la $15,0xB30E6060			/* $15 = 0xB30E6060*/
		ori $12,$5,0				/* $12 = count*/

		addiu $sp,$sp,-(8 * 4)
		sw $16,0($sp)
		sw $17,4($sp)
		sw $18,8($sp)
		sw $19,12($sp)
		sw $20,16($sp)
		sw $21,20($sp)
		sw $22,24($sp)
		sw $23,28($sp)

		addi $16,$0,0x04			/*shift/inc addr by 4*/
		addi $22,$0,0x08			/*shift by 8*/
		addi $23,$0,-1				/*dec by 1*/

		ori $18,$0,0xf000			/*mask = 0000 f000*/
		ori $19,$0,0x0f00			/*mask = 0000 0f00*/
		ori $20,$0,0x00f0			/*mask = 0000 00f0*/
		ori $21,$0,0x000f			/*mask = 0000 000f*/

		lui $24,0xF000				/*mask = f000 0000*/
		lui $25,0x0F00				/*mask = 0f00 0000*/
		lui $10,0x00F0				/*mask = 00f0 0000*/
		lui $17,0x000F				/*mask = 000f 0000*/
		lui $1,0xb000				/*$at = psram base ; b000 0000*/

		neo2_recv_sd_psram_ds1_oloop:
		lui $11,0x0001				/* $11 = timeout = 64 * 1024*/
		addi $8,$0,0x0100
		addi $9,$0,0x01

		neo2_recv_sd_psram_ds1_tloop:
		lw $2,($15)					/* rdMmcDatBit4*/
		and $2,$2,$8				/* eqv of (data>>8)&0x01*/
		beq $2,$0,neo2_recv_sd_psram_ds1_get_sect			/* start bit detected*/
		nop
		bne $11,$0,neo2_recv_sd_psram_ds1_tloop			/* not timed out*/
		addu $11,$11,$9
		beq $11,$0,neo2_recv_sd_psram_ds1_exit		/* timeout*/
		addu $2,$0,$0					/* res = FALSE*/

		neo2_recv_sd_psram_ds1_get_sect:
		addi $13,$0,128				/* $13 = long count*/

		neo2_recv_sd_psram_ds1_gsloop:

		lw $4,($15)					/* rdMmcDatBit4 => -c-- -c--*/
		lw $5,($15)					/* rdMmcDatBit4 => -d-- -d--*/

		lw $2,($15)					/* rdMmcDatBit4 => -a-- -a--*/
		sll $2,$2,$16				/* a--- a---*/

		lw $3,($15)					/* rdMmcDatBit4 => -b-- -b--*/
		and $2,$2,$24				/* a000 0000*/
		and $3,$3,$25				/* 0b00 0000*/

		or $11,$3,$2				/* $11 = ab00 0000*/
		srl $4,$4,$16				/* --c- --c-*/

		and $4,$4,$10				/* 00c0 0000*/
		srl $5,$5,$22				/* ---d ---d*/
		or $11,$11,$4				/* $11 = abc0 0000*/

		lw $8,($15)					/* rdMmcDatBit4 => -g-- -g--*/
		lw $9,($15)					/* rdMmcDatBit4 => -h-- -h--*/

		lw $6,($15)					/* rdMmcDatBit4 => -e-- -e--*/
		and $5,$5,$17				/* 000d 0000*/
		sll $6,$6,$16				/* e--- e---*/
		or $11,$11,$5				/* $11 = abcd 0000*/

		lw $7,($15)					/* rdMmcDatBit4 => -f-- -f--*/
		and $6,$6,$18				/* 0000 e000*/
		or $11,$11,$6				/* $11 = abcd e000*/
		and $7,$7,$19				/* 0000 0f00*/

		or $11,$11,$7				/* $11 = abcd ef00*/
		srl $8,$8,$16				/* --g- --g-*/
		and $8,$8,$20				/* 0000 00g0*/
		or $11,$11,$8				/* $11 = abcd efg0*/

		srl $9,$9,$22				/* ---h ---h*/
		and $9,$9,$21				/* 0000 000h*/
		or $11,$11,$9				/* $11 = abcd efgh*/

		addu $9,$1,$14				/*psram base + rel*/
		sw $11,0($9)				/*save sector data*/
		lbu $8,0($1)				/*dl bus*/
		addu $13,$13,$23			/*dec long_cnt*/
		
		bne $13,$0,neo2_recv_sd_psram_ds1_gsloop
		addu $14,$14,$16				/*inc psram offs*/

		lw $2,($15)					/* rdMmcDatBit4 - just toss checksum bytes */
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/
		lw $2,($15)					/* rdMmcDatBit4*/

		addu $12,$12,$23			/* count--*/
		bne $12,$0,neo2_recv_sd_psram_ds1_oloop	/* next sector*/
		lw $2,($15)					/* rdMmcDatBit4 - clock out end bit*/

		addi $2,$0,1					/* res = TRUE*/

		neo2_recv_sd_psram_ds1_exit:
		lw $16,0($sp)
		lw $17,4($sp)
		lw $18,8($sp)
		lw $19,12($sp)
		lw $20,16($sp)
		lw $21,20($sp)
		lw $22,24($sp)
		lw $23,28($sp)
		
		la $8,PSRAM_ADDR
		sw $14,0($8)

	jr $ra
	addiu $sp,$sp,(8 * 4)

.end neo2_recv_sd_psram_ds1
/*END neo2_recv_sd_psram_ds1 */

/**************************/

.global neo2_recv_sd_multi
.ent    neo2_recv_sd_multi
		neo2_recv_sd_multi:

 

		la $15,last_sector_before_failure
		sw $0,($15)

		la $15,0xB30E6060		 /* $15 = 0xB30E6060*/
		ori $14,$4,0		      /* $14 = buf*/
		ori $12,$5,0		      /* $12 = count*/

		lui $24,0xF000
		lui $25,0x0F00
		lui $10,0x00F0
		lui $1,0x000F
		
		oloop:
		lui $11,0x0001		    /* $11 = timeout = 64 * 1024*/

		tloop:
		lw $2,($15)		       /* rdMmcDatBit4*/
		andi $2,$2,0x0100		 /* eqv of (data>>8)&0x01*/
		beq $2,$0,getsect		 /* start bit detected*/
		nop
		bne $11,$0,tloop		  /* not timed out*/
		addiu $11,$11,-1
		beq $11,$0,___exit		/* timeout*/
		ori $2,$0,0		       /* res = FALSE*/

		/*Remember WHERE it failed (if it failed)*/
		la $2,last_sector_before_failure
		lw $3,($2)
		addi $3,$3,1
		sw $3,($2)

		getsect:
		ori $13,$0,128		    /* $13 = long count*/

		gsloop:
		lw $2,($15)		       /* rdMmcDatBit4 => -a-- -a--*/
		sll $2,$2,4		       /* a--- a---*/

		lw $3,($15)		       /* rdMmcDatBit4 => -b-- -b--*/
		and $2,$2,$24		     /* a000 0000*/
		and $3,$3,$25		     /* 0b00 0000*/

		lw $4,($15)		       /* rdMmcDatBit4 => -c-- -c--*/
		or $11,$3,$2		      /* $11 = ab00 0000*/
		srl $4,$4,4		       /* --c- --c-*/

		lw $5,($15)		       /* rdMmcDatBit4 => -d-- -d--*/
		and $4,$4,$10		     /* 00c0 0000*/
		srl $5,$5,8		       /* ---d ---d*/
		or $11,$11,$4		     /* $11 = abc0 0000*/

		lw $6,($15)		       /* rdMmcDatBit4 => -e-- -e--*/
		and $5,$5,$1		     /* 000d 0000*/
		sll $6,$6,4		       /* e--- e---*/
		or $11,$11,$5		     /* $11 = abcd 0000*/

		lw $7,($15)		       /* rdMmcDatBit4 => -f-- -f--*/
		andi $6,$6,0xF000		     /* 0000 e000*/
		or $11,$11,$6		     /* $11 = abcd e000*/
		and $7,$7,0x0F00		     /* 0000 0f00*/

		lw $8,($15)		       /* rdMmcDatBit4 => -g-- -g--*/
		or $11,$11,$7		     /* $11 = abcd ef00*/
		srl $8,$8,4		       /* --g- --g-*/

		lw $9,($15)		       /* rdMmcDatBit4 => -h-- -h--*/
		andi $8,$8,0x00F0		    /* 0000 00g0*/
		or $11,$11,$8		     /* $11 = abcd efg0*/

		srl $9,$9,8		       /* ---h ---h*/
		andi $9,$9,0x000F		     /* 0000 000h*/
		or $11,$11,$9		     /* $11 = abcd efgh*/

		sw $11,0($14)		     /* save sector data*/

		addiu $13,$13,-1
		bne $13,$0,gsloop
		addiu $14,$14,4		   /* inc buffer pointer */

		lw $2,($15)		       /* rdMmcDatBit4 - just toss checksum bytes */
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/
		lw $2,($15)		       /* rdMmcDatBit4*/

		addiu $12,$12,-1		  /* count--*/
		bne $12,$0,oloop		  /* next sector*/
		lw $2,($15)		       /* rdMmcDatBit4 - clock out end bit*/

		ori $2,$0,1		       /* res = TRUE*/

	___exit:
 
	jr $ra
	nop

.end neo2_recv_sd_multi


/**************************/

.global zero_memory_asm
.ent    zero_memory_asm
		zero_memory_asm:

		addu $t0,$a0,$a1
		zero_memory_asm_loop:
		sw $0,($a0)
		bne	$a0,$t0,zero_memory_asm_loop
		addiu $a0,$a0,4

		jr $ra
		nop
.end zero_memory_asm

.global neo_xferto_psram
.ent    neo_xferto_psram
		neo_xferto_psram:

	mfc0 $15,$12
	addi $8,$0,-2 /*la $8,~1*/
	and $15,$8
	mtc0 $15,$12

	lui $10,0xB000
	ori $8,$4,0
	addu $8,$8,$6
	addu $10,$10,$5

	0:
		lw $12,0($4)
		lbu $11,0($10)
		sw $12,0($10)

		addiu $10,$10,4
		
		bne $4,$8,0b
		addiu $4,$4,4

	ori $15,1 /*enable ints back*/
	mtc0 $15,$12

	jr $ra
	nop

.end neo_xferto_psram

.global neo_memcpy64
.ent    neo_memcpy64
		neo_memcpy64:

	ori $8,$5,0
	addu $8,$8,$6

	0:
		ld $9,($5)
		sd $9,($4)
		addiu $4,$4,8
		
	bne $5,$8,0b
	addiu $5,$5,8

	jr $ra
	nop
.end neo_memcpy64

.global neo_memcpy32
.ent    neo_memcpy32
		neo_memcpy32:

	ori $8,$5,0
	addu $8,$8,$6

	0:
		lw $9,($5)
		sw $9,($4)
		addiu $4,$4,4
		
	bne $5,$8,0b
	addiu $5,$5,4

	jr $ra
	nop
.end neo_memcpy32

.global neo_memcpy16
.ent    neo_memcpy16
		neo_memcpy16:

	ori $8,$5,0
	addu $8,$8,$6

	0:
		lhu $9,($5)
		sh $9,($4)
		addiu $4,$4,2
		
	bne $5,$8,0b
	addiu $5,$5,2

	jr $ra
	nop
.end neo_memcpy16

.set pop
.set reorder
.set at


