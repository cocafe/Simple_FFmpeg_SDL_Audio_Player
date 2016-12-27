#pragma once

#include "stdafx.h"

#include "ffmpeg.h"
#include "sdl_snd.h"
#include "playback_buf.h"

#define VOLUME_VALUE_DEFAULT				(AUDIO_VOLUME_MAX)
#define VOLUME_STEP_DEFAULT					(8)
#define VOLUME_ENABLE_OVERLOUD				(TRUE)

#define SEEK_INTERVAL_DELAY_MS				(200)
#define SEEK_STEP_DEFAULT					(1000)

typedef enum PlaybackSeekDirection {
	SEEK_FORWARD = 1,
	SEEK_BACKWARD = -1
} PlaybackSeekDirection;

typedef enum PlaybackState {
	PLAYBACK_PLAY = 0,
	PLAYBACK_PAUSE,
	PLAYBACK_STOP,
	PLAYBACK_DONE,					/* not used yet */

	NUM_PLAYBACK_STATUS,
} PlaybackState;

typedef struct PlayerData {
	int32_t							fileload;

	/* playback device */
	SDLAudioData					*sad;
	AVDataCtx						*avd;

	/* playback buffer */
	PlaybackBuf						*buf_decode;
	PlaybackBuf						*buf_playback;

	/* playback thread */
	HANDLE							hThreadSDL;
	HANDLE							hThreadFFM;
	HANDLE							hThreadDaemon;
#define DAEMON_THREAD_INTERVAL_MS	(200)

	WORD							ThreadSDLid;
	WORD							ThreadFFMid;
	WORD							ThreadDaemonId;

	int32_t							ThreadSDLExit;
	int32_t							ThreadFFMExit;

	/* playback thread synchronization */
	HANDLE							hSemPlaybackFinish;
	HANDLE							hSemPlayperiodDone;
	HANDLE							hSemBufferSend;
	HANDLE							hSemBufferRecv;
	HANDLE							hSemSDLExit;
	HANDLE							hSemFFMExit;

	/* playback control */
	HANDLE							hSemPlaybackSDLStart;
	HANDLE							hSemPlaybackSDLPause;
	HANDLE							hSemPlaybackSDLResume;
	HANDLE							hSemPlaybackSDLPreSeek;
	HANDLE							hSemPlaybackSDLPostSeek;

	HANDLE							hSemPlaybackFFMStart;
	HANDLE							hSemPlaybackFFMPause;
	HANDLE							hSemPlaybackFFMResume;
	HANDLE							hSemPlaybackFFMPreSeek;
	HANDLE							hSemPlaybackFFMPostSeek;

	HANDLE							hMutexPlaybackSwitch;
	HANDLE							hMutexPlaybackSeek;

	/* playback volume control */
	int32_t							volume_def;
	int32_t							volume_min;
	int32_t							volume_max;
	int32_t							volume_mute;
	int32_t							volume_step;
	int32_t							volume_over;
	int32_t							*volume_sdl;

	/* playback position control: stream time_base */
	int64_t							pos_min;
	int64_t							pos_cur;
	int64_t							pos_max;

	int64_t							seekable;
	int64_t							seeking;
	int64_t							seek_step;

	PlaybackState					playback_state;
} PlayerData;

extern PlayerData *gPlayerIns;

#define InitPlayerData				player_data_init
#define DeinitPlayerData			player_data_deinit

#define AllocPlayerData				player_data_alloc
#define FreePlayerData				player_data_free

#define InitPlayerAudio				player_audio_init
#define DeinitPlayerAudio			player_audio_deinit

#define RegisterPlayerData			player_data_register

#define GetPlaybackState			player_playback_state_get
#define SwitchPlaybackState			player_playback_state_switch

#define SeekPlayerDirection			player_seek_direction
#define SeekPlayerTimestamp			player_seek_timestamp

#define GetPlayerVolume				player_vol_get
#define SetPlayerVolume				player_vol_set
#define SetPlayerVolumeByStep		player_vol_set_by_step

#define GetPlayerVolumeMute			player_vol_mute_get
#define TogglePlayerVolumeMute		player_vol_mute_toggle

#define GetAudiofileBitrate			player_file_bitrate_get
#define GetAudiofileSampleRate		player_file_sample_rate_get
#define GetAudiofileDuration		player_duration_get
#define GtePlayerStreamDuration		player_stream_duration
#define GetPlayerCurrentTimestamp	player_timestamp_get

int player_data_init(PlayerData *player);
int player_data_deinit(PlayerData *player);

int player_audio_init(
	PlayerData *player,
	char *filepath,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);

int player_audio_deinit(PlayerData *player);

PlayerData *player_data_alloc(void);
void player_data_free(PlayerData **data);

int player_data_register(PlayerData *player);

PlaybackState player_playback_state_get(PlayerData *player);
int player_playback_state_switch(PlayerData *player, PlaybackState state);

int player_seek_direction(
	PlayerData *player,
	PlaybackSeekDirection dir);

int player_seek_timestamp(
	PlayerData *player,
	int64_t timestamp,
	int flags);

int player_vol_get(PlayerData *player);
int player_vol_set(PlayerData *player, int target_vol);
int player_vol_set_by_step(PlayerData *player, int steps);

int player_vol_mute_get(PlayerData *player);
int player_vol_mute_toggle(PlayerData *player);

int64_t player_file_bitrate_get(PlayerData *player);
int player_file_sample_rate_get(PlayerData *player);
int64_t player_duration_get(PlayerData *player);
int64_t player_stream_duration(PlayerData *player);
int64_t player_timestamp_get(PlayerData *player);