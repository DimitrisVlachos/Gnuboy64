#include "gnuboy.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu.h"
#include "sound.h"
#include "mem.h"
#include "lcd.h"
#include "rtc.h"
#include "pcm.h"

#include "hw.h"
#include "../pcm_ring_buf.h"

byte g_emu_running = 0;






void emu_init()
{
	g_emu_running = 0;
}


/*
 * emu_reset is called to initialize the state of the emulated
 * system. It should set cpu registers, hardware registers, etc. to
 * their appropriate values at powerup time.
 */

void emu_reset()
{
	hw_reset();
	lcd_reset();
	cpu_reset();
	mbc_reset();
	sound_reset();
	pcm_rbuf_reset();
	g_emu_running = 0;
}




/* emu_step()
	make CPU catch up with LCDC
*/
void emu_step()
{

	cpu_emulate( cpu.lcdc  );

}


/*
	Time intervals throughout the code, unless otherwise noted, are
	specified in double-speed machine cycles (2MHz), each unit
	roughly corresponds to 0.477us.
	
	For CPU each cycle takes 2dsc (0.954us) in single-speed mode
	and 1dsc (0.477us) in double speed mode.
	
	Although hardware gbc LCDC would operate at completely different
	and fixed frequency, for emulation purposes timings for it are
	also specified in double-speed cycles.
	
	line = 228 dsc (109us)
	frame (154 lines) = 35112 dsc (16.7ms)
	of which
		visible lines x144 = 32832 dsc (15.66ms)
		vblank lines x10 = 2280 dsc (1.08ms)
*/

 

void emu_run()
{
	g_emu_running = 1;
 
	lcd_init();
	lcd_begin();


	extern struct hw hw;
	extern struct pcm pcm;

	extern byte g_curr_snd_driver;

	if (hw.cgb) {
		pcm_rbuf_set_mode(PCMRBUF_MODE_NON_BLOCKING);
		g_curr_snd_driver = 1;
	} else {
		pcm_rbuf_set_mode(PCMRBUF_MODE_IMMEDIATE);
		g_curr_snd_driver = 2;
	}

	const unsigned long long t2 = TIMER_TICKS_LL(17 * 1000);
	unsigned long long t0=0,t1=0;
	while (g_emu_running != 0)
	{
		//t0 = get_ticks();
		cpu_emulate(2280   );
 
		while ( R_LY > 0 &&  R_LY < 144) {
			emu_step();
		}

		rtc_tick();
		sound_mix();
 		pcm_submit();
		doevents();

		if (!(R_LCDC & 0x80)) {
			cpu_emulate(32832);
		}

		while ( R_LY > 0  ) {
			emu_step();
		}
		
		//while (get_ticks()-t0 < t2);
	}
}

