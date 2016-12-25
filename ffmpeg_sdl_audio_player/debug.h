#pragma once

#include "stdafx.h"

#include <conio.h>

#define InitDebugConsole		debug_console_init
#define DeinitDebugConsole		debug_console_exit

#ifdef DEBUG

#define pr_console(...)			do { _cprintf(__VA_ARGS__); } while (0)

int debug_console_init(void);
void debug_console_exit(void);

#else /* DEBUG */

#define pr_console(...)			do { } while (0)

static inline int debug_console_init(void)
{
	return 0;
}

static inline void debug_console_exit(void)
{

}

#endif /* DEBUG */