#include "player.h"
#include "wnd.h"

const uint16_t avfmt_2_sdl_fmt[] = {
	[AV_SAMPLE_FMT_U8] = AUDIO_U8,
	[AV_SAMPLE_FMT_S16] = AUDIO_S16SYS,
	[AV_SAMPLE_FMT_S32] = AUDIO_S32SYS,
	[AV_SAMPLE_FMT_FLT] = AUDIO_S32SYS,
	[AV_SAMPLE_FMT_S16P] = AUDIO_S16SYS,
};

PlayerData *gPlayerIns;

void player_mutex_lock_init(PlayerData *player) 
{
	player->hMutexPlaybackSwitch = CreateMutex(NULL, FALSE, NULL);
	player->hMutexPlaybackSeek = CreateMutex(NULL, FALSE, NULL);
}

void player_mutex_lock_deinit(PlayerData *player)
{
	CloseHandle(player->hMutexPlaybackSwitch);
	CloseHandle(player->hMutexPlaybackSeek);
}

void player_threadctx_init(PlayerData *player)
{
	player->hSemPlayperiodDone = CreateSemaphore(NULL, 1, 1, NULL);
	player->hSemPlaybackFinish = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemPlaybackFFMStart = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackFFMPause = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackFFMResume = CreateSemaphore(NULL, 0, 1, NULL);

	player->hSemPlaybackSDLStart = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLPause = CreateSemaphore(NULL, 0, 1, NULL);
	player->hSemPlaybackSDLResume = CreateSemaphore(NULL, 0, 1, NULL);

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
	CloseHandle(player->hSemPlaybackFFMPause);
	CloseHandle(player->hSemPlaybackFFMResume);
	CloseHandle(player->hSemPlaybackSDLStart);
	CloseHandle(player->hSemPlaybackSDLPause);
	CloseHandle(player->hSemPlaybackSDLResume);
	CloseHandle(player->hSemPlayperiodDone);
	CloseHandle(player->hSemPlaybackFinish);
	CloseHandle(player->hSemBufferSend);
	CloseHandle(player->hSemBufferRecv);
	CloseHandle(player->hSemFFMExit);
	CloseHandle(player->hSemSDLExit);

	player->ThreadFFMExit = 1;
	player->ThreadSDLExit = 1;
}

int player_volume_init(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	player->volume_min = AUDIO_VOLUME_MIN;
	player->volume_max = AUDIO_VOLUME_MAX;
	player->volume_def = AUDIO_VOLUME_MAX;
	player->volume_step = VOLUME_STEP_DEFAULT;
	player->volume_mute = 0;
	player->volume_over = 1;

	return 0;
}

int player_volume_save(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	/* save last volume */
	player->volume_def = *player->volume_sdl;

	return 0;
}

int player_volume_reload(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (!player->sad)
		return -EINVAL;

	if (!player->sad->playback_data)
		return -EINVAL;

	player->volume_sdl = &player->sad->playback_data->volume;

	return 0;
}

int player_volume_apply(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	*player->volume_sdl = player->volume_def;

	return 0;
}

PlayerData *player_data_alloc(void)
{
	PlayerData *data;

	data = (PlayerData *)calloc(1, sizeof(PlayerData));
	if (!data)
		return NULL;

	return data;
}

void player_data_free(PlayerData **data)
{
	if (!*data)
		return;

	free(*data);

	*data = NULL;
}

int player_data_init(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	memset(player, 0x00, sizeof(PlayerData));

	player->avd = avdata_alloc();
	if (!player->avd)
		return -ENOMEM;

	player->sad = sdl_audio_data_alloc();
	if (!player->sad)
		return -ENOMEM;

	player_volume_init(player);
	player_mutex_lock_init(player);

	/* those values are 1 by default */
	player->ThreadFFMExit = 1;
	player->ThreadSDLExit = 1;

	return 0;
}

int player_data_deinit(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	avdata_free(&player->avd);
	sdl_audio_data_free(&player->sad);
	player_mutex_lock_deinit(player);

	return 0;
}


