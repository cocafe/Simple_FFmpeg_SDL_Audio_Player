/**
 * Header file for testcase
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */

#pragma once

#ifdef COMPLIE_TEST

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