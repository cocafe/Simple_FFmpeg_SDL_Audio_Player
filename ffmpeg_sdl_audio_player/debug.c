#include "stdafx.h"
#include "debug.h"

#ifdef DEBUG

int debug_console_init(void)
{
	if (!AllocConsole()) {
		MessageBoxErr(L"Failed to init debug console");
		return -ENODEV;
	}

	return 0;
}

void debug_console_exit(void)
{
	FreeConsole();
}

#endif /* DEBUG */