int player_data_register(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (gPlayerIns)
		return -EEXIST;

	gPlayerIns = player;

	return 0;
}

int player_pos_init(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	player->pos_min = 0;
	player->pos_max = player->avd->format_ctx->streams[player->avd->stream_idx]->duration;
	player->pos_cur = player->pos_min;
	player->seekable = player->avd->format_ctx->pb->seekable;
	player->seek_step = player->avd->swr->dst_nb_samples * 100;

	return 0;
}

int player_audio_init(
	PlayerData *player,
	char *filepath,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	if (!player)
		return -EINVAL;

	if (player->fileload)
		return -EEXIST;

	if (avdata_init(player->avd)) {
		return -ENODEV;
	}

	if (avdata_open_file(player->avd, filepath)) {
		return -ENODATA;
	}

	if (avswr_init(
		player->avd->swr,
		player->avd,
		target_chnl_layout,
		target_sample_rate,
		target_sample_fmt)) {
		return -ENODEV;
	}

	if (sdl_audio_data_init(player->sad,
		target_sample_rate,
		avfmt_2_sdl_fmt[target_sample_fmt],
		(int32_t)player->avd->swr->dst_nb_samples_max,
		av_get_channel_layout_nb_channels(player->avd->swr->dst_chnl_layout))) {
		return -ENODATA;
	}

	sdl_audio_playback_init(
		player->sad->playback_data,
		player->volume_def);

	player->buf_decode = playback_buf_alloc();
	if (!player->buf_decode)
		return -ENOMEM;

	player->buf_playback = playback_buf_alloc();
	if (!player->buf_playback)
		return -ENOMEM;

	playback_buf_init(player->buf_decode, player->avd->swr->dst_linesize, 4);
	playback_buf_init(player->buf_playback, player->avd->swr->dst_linesize, 4);

	player_pos_init(player);

	player->playback_state = PLAYBACK_STOP;

	player_volume_reload(player);
	player_volume_apply(player);

	player->fileload = TRUE;

	return 0;
}

