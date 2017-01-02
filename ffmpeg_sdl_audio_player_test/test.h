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

#pragma once

#ifdef COMPILE_PLAYER_TEST

/**
 * Test ffmpeg decode function,
 * save decoded PCM stream to a file
 */
int test_ffmpeg_decode(const char *url_open, const char *url_output);

/**
 * Test ffmpeg decode and SWR function,
 * save converted PCM stream to a file
 */
int test_ffmpeg_swr(
	const char *url_open,
	const char *url_output,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);

/**
 * Test ffmpeg decode and SWR with buffer,
 * save converted PCM stream to a file
 */
int test_ffmpeg_swr_buffered(
	const char *url_open,
	const char *url_output,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);

/**
* Test ffmpeg decode, SWR resampling, SDL output
*/
int test_ffmpeg_swr_sdl_playback(
	const char *url_open,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);

int test_ffmpeg_swr_sdl_playback_buffered(
	const char *url_open,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);

#endif