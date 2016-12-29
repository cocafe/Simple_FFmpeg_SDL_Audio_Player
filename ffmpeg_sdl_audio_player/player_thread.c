#include "wnd.h"
#include "player.h"
#include "player_thread.h"

void player_threadctx_init(PlayerData *player)
{
	player->hSemPlayperiodDone = CreateSemaphore(NULL, 1, 2, NULL);
	player->hSemPlaybackFinish = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemPlaybackFFMStart = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackFFMPreSeek = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackFFMPostSeek = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemPlaybackSDLStart = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLPause = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLResume = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLPreSeek = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLPostSeek = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemBufferSend = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemBufferRecv = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemSDLExit = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemFFMExit = CreateSemaphore(NULL, 0, 1, NULL);

	player->ThreadFFMExit = 1;
	player->ThreadSDLExit = 1;
}

void player_threadctx_deinit(PlayerData *player)
{
	CloseHandle(player->hSemPlaybackFFMStart);
	CloseHandle(player->hSemPlaybackFFMPreSeek);
	CloseHandle(player->hSemPlaybackFFMPostSeek);
	CloseHandle(player->hSemPlaybackSDLStart);
	CloseHandle(player->hSemPlaybackSDLPause);
	CloseHandle(player->hSemPlaybackSDLResume);
	CloseHandle(player->hSemPlaybackSDLPreSeek);
	CloseHandle(player->hSemPlaybackSDLPostSeek);
	CloseHandle(player->hSemPlayperiodDone);
	CloseHandle(player->hSemPlaybackFinish);
	CloseHandle(player->hSemBufferSend);
	CloseHandle(player->hSemBufferRecv);
	CloseHandle(player->hSemFFMExit);
	CloseHandle(player->hSemSDLExit);
	CloseHandle(player->hThreadFFM);
	CloseHandle(player->hThreadSDL);

	player->hThreadFFM = NULL;
	player->hThreadSDL = NULL;

	player->ThreadFFMid = 0;
	player->ThreadSDLid = 0;

	player->ThreadFFMExit = 1;
	player->ThreadSDLExit = 1;
}


LRESULT WINAPI FFMThread(LPVOID data)
{
	PlayerData *player;
	PlaybackBuf *buf;
	AVDataCtx *avd;
	int ret;

	pr_console("%s: init\n", __func__);

	player = (PlayerData *)data;
	if (!player)
		goto err_nodata;

	avd = player->avd;
	buf = player->buf_decode;

	player->ThreadFFMExit = 0;

	/* signal: unblock sdl thread starting */
	ReleaseSemaphore(player->hSemPlaybackFFMStart, 1, NULL);

	/* wait: sync with sdl thread starting */
	if (WaitForSingleObject(player->hSemPlaybackSDLStart, INFINITE) == WAIT_OBJECT_0)
		pr_console("%s: synced with sdl thread starting\n", __func__);

	while (1) {
		ret = -1;

		if (!player->seeking)
			ret = avcodec_decode_file(avd);

		if (ret < 0) {
			pr_console("%s: error occurred in decoder (%#04x)\n", __func__, ret);
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

			player->pos_cur = avd->frame->pts;

			avcodec_decode_unref(avd);
		}

		if (avd->format_ctx->pb->eof_reached) {
			/* signal: notify daemon thread to reset playback */
			ReleaseSemaphore(player->hSemPlaybackFinish, 1, NULL);
			break;
		}

		/* don't exit early before releasing playback finish signal */
		if (player->ThreadSDLExit)
			break;

		if (player->playback_state == PLAYBACK_STOP) {
			break;
		}

		/* buffer is full or eof reached but we still have buf not flushed  */
		if (playback_buf_try_full(buf, avd->swr->dst_bufsize) ||
			(avd->format_ctx->pb->eof_reached && playback_buf_pos_get(buf))) {

			/* avoid seeking during av_read_frame() */
			if (player->seeking)
				ReleaseSemaphore(player->hSemPlaybackFFMPreSeek, 1, NULL);

			/* wait: data playback done, wait to fill data init: 1
			* we wait here if playback is paused,
			* use PlayPeriodDone semaphore to block
			*/
			if (WaitForSingleObject(player->hSemPlayperiodDone, INFINITE) == WAIT_OBJECT_0) {
				pr_console("%s: received play done\n", __func__);
			}

			/* we perform seeking after pause here, buffer will be flushed */
			if (buf->buf_len == 0) {
				continue;
			}

			/* signal: get buffer */
			ReleaseSemaphore(player->hSemBufferSend, 1, NULL);

			/* signal: update status bar*/
			NotifyUpdateMainStatusBar();

			/* wait: buffer got */
			if (WaitForSingleObject(player->hSemBufferRecv, INFINITE) == WAIT_OBJECT_0) {
				pr_console("%s: received buffer got\n", __func__);
			}

			playback_buf_flush(buf);
		}

		playback_buf_write(
			buf, avd->swr->dst_data[0],
			avd->swr->dst_bufsize,
			(int32_t)avd->swr->dst_nb_samples);
	}

	player->ThreadFFMExit = 1;

	/* unblock thread, let sdl thread exit */
	ReleaseSemaphore(player->hSemBufferSend, 1, NULL);

	/* signal: ffm thread exit */
	ReleaseSemaphore(player->hSemFFMExit, 1, NULL);

err_nodata:
	pr_console("%s: exit\n", __func__);

	ExitThread(0);
}

