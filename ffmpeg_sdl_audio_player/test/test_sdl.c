/**
* This file test the ffmpeg audio decoding
* Produce a PCM audio stream into a file with given format
*
*		Author: cocafe <cocafehj@gmail.com>
*/

#include "..\stdafx.h"
#include "..\ffmpeg.h"
#include "..\sdl_snd.h"
#include "..\debug.h"

#include <test.h>

SDLAudioData *sad;
AVDataCtx *avd;

int64_t timebase;

HANDLE hThreadSDL;
LPDWORD wdSDLThreadID;

HANDLE hThreadFFM;
LPDWORD wdFFMThreadID;

int64_t ThreadFFMExit;
int64_t ThreadSDLExit;

HANDLE hSemPlayperiodDone;
HANDLE hSEmPlaybackDone;

HANDLE hSemBufferGet;
HANDLE hSemBufferGot;

HANDLE hSemFFMExit;
HANDLE hSemSDLExit;

typedef struct PCMBuffer {
	uint8_t *buffer_data;
	int32_t buffer_pos;
	int32_t buffer_len;	/* valid buffer size */
	int32_t buffer_size;
} PCMBuffer;

PCMBuffer buf_decode;
PCMBuffer buf_playback;

int pcm_buffer_init(PCMBuffer *buf, int32_t buf_size)
{
	if (!buf)
		return -EINVAL;

	buf->buffer_pos = 0;
	buf->buffer_len = 0;
	buf->buffer_size = buf_size;
	buf->buffer_data = (uint8_t *)calloc(1, buf->buffer_size);

	return 0;
}

void pcm_buffer_deinit(PCMBuffer *buf)
{
	if (!buf)
		return;

	free(buf->buffer_data);
}

int pcm_buffer_write(PCMBuffer *buf, uint8_t *data, int32_t size)
{
	memcpy(buf->buffer_data + buf->buffer_pos, data, size);
	buf->buffer_pos += size;
	buf->buffer_len = buf->buffer_pos;

	return 0;
}

int pcm_buffer_copy(PCMBuffer *dst, PCMBuffer *src, int32_t size)
{
	if (!dst || !src)
		return -EINVAL;

	if (size > dst->buffer_size)
		return -EINVAL;

	if (size > src->buffer_size)
		return -EINVAL;

	memcpy(dst->buffer_data, src->buffer_data, size);
	dst->buffer_len = size;

	return 0;
}

int pcm_buffer_is_full(PCMBuffer *buf)
{
	return (buf->buffer_pos >= buf->buffer_size);
}

int pcm_buffer_try_full(PCMBuffer *buf, int32_t write_size)
{
	return (buf->buffer_pos + write_size > buf->buffer_size);
}

int pcm_buffer_len_set(PCMBuffer *buf, int32_t len)
{
	if (!buf)
		return -EINVAL;

	buf->buffer_len = len;

	return 0;
}

int pcm_buffer_pos_reset(PCMBuffer *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buffer_pos = 0;

	return 0;
}

int pcm_buffer_pos_get(PCMBuffer *buf)
{
	return buf->buffer_pos;
}

int pcm_buffer_flush(PCMBuffer *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buffer_len = 1;
	buf->buffer_pos = 0;
	memset(buf->buffer_data, 0x00, buf->buffer_size);

	return 0;
}

void InitSemaphore(void)
{
	hSemPlayperiodDone = CreateSemaphore(NULL, 1, 1, NULL);
	hSEmPlaybackDone = CreateSemaphore(NULL, -2, 1, NULL);

	hSemBufferGet = CreateSemaphore(NULL, 0, 1, NULL);
	hSemBufferGot = CreateSemaphore(NULL, 0, 1, NULL);

	hSemSDLExit = CreateSemaphore(NULL, 0, 1, NULL);
	hSemFFMExit = CreateSemaphore(NULL, 0, 1, NULL);
}

void FreeSemaphore(void)
{
	CloseHandle(hSemPlayperiodDone);
	CloseHandle(hSEmPlaybackDone);
	CloseHandle(hSemBufferGet);
	CloseHandle(hSemBufferGot);
	CloseHandle(hSemFFMExit);
	CloseHandle(hSemSDLExit);
}