int player_audio_deinit(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	player->fileload = FALSE;

	player->playback_state = PLAYBACK_STOP;

	player_volume_save(player);

	playback_buf_deinit(player->buf_decode);
	playback_buf_deinit(player->buf_playback);

	playback_buf_free(&player->buf_decode);
	playback_buf_free(&player->buf_playback);

	sdl_audio_data_exit(player->sad);
	avdata_exit(player->avd);

	return 0;
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
		if (player->playback_state == PLAYBACK_PAUSE) {
			/* wait: block thread */
			WaitForSingleObject(player->hSemPlaybackFFMPause, INFINITE);

			/* wait: block thread until state changed */
			WaitForSingleObject(player->hSemPlaybackFFMResume, INFINITE);
		}

		if (player->playback_state == PLAYBACK_STOP) {
			break;
		}

		ret = avcodec_decode_file(avd);
		if (ret < 0) {
			pr_console("%s: error occurred or eof (%#04x)\n", __func__, ret);
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

		/* buffer is full or eof reached but we still have buf not flushed  */
		if (playback_buf_try_full(buf, avd->swr->dst_bufsize) ||
			(avd->format_ctx->pb->eof_reached && playback_buf_pos_get(buf))) {

			/* wait: data playback done, wait to fill data init: 1 */
			if (WaitForSingleObject(player->hSemPlayperiodDone, INFINITE) == WAIT_OBJECT_0) {
				pr_console("%s: received play done\n", __func__);
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
		if (player->playback_state == PLAYBACK_PAUSE) {
			SDL_PauseAudio(TRUE);

			/* wait: block thread */
			WaitForSingleObject(player->hSemPlaybackSDLPause, INFINITE);
			/* wait: block thread until state changed */
			WaitForSingleObject(player->hSemPlaybackSDLResume, INFINITE);

			SDL_PauseAudio(FALSE);
		}

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
			player_playback_state_switch(player, PLAYBACK_STOP);

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

	/* TODO: Timeout: TerminateThread() */

	if (WaitForSingleObject(player->hSemFFMExit, INFINITE) == WAIT_OBJECT_0) {
		pr_console("%s: receive ffm thread exit\n", __func__);
	}

	if (WaitForSingleObject(player->hSemSDLExit, INFINITE) == WAIT_OBJECT_0) {
		pr_console("%s: receive sdl thread exit\n", __func__);
	}

	CloseHandle(player->hThreadFFM);
	CloseHandle(player->hThreadSDL);

	player->hThreadFFM = NULL;
	player->hThreadSDL = NULL;

	player->ThreadFFMid = 0;
	player->ThreadSDLid = 0;

	/* if use TerminateThread() make sure variable changed */
	player_threadctx_deinit(player);

	return 0;
}

int player_seek_timestamp(
	PlayerData *player, 
	int64_t timestamp,
	int flags) 
{
	int paused = 0;
	int ret;

	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	if (!player->seekable)
		return -EINVAL;

	if (timestamp < player->pos_min)
		timestamp = player->pos_min;

	if (timestamp > player->pos_max)
		timestamp = player->pos_max;

	/* if busy, return immediately, or UI will freeze */
	if (WaitForSingleObject(player->hMutexPlaybackSeek, 10) != WAIT_OBJECT_0) {
		pr_console("%s: busy\n", __func__);
		return -EBUSY;
	}

	if (player->playback_state == PLAYBACK_PLAY) {
		paused = 1;
		if (player_playback_state_switch(player, PLAYBACK_PAUSE) == -ETIMEDOUT)
			goto unlock;
	}

	/* WORKAROUND: we need some time to let av_read_frame() sync up */
	Sleep(SEEK_INTERVAL_DELAY_MS);

	ret = av_seek_frame(player->avd->format_ctx, player->avd->stream_idx, timestamp, flags);

	/* seek frame succeed */
	if (ret >= 0)
		player->pos_cur = timestamp;

	/* clear ffmpeg decoded buffer */
	playback_buf_flush(player->buf_decode);

	/* resume */
	if (paused)
		player_playback_state_switch(player, PLAYBACK_PLAY);

unlock:
	ReleaseMutex(player->hMutexPlaybackSeek);

	return 0;
}

int player_seek_direction(
	PlayerData *player,
	PlaybackSeekDirection dir)
{
	return player_seek_timestamp(
		player,
		player->pos_cur + (player->seek_step * dir),
		AVSEEK_FLAG_ANY);
}

int player_playback_resume(PlayerData *player) {
	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	if (player->ThreadFFMExit) {
		pr_console("%s: ffm thread is not running\n", __func__);
		return -EEXIST;
	}

	if (player->ThreadSDLExit) {
		pr_console("%s: sdl thread is not running\n", __func__);
		return -EEXIST;
	}

	/* signal: unblock playback threads */
	ReleaseSemaphore(player->hSemPlaybackFFMPause, 1, NULL);
	ReleaseSemaphore(player->hSemPlaybackSDLPause, 1, NULL);

	/* avoid trigging block thread condition */
	player->playback_state = PLAYBACK_PLAY;

	/* signal: state changed */
	ReleaseSemaphore(player->hSemPlaybackFFMResume, 1, NULL);
	ReleaseSemaphore(player->hSemPlaybackSDLResume, 1, NULL);

	return 0;
}

int player_playback_stop(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	switch (player->playback_state)
	{
		/* pause --> resume (--> play --> stop) */
		case PLAYBACK_PAUSE:
			player_playback_resume(player);

		/* play --> stop */
		case PLAYBACK_PLAY:
			player->playback_state = PLAYBACK_STOP;

			player_thread_destroy(player);
			player_seek_timestamp(player, 0, AVSEEK_FLAG_ANY);

			break;

		case PLAYBACK_STOP:
		case PLAYBACK_DONE:
		default:
			break;
	}

	return 0;
}

int player_playback_play(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	/* do not change playback_sate ahead */
	switch (player->playback_state) {
		/* stop --> play */
		case PLAYBACK_STOP:
			if (!player->ThreadFFMExit) {
				pr_console("%s: ffm thread is running\n", __func__);
				return -EEXIST;
			}

			if (!player->ThreadSDLExit) {
				pr_console("%s: sdl thread is running\n", __func__);
				return -EEXIST;
			}

			player->playback_state = PLAYBACK_PLAY;
			player_thread_create(player);

			break;

		/* pause --> resume */
		case PLAYBACK_PAUSE:
			player_playback_resume(player);
			break;

		case PLAYBACK_PLAY:
		case PLAYBACK_DONE:
		default:
			break;
	}

	return 0;
}

int player_playback_pause(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	switch (player->playback_state) {
		case PLAYBACK_PLAY:
			player->playback_state = PLAYBACK_PAUSE;
			break;

		case PLAYBACK_STOP:
		case PLAYBACK_PAUSE:
		case PLAYBACK_DONE:
		default:
			break;
	}

	return 0;
}

int player_playback_state_switch(PlayerData *player, PlaybackState state)
{
	int ret = 0;

	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -EINVAL;

	if (WaitForSingleObject(player->hMutexPlaybackSwitch, 200) == WAIT_TIMEOUT)
		return -ETIMEDOUT;

	if (player->playback_state != state) {
		switch (state) {
			case PLAYBACK_STOP:
				ret = player_playback_stop(player);
				break;

			case PLAYBACK_PLAY:
				ret = player_playback_play(player);
				break;

			case PLAYBACK_PAUSE:
				ret = player_playback_pause(player);
				break;

			default:
				break;
		}
	}

	ReleaseMutex(player->hMutexPlaybackSwitch);

	NotifyUpdateMainStatusBar();

	return ret;
}

PlaybackState player_playback_state_get(PlayerData *player)
{
	return player->playback_state;
}

int player_vol_mute_get(PlayerData *player)
{
	return player->volume_mute ? TRUE : FALSE;
}

int player_vol_mute_toggle(PlayerData *player)
{
	int t;

	if (!player)
		return -EINVAL;

	if (!player->sad)
		return -EINVAL;

	if (!player->sad->playback_data)
		return -EINVAL;

	t = player->volume_mute;
	player->volume_mute = *player->volume_sdl;
	*player->volume_sdl = t;

	return 0;
}

int player_vol_get(PlayerData *player)
{
	return *player->volume_sdl;
}

int player_vol_set(PlayerData *player, int target_vol)
{
	int32_t min;
	int32_t max;

	if (!player)
		return -EINVAL;

	if (!player->sad)
		return -EINVAL;

	if (!player->sad->playback_data)
		return -EINVAL;

	if (player->volume_mute)
		return -EACCES;

	min = player->volume_min;
	max = player->volume_over ? AUDIO_VOLUME_OVER : AUDIO_VOLUME_MAX;

	if (target_vol < min)
		target_vol = min;
	else if (target_vol > max)
		target_vol = max;

	*player->volume_sdl = target_vol;

	return 0;
}

int player_vol_set_by_step(PlayerData *player, int steps)
{
	if (!player)
		return -EINVAL;

	if (!player->sad)
		return -EINVAL;

	if (!player->sad->playback_data)
		return -EINVAL;

	player_vol_set(player, player_vol_get(player) + steps * player->volume_step);

	return 0;
}

int64_t player_file_bitrate_get(PlayerData *player)
{
	return player->avd->format_ctx->bit_rate;
}

int player_file_sample_rate_get(PlayerData *player)
{
	return player->avd->codec_ctx->sample_rate;
}

int64_t player_duration_get(PlayerData *player)
{
	return player->avd->format_ctx->duration;
}

int64_t player_stream_duration(PlayerData *player)
{
	return player->avd->format_ctx->streams[player->avd->stream_idx]->duration;
}

int64_t player_timestamp_get(PlayerData *player)
{
	return player->pos_cur;
}