LRESULT WINAPI SDLThread(LPVOID data)
{
	PlayerData *player;
	PlaybackBuf *buf;
	SDLAudioData *sad;
	AVDataCtx *avd;
	int idx;

	pr_console("%s: init\n", __func__);

	player = (PlayerData *)data;
	if (!player)
		goto err_nodata;

	player->ThreadSDLExit = 0;

	sad = player->sad;
	avd = player->avd;
	buf = player->buf_playback;

	/* wait: sync with ffm thread staring */
	if (WaitForSingleObject(player->hSemPlaybackFFMStart, INFINITE) == WAIT_OBJECT_0)
		pr_console("%s: synced with ffm thread starting\n", __func__);

	/* signal: unblock ffm thread staring */
	ReleaseSemaphore(player->hSemPlaybackSDLStart, 1, NULL);

	while (1) {
		if (player->playback_state == PLAYBACK_STOP) {
			break;
		}

		/* wait: buffer fill up */
		if (WaitForSingleObject(player->hSemBufferSend, INFINITE) == WAIT_OBJECT_0) {
			pr_console("%s: received buffer get\n", __func__);
		}

		/* if ffm thread died, we catch that signal here */
		if (player->ThreadFFMExit) {
			break;
		}

		/* eof and wait for ffm thread, if still have buffer to flush */
		if (avd->format_ctx->pb->eof_reached && player->ThreadFFMExit) {
			break;
		}

		playback_buf_copy(
			buf,
			player->buf_decode,
			player->buf_decode->buf_pos);

		/* signal: buffer copied */
		ReleaseSemaphore(player->hSemBufferRecv, 1, NULL);

		if (player->playback_state == PLAYBACK_PAUSE) {
			SDL_PauseAudio(TRUE);

			/* wait: block thread */
			WaitForSingleObject(player->hSemPlaybackSDLPause, INFINITE);
			/* wait: block thread until state changed */
			WaitForSingleObject(player->hSemPlaybackSDLResume, INFINITE);

			SDL_PauseAudio(FALSE);
		}

		if (player->seeking) {
			SDL_PauseAudio(TRUE);

			/* signal: thread blocked, allow to seek */
			ReleaseSemaphore(player->hSemPlaybackSDLPreSeek, 1, NULL);
			/* wait: wait for seek done */
			WaitForSingleObject(player->hSemPlaybackSDLPostSeek, INFINITE);

			SDL_PauseAudio(FALSE);

			/* signal: require new buffer after seeking */
			ReleaseSemaphore(player->hSemPlayperiodDone, 2, NULL);
			continue;
		}

		playback_buf_pos_reset(buf);

		SDL_PauseAudio(FALSE);
		while (1) {
			idx = buf->buf_pos / buf->buf_per_size;

			SDL_LockAudio();
			sdl_audio_playback_refill(
				sad->playback_data,
				buf->buf_data + buf->buf_pos,
				buf->buf_per_size,
				buf->buf_per_samples[idx]);
			SDL_UnlockAudio();

			buf->buf_pos += buf->buf_per_size;

			sdl_audio_playback_pending(sad->playback_data);

			if (buf->buf_pos >= buf->buf_len)
				break;
		}
		SDL_PauseAudio(TRUE);

		/* signal: we need new data to playback */
		ReleaseSemaphore(player->hSemPlayperiodDone, 1, NULL);
	}

	player->ThreadSDLExit = 1;

	/* unblock thread, let ffm thread exit */
	ReleaseSemaphore(player->hSemPlayperiodDone, 1, NULL);
	ReleaseSemaphore(player->hSemBufferRecv, 1, NULL);

	/* signal: sdl thread exit */
	ReleaseSemaphore(player->hSemSDLExit, 1, NULL);

err_nodata:
	pr_console("%s: exit\n", __func__);

	ExitThread(0);
}

