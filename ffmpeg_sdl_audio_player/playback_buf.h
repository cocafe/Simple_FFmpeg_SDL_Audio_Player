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

#include "stdafx.h"

/*
 *   frame    frame    frame
 * ----------------------------
 * |        |        |        |
 * |  data  |  data  |  data  |
 * |        |        |        |
 * ----------------------------
 *          ^
 *          |
 *         pos
 *
 */
typedef struct PlaybackBuf {
	uint8_t							*buf_data;
	int32_t							buf_pos;
	int32_t							buf_len;		/* valid size */
	int32_t							buf_size;		/* buffer size */

	int32_t							buf_per_size;
	int32_t							buf_per_count;
	int32_t							*buf_per_samples;	/* nb_samples of per frame */
} PlaybackBuf;

PlaybackBuf *playback_buf_alloc(void);
void playback_buf_free(PlaybackBuf **buf);

int playback_buf_init(PlaybackBuf *buf, int32_t per_size, int32_t multiple);
void playback_buf_deinit(PlaybackBuf *buf);

int playback_buf_write(PlaybackBuf *buf, uint8_t *data, int32_t size, int32_t nb_samples);
int playback_buf_copy(PlaybackBuf *dst, PlaybackBuf *src, int32_t size);

int playback_buf_is_full(PlaybackBuf *buf);
int playback_buf_try_full(PlaybackBuf *buf, int32_t write_size);

int playback_buf_len_set(PlaybackBuf *buf, int32_t len);

int playback_buf_pos_reset(PlaybackBuf *buf);
int playback_buf_pos_get(PlaybackBuf *buf);

int playback_buf_flush(PlaybackBuf *buf);