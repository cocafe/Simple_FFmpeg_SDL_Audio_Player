/**
 * Copyright 2016 cocafe <cocafehj@gmail.com> All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