LRESULT WINAPI FFMThread(void)
{
	int ret;

	ThreadFFMExit = 0;

	timebase = avd->codec_ctx->time_base.num * AV_TIME_BASE / avd->codec_ctx->time_base.den;

	//av_seek_frame(avd->format_ctx, 0, (int64_t)(avd->format_ctx->streams[0]->duration * 0.90), AVSEEK_FLAG_ANY);

	while (1) {
		if (ThreadSDLExit)
			break;

		ret = avcodec_decode_file(avd);
		if (ret < 0) {
			if (avd->format_ctx->pb->eof_reached)
				pr_console("%s: eof reached\n", __func__);
			else
				pr_console("%s: error occurred or eof (%#04x)\n", __func__, AVSEEK_FLAG_ANY);

		} else {
			pr_console("pts: (%lu/%lu) nb_samples: %d pkt_sz: %d swr_nb_samples: %d swr_bufsz: %d\n",
				avd->frame->pts,
				avd->format_ctx->streams[avd->stream_idx]->duration,
				avd->frame->nb_samples,
				avd->frame->pkt_size,
				avd->swr->dst_nb_samples,
				avd->swr->dst_bufsize);

			avswr_param_update(avd->swr, avd->frame->nb_samples);

			/* swr resample */
			ret = avswr_convert(avd->swr, avd->frame->data);
			if (ret < 0) {
				pr_console("%s: error occurred in swr (%#04x)\n", __func__, ret);
				break;
			}

			avcodec_decode_unref(avd);
		}

		/* buffer is full or eof reached */
		if (pcm_buffer_try_full(&buf_decode, avd->swr->dst_bufsize) || 
			(avd->format_ctx->pb->eof_reached && pcm_buffer_pos_get(&buf_decode))) {

			/* wait: data playback done, wait to fill data init: 1 */
			if (WaitForSingleObject(hSemPlayperiodDone, INFINITE) == WAIT_OBJECT_0) {
				pr_console("%s: received play done\n", __func__);
			}

			/* signal: get buffer */
			ReleaseSemaphore(hSemBufferGet, 1, NULL);

			/* wait: buffer got */
			if (WaitForSingleObject(hSemBufferGot, INFINITE) == WAIT_OBJECT_0) {
				pr_console("%s: received buffer got\n", __func__);
			}

			pcm_buffer_flush(&buf_decode);
		}

		if (avd->format_ctx->pb->eof_reached) {
			break;
		}

		pcm_buffer_write(&buf_decode, avd->swr->dst_data[0], avd->swr->dst_bufsize);
	}

	ThreadFFMExit = 1;

	/* let sdl thread exit */
	ReleaseSemaphore(hSemBufferGet, 1, NULL);
	ReleaseSemaphore(hSemFFMExit, 1, NULL);

	ExitThread(0);
}

LRESULT WINAPI SDLThread(void)
{
	ThreadSDLExit = 0;

	SDL_PauseAudio(FALSE);

	while (1) {
		/* wait: buffer fill up */
		if (WaitForSingleObject(hSemBufferGet, INFINITE) == WAIT_OBJECT_0) {
			pr_console("%s: received buffer get\n", __func__);
		}

		if (ThreadFFMExit || (avd->format_ctx->pb->eof_reached && ThreadFFMExit)) {
			break;
		}

		pcm_buffer_copy(&buf_playback, &buf_decode, buf_decode.buffer_pos);

		/* signal: buffer copied */
		ReleaseSemaphore(hSemBufferGot, 1, NULL);

		SDL_LockAudio();
		sdl_audio_playback_refill(
			sad->playback_data,
			buf_playback.buffer_data,
			buf_playback.buffer_len,
			(int32_t)avd->swr->dst_nb_samples);
		SDL_UnlockAudio();

		sdl_audio_playback_pending(sad->playback_data);
		/* signal: we need new data to playback */
		ReleaseSemaphore(hSemPlayperiodDone, 1, NULL);
	}

	SDL_PauseAudio(TRUE);

	ThreadSDLExit = 1;

	/* let ffm thread exit */
	ReleaseSemaphore(hSemBufferGot, 1, NULL);
	ReleaseSemaphore(hSemSDLExit, 1, NULL);

	ExitThread(0);
}

