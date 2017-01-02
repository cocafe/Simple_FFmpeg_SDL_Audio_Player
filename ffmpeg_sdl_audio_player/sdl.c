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
 
#include "sdl_snd.h"

SDLPlaybackBuffer *sdl_playback_buffer_alloc(void)
{
	SDLPlaybackBuffer *buf;

	buf = (SDLPlaybackBuffer *)calloc(1, sizeof(SDLPlaybackBuffer));
	if (!buf)
		return NULL;

	return buf;
}

void sdl_playback_buffer_free(SDLPlaybackBuffer **buf)
{
	if (!*buf)
		return;

	free(*buf);

	*buf = NULL;
}

SDLPlaybackData *sdl_playback_data_alloc(void)
{
	SDLPlaybackData *data;

	data = (SDLPlaybackData *)calloc(1, sizeof(SDLPlaybackData));
	if (!data)
		return NULL;

	return data;
}

void sdl_playback_data_free(SDLPlaybackData **data)
{
	if (!*data)
		return;

	free(*data);

	*data = NULL;
}

SDL_AudioSpec *sdl_audio_spec_alloc(void)
{
	SDL_AudioSpec *spec;

	spec = (SDL_AudioSpec *)calloc(1, sizeof(SDL_AudioSpec));
	if (!spec)
		return NULL;

	return spec;
}

void sdl_audio_spec_free(SDL_AudioSpec **spec)
{
	if (!*spec)
		return;

	free(*spec);

	*spec = NULL;
}

SDLAudioData *sdl_audio_data_alloc(void)
{
	SDLAudioData *sad;

	sad = (SDLAudioData *)calloc(1, sizeof(SDLAudioData));
	if (!sad)
		return NULL;

	return sad;
}

void sdl_audio_data_free(SDLAudioData **sad)
{
	if (!sad)
		return;

	free(*sad);

	*sad = NULL;
}

void SDLCALL sdl_audio_callback(void *userdata, uint8_t *stream, int32_t len)
{
	SDLPlaybackData *pbdata;
	int32_t len_play;

	pbdata = (SDLPlaybackData *)userdata;

	len_play = pbdata->buf->chunk_size - pbdata->buf->pos;
	if (len_play > len)
		len_play = len;

	SDL_memset(stream, 0x00, len_play);

	SDL_MixAudio(stream, pbdata->buf->chunk + pbdata->buf->pos, len_play, pbdata->volume);
	pbdata->buf->pos += len_play;

	if (pbdata->buf->pos >= pbdata->buf->chunk_size)
		ReleaseSemaphore(pbdata->hSemSDLPlaybackPending, 1, NULL);
}

int sdl_audio_data_init(
	SDLAudioData *sad,
	uint16_t sdl_sample_rate,
	SDL_AudioFormat sdl_sample_fmt,
	uint16_t sdl_nb_samples,
	uint8_t sdl_channels)
{
	if (!sad)
		return -EINVAL;

	if (sdl_channels > 2 || sdl_channels < 1)
		return -EINVAL;

	sad->playback_data = sdl_playback_data_alloc();
	if (!sad->playback_data)
		return -ENOMEM;

	sad->playback_data->buf = sdl_playback_buffer_alloc();
	if (!sad->playback_data->buf)
		return -ENOMEM;

	sad->spec_ret = sdl_audio_spec_alloc();
	if (!sad->spec_ret)
		return -ENOMEM;

	sad->spec = sdl_audio_spec_alloc();
	if (!sad->spec)
		return -ENOMEM;

	sad->spec->freq = sdl_sample_rate;
	sad->spec->format = sdl_sample_fmt;
	sad->spec->samples = sdl_nb_samples;
	sad->spec->channels = sdl_channels;
	sad->spec->callback = sdl_audio_callback;
	sad->spec->padding = 0;
	sad->spec->silence = 0;
	sad->spec->userdata = (void *)sad->playback_data;

	sad->playback_data->hSemSDLPlaybackPending = CreateSemaphore(NULL, 0, 1, NULL);

	if (SDL_OpenAudio(sad->spec, sad->spec_ret)) {
		return -ENODEV;
	}

	memcpy(sad->spec, sad->spec_ret, sizeof(SDL_AudioSpec));

	return 0;
}

void sdl_audio_data_exit(SDLAudioData *sad)
{
	if (!sad)
		return;

	SDL_PauseAudio(TRUE);

	CloseHandle(sad->playback_data->hSemSDLPlaybackPending);

	sdl_playback_buffer_free(&sad->playback_data->buf);
	sdl_playback_data_free(&sad->playback_data);
	sdl_audio_spec_free(&sad->spec_ret);
	sdl_audio_spec_free(&sad->spec);

	SDL_CloseAudio();
	SDL_Quit();
}

int sdl_audio_playback_init(
	SDLPlaybackData *pbdata,
	int32_t volume_def)
{
	if (!pbdata)
		return -EINVAL;

	pbdata->buf->pos = 0;
	pbdata->buf->chunk = NULL;
	pbdata->buf->chunk_size = 0;
	pbdata->buf->nb_samples = 0;
	pbdata->buf->refill_pending = FALSE;

	pbdata->volume = volume_def;

	return 0;
}

int sdl_audio_playback_refill(SDLPlaybackData *pbdata, uint8_t *chunk_ref, int32_t chunk_size, int32_t nb_samples) 
{
	if (!pbdata)
		return -EINVAL;

	/* finish last chunk playback, 
	 * position will be reset */
	pbdata->buf->pos = 0;
	pbdata->buf->chunk = chunk_ref;
	pbdata->buf->chunk_size = chunk_size;
	pbdata->buf->nb_samples = nb_samples;
	pbdata->buf->refill_pending = TRUE;

	return 0;
}

int sdl_audio_playback_pending(SDLPlaybackData *pbdata)
{
	if (!pbdata)
		return -EINVAL;

	WaitForSingleObject(pbdata->hSemSDLPlaybackPending, INFINITE);

	return 0;
}