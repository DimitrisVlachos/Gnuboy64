#include <libdragon.h>
#include "interrupts.h"

void ints_dump_regs(TExceptionBlock* e)
{
	//dummy
}

void onException(TExceptionBlock* e)
{
	switch(e->type)
	{
		case EXCEPTION_TYPE_RESET:
		return;

		case EXCEPTION_TYPE_CRITICAL:
		default:
			ints_dump_regs(e);
		return;
	}
}

void ints_setup()
{
	register_exception_handler(onException);
}
