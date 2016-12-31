/**
 * Header file for debug
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */
 
#pragma once

#include "stdafx.h"

#include <conio.h>

#define PCM_DUMP_FILEPATH		"R:/output.pcm"

#define InitDebugConsole		debug_console_init
#define DeinitDebugConsole		debug_console_exit
#define CreatePCMDump			pcm_debug_file_open
#define FlushPCMDump			pcm_debug_file_close

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

#ifdef DEBUG_PCM_OUTPUT

int pcm_debug_file_open(char *filepath);
void pcm_debug_file_close(void);
void pcm_debug_file_write(void *buf, size_t size, size_t count);

#else

static inline int pcm_debug_file_open(char *filepath)
{
	return 0;
}

static inline void pcm_debug_file_close(void)
{

}

static inline void pcm_debug_file_write(void *buf, size_t size, size_t count)
{
	
}

#endif /* DEBUG_PCM_OUTPUT */