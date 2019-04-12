
/*
	Simple game importer for the rom version of GNUBOY64 ~ (Dimitrios Vlachos : https://github.com/DimitrisVlachos) 

	256byte header :
	0...4 : GAME SIZE (in bytes)
	4...7 : (optional)STATE FILE SIZE (in bytes)
	8..15 : GNUBOY64
	16..255 : 0xffs
	[if STATE FILE SIZE != 0 | Import STATE FILE]
	[if GAME SIZE != 0 | DMA GAME from ROM to RAM]

	Encoding :

	inline void encode32(unsigned char* dst,unsigned int val) {
		*(dst++) = (unsigned char)(val >> 24U);
		*(dst++) = (unsigned char)(val >> 16U);
		*(dst++) = (unsigned char)(val >> 8U);
		*(dst++) = (unsigned char)(val & 0xffU);
	}
*/
#include <string.h>
#include <stdio.h>
#include <malloc.h>

int flen(FILE* in) {
	int o = ftell(in);
	fseek(in,0,SEEK_END);
	int r = ftell(in);
	fseek(in,o,SEEK_SET);
	return r;
}

int main(int argc,char** argv) {

	printf("GNUBOY64 rom-pack tool ~(Dimitrios Vlachos : https://github.com/DimitrisVlachos) 2014\n");
	if (argc < 4) {
		printf("Usage : emu_binary.z64 rom.gb/gbc output.z64 gnuboystate_bin(optional)\n");
		printf("(don't use with byteswapped versions of emu)\n(It is assumed that emu was compiled with makefile -f makefile.rom)\n");
		return 1;
	}

	const char* emu_bin = argv[1];
	const char* game_bin = argv[2];
	const char* out_bin = argv[3];
	const char* state_bin = (argc > 4) ? argv[4] : NULL;
	const int emu_pad_size = 2 * 1024 * 1024;
	FILE* emu;
	FILE* game;
	FILE* rom;
 	FILE* state = NULL;

	emu = fopen(emu_bin,"rb");
	if (!emu) {
		printf("Cant open %s\n",emu_bin);
		return 1;
	}

	game = fopen(game_bin,"rb");
	if (!game) {
		fclose(emu);
		printf("Cant open %s\n",game_bin);
		return 1;
	}

	rom = fopen(out_bin,"wb");
	if (!rom) {
		fclose(emu);
		fclose(game);
		printf("Cant open %s\n",out_bin);
		return 1;
	}

	int i = flen(emu);
	int j = 0;
	int out_len = i;
	unsigned char pad_byte = 0;

	for (;j < i;++j)
		fputc(fgetc(emu),rom);

	for (;j < emu_pad_size;++j,++out_len,pad_byte^=1)
		fputc(pad_byte,rom);

	j = 0;
	i = flen(game);

	//16byte hdr ( game size(4b) + state file size(4b)  )
	char hdr[256];

	memset(hdr,0xff,sizeof(hdr));
	hdr[8] = 'G';
	hdr[9] = 'N';
	hdr[10] = 'U';
	hdr[11] = 'B';
	hdr[12] = 'O';
	hdr[13] = 'Y';
	hdr[14] = '6';
	hdr[15] = '4';
	{
		unsigned int ui = (unsigned int)i;
		hdr[0] = (char)(ui >> 24U);
		hdr[1] = (char)(ui >> 16U);
		hdr[2] = (char)(ui >> 8U);
		hdr[3] = (char)(ui & 0xffU);
	}	
	

	out_len += i;

	if (state_bin != NULL) {
		state = fopen(state_bin,"rb");
		if (!state) {
			fclose(emu);
			fclose(game);		
			fclose(rom);
			printf("Cant open %s\n",state_bin);
			return 1;
		}
		unsigned int b = flen(state);
		hdr[4] = (char)(b >> 24U);
		hdr[5] = (char)(b >> 16U);
		hdr[6] = (char)(b >> 8U);
		hdr[7] = (char)(b & 0xffU);
	} else memset(&hdr[4],0,4);

	fwrite(hdr,1,sizeof(hdr),rom);
	
	if (state != NULL) {
		int sa = 0,sb = flen(state);
		for (;sa < sb;++sa)
			fputc(fgetc(state),rom);

		out_len += sb;
	}

	for (;j < i;++j)
		fputc(fgetc(game),rom);

	int alen = (out_len + (1*1024*1024 - 1)) & (~(1*1024*1024 - 1));

	while (out_len < alen) {
		fputc(pad_byte,rom);pad_byte^=1;
		++out_len;
	}
 

	fclose(rom);	
	fclose(emu);
	fclose(game);
	if (state != NULL) {
		fclose(state);
		printf("\nEmu+Game+State packed to %s!\n",out_bin);
	} else {
		printf("\nEmu+Game packed to %s!\n",out_bin);
	}
	return 0;
}

