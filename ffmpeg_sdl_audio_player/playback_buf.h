/**
 * Header file for internal playback buffer
 *
 *		Author: cocafe <cocafehj@gmail.com>
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