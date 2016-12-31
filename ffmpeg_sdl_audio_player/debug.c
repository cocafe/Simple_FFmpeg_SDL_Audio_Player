/**
 * Common Debugging 
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */

#include "stdafx.h"

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

#ifdef DEBUG_PCM_OUTPUT

FILE *pcm_output;

int pcm_debug_file_open(char *filepath)
{
	pcm_output = fopen(filepath, "wb");
	if (!pcm_output) {
		pr_console("%s: failed to create output pcm file\n", __func__);
		return -ENODATA;
	}

	return 0;
}

void pcm_debug_file_close(void)
{
	if (pcm_output) {
		fflush(pcm_output);
		fclose(pcm_output);
	}
}

void pcm_debug_file_write(void *buf, size_t size, size_t count)
{
	if (pcm_output) {
		fwrite(buf, size, count, pcm_output);
	}
}
#endif