LRESULT PlaybackDaemonThread(LPVOID *data)
{
	PlayerData *player;
	int ret;

	pr_console("%s: init\n", __func__);

	player = (PlayerData *)data;
	if (!player)
		goto err;

	while (1) {
		ret = WaitForSingleObject(player->hSemPlaybackFinish, DAEMON_THREAD_INTERVAL_MS);

		if (ret == WAIT_TIMEOUT)
			if (player->playback_state == PLAYBACK_STOP)
				break;

		if (ret == WAIT_OBJECT_0) {
			/* stop and reset playback */
			player_playback_state_switch(player, PLAYBACK_STOP, INFINITE);

			break;
		}
	}

err:
	pr_console("%s: exit\n", __func__);

	ExitThread(0);
}



int player_thread_create(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	player_threadctx_init(player);

	player->hThreadFFM = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)FFMThread,
		(LPVOID)player,
		0,
		(LPDWORD)&player->ThreadFFMid);

	player->hThreadSDL = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)SDLThread,
		(LPVOID)player,
		0,
		(LPDWORD)&player->ThreadSDLid);

	player->hThreadDaemon = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)PlaybackDaemonThread,
		(LPVOID)player,
		0,
		(LPDWORD)&player->ThreadDaemonId);

	return 0;
}

int player_thread_destroy(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	switch (WaitForSingleObject(player->hSemFFMExit, 3000)) {
		case WAIT_OBJECT_0:
			pr_console("%s: receive ffm thread exit\n", __func__);
			break;

		case WAIT_TIMEOUT:
			pr_console("%s: terminating ffm thread!\n", __func__);
			TerminateThread(player->hThreadFFM, -ETIMEDOUT);
			break;

		default:
			MessageBoxErr(L"Exception occurred during stop\n");
			break;
	}

	switch (WaitForSingleObject(player->hSemSDLExit, 3000)) {
		case WAIT_OBJECT_0:
			pr_console("%s: receive sdl thread exit\n", __func__);
			break;

		case WAIT_TIMEOUT:
			pr_console("%s: terminating sdl thread!\n", __func__);
			TerminateThread(player->hThreadSDL, -ETIMEDOUT);
			break;

		default:
			MessageBoxErr(L"Exception occurred during stop\n");
			break;
	}

	/* TODO: if exception occurred, reload audio context */

	player_threadctx_deinit(player);

	return 0;
}