int test_ffmpeg_swr_sdl_playback_buffered(
	const char *url_open,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	int ret = 0;

	avd = avdata_alloc();
	if (!avd)
		return 1;

	if (avdata_init(avd)) {
		return 1;
	}

	if (avdata_open_file(avd, url_open)) {
		return 1;
	}

	if (avswr_init(
		avd->swr,
		avd,
		target_chnl_layout,
		target_sample_rate,
		target_sample_fmt)) {
		return 1;
	}

	sad = sdl_audio_data_alloc();
	if (!sad)
		return 1;
	
	if (sdl_audio_data_init(sad,
							target_sample_rate,
							AUDIO_S16SYS,
							(int32_t)avd->swr->dst_nb_samples_max,
							av_get_channel_layout_nb_channels(avd->swr->dst_chnl_layout))) {
		return 1;
	}

	sdl_audio_playback_init(
		sad->playback_data, 
		SDL_MIX_MAXVOLUME);

	SDL_PauseAudio(FALSE);

	InitSemaphore();

	pcm_buffer_init(&buf_decode, avd->swr->dst_linesize * 4);
	pcm_buffer_init(&buf_playback, avd->swr->dst_linesize * 4);

	hThreadFFM = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FFMThread, NULL, 0, wdFFMThreadID);
	hThreadSDL = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SDLThread, NULL, 0, wdSDLThreadID);

	WaitForSingleObject(hSemFFMExit, INFINITE);
	WaitForSingleObject(hSemSDLExit, INFINITE);

	pcm_buffer_deinit(&buf_decode);
	pcm_buffer_deinit(&buf_playback);

	FreeSemaphore();

	sdl_audio_data_exit(sad);
	sdl_audio_data_free(&sad);

	avdata_exit(avd);
	avdata_free(&avd);

	return 0;
}

int test_ffmpeg_swr_sdl_playback(
	const char *url_open,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	SDLAudioData *sad;
	AVDataCtx *avd;
	int ret = 0;

	avd = avdata_alloc();
	if (!avd)
		goto err_ret;

	if (avdata_init(avd)) {
		goto free_av;
	}

	if (avdata_open_file(avd, url_open)) {
		goto free_av;
	}

	if (avswr_init(
		avd->swr,
		avd,
		target_chnl_layout,
		target_sample_rate,
		target_sample_fmt)) {
		goto free_av;
	}

	sad = sdl_audio_data_alloc();
	if (!sad)
		goto free_av;

	if (sdl_audio_data_init(sad,
		target_sample_rate,
		AUDIO_S16SYS,
		(int32_t)avd->swr->dst_nb_samples_max,
		av_get_channel_layout_nb_channels(avd->swr->dst_chnl_layout))) {
		goto free_sdl;
	}

	sdl_audio_playback_init(
		sad->playback_data,
		SDL_MIX_MAXVOLUME);

	SDL_PauseAudio(FALSE);

	while (1) {
		ret = avcodec_decode_file(avd);
		if (ret < 0) {
			if (avd->format_ctx->pb->eof_reached)
				pr_console("%s: eof reached\n", __func__);
			else
				pr_console("%s: error occurred during frame decode\n", __func__);

			break;
		}
		 
		pr_console("pts: (%lu/%lu) nb_samples: %d pkt_sz: %d swr_nb_samples: %d swr_bufsz: %d\n",
			avd->frame->pts,
			avd->format_ctx->streams[avd->stream_idx]->duration,
			avd->frame->nb_samples,
			avd->frame->pkt_size,
			avd->swr->dst_nb_samples,
			avd->swr->dst_bufsize);

		avswr_param_update(avd->swr, avd->frame->nb_samples);

		ret = avswr_convert(avd->swr, avd->frame->data);
		if (ret < 0) {
			pr_console("%s: error occurred during swr\n", __func__);
			break;
		}

		avcodec_decode_unref(avd);

		SDL_LockAudio();
		sdl_audio_playback_refill(
			sad->playback_data, 
			avd->swr->dst_data[0], 
			avd->swr->dst_bufsize, 
			(int32_t)avd->swr->dst_nb_samples);
		SDL_UnlockAudio();

		sdl_audio_playback_pending(sad->playback_data);
	}

free_sdl:
	sdl_audio_data_exit(sad);
	sdl_audio_data_free(&sad);

free_av:
	avdata_exit(avd);
	avdata_free(&avd);

	return 0;

err_ret:
	return -ENODATA;
}