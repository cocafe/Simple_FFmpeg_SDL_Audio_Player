/**
 * Player Related Functions
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */
 
#include "player.h"
#include "player_thread.h"
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
	player->seeking = 0;
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

int player_buffer_flush(PlayerData *player)
{
	if (!player)
		return -EINVAL;

	playback_buf_flush(player->buf_decode);
	playback_buf_flush(player->buf_playback);

	return 0;
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
	ReleaseSemaphore(player->hSemPlaybackSDLPause, 1, NULL);

	/* avoid trigging block thread condition */
	player->playback_state = PLAYBACK_PLAY;

	/* signal: state changed */
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
			player_buffer_flush(player);
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

int player_playback_state_switch(PlayerData *player, PlaybackState state, int timeout_ms)
{
	int ret = 0;

	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -EINVAL;

	if (WaitForSingleObject(player->hMutexPlaybackSwitch, timeout_ms) == WAIT_TIMEOUT) {
		pr_console("%s: busy\n", __func__);
		return -ETIMEDOUT;
	}

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

int player_seek_timestamp(
	PlayerData *player,
	int64_t timestamp,
	int flags)
{
	int last_paused = 0;
	int ret;

	if (!player)
		return -EINVAL;

	if (!player->fileload)
		return -ENODATA;

	if (!player->seekable)
		return -EINVAL;

	if (player->seeking)
		return -EBUSY;

	if (timestamp < player->pos_min)
		timestamp = player->pos_min;

	if (timestamp > player->pos_max)
		timestamp = player->pos_max;

	/* if busy, return immediately, or UI will freeze */
	if (WaitForSingleObject(player->hMutexPlaybackSeek, 10) != WAIT_OBJECT_0) {
		pr_console("%s: lock failed: busy\n", __func__);
		return -EBUSY;
	}

	/* not allowed to change playback state during seeking */
	if (WaitForSingleObject(player->hMutexPlaybackSwitch, 10) != WAIT_OBJECT_0)
		goto seek_unlock;

	player->seeking = 1;

	if (player->playback_state == PLAYBACK_PAUSE) {
		/* call player_playback_resume() directly */
		player_playback_resume(player);

		last_paused = 1;
	}

	if (player->playback_state == PLAYBACK_PLAY) {
		/* wait: block sdl thread, sync with ffm thread */
		while (WaitForSingleObject(player->hSemPlaybackSDLPreSeek, 100) == WAIT_TIMEOUT) {
			if (player->ThreadSDLExit)
				goto switch_unlock;
		}

		while (WaitForSingleObject(player->hSemPlaybackFFMPreSeek, 100) == WAIT_TIMEOUT) {
			if (player->ThreadFFMExit)
				goto switch_unlock;
		}
	}

	ret = av_seek_frame(player->avd->format_ctx, player->avd->stream_idx, timestamp, flags);

	/* WORKAROUND: av_read_frame() needs some time to sync up context */
	Sleep(SEEK_INTERVAL_DELAY_MS);

	/* seek frame succeed */
	if (ret >= 0)
		player->pos_cur = timestamp;

	/* clear ffmpeg decoded buffer */
	playback_buf_flush(player->buf_decode);

	if (player->playback_state == PLAYBACK_PLAY) {
		/* signal: unblock sdl playback thread */
		ReleaseSemaphore(player->hSemPlaybackSDLPostSeek, 1, NULL);
	}

	if (last_paused)
		player_playback_pause(player);

switch_unlock:
	player->seeking = 0;
	ReleaseMutex(player->hMutexPlaybackSwitch);

seek_unlock:
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

