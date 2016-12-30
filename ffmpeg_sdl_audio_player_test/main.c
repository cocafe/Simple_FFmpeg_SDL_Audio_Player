/**
 * FFMPEG and SDL Player Test Main Entry
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */

#include "..\ffmpeg_sdl_audio_player\stdafx.h"
#include "..\ffmpeg_sdl_audio_player\ffmpeg.h"
#include "..\ffmpeg_sdl_audio_player\sdl_snd.h"
#include "..\ffmpeg_sdl_audio_player\debug.h"

#include "test.h"

INT APIENTRY wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdline,
	INT nCmdshow)
{
#ifdef COMPILE_PLAYER_TEST
	/* call the function that wanna test here */
	test_ffmpeg_swr_sdl_playback_buffered(
		"R:\testfile", 
		AV_CH_LAYOUT_STEREO, 
		AV_SAMPLE_RATE_44_1KHZ, 
		AV_SAMPLE_FMT_S16);
#endif

	return 0;
}
