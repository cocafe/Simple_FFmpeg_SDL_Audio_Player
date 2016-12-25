#pragma once

#include "stdafx.h"

typedef struct PlaybackBuf {
	uint8_t							*buf_data;
	int32_t							buf_pos;
	int32_t							buf_len;
	int32_t							buf_size;
} PlaybackBuf;

PlaybackBuf *playback_buf_alloc(void);
void playback_buf_free(PlaybackBuf **buf);

int playback_buf_init(PlaybackBuf *buf, int32_t buf_size);
void playback_buf_deinit(PlaybackBuf *buf);

int playback_buf_write(PlaybackBuf *buf, uint8_t *data, int32_t size);
int playback_buf_copy(PlaybackBuf *dst, PlaybackBuf *src, int32_t size);

int playback_buf_is_full(PlaybackBuf *buf);
int playback_buf_try_full(PlaybackBuf *buf, int32_t write_size);

int pcm_buf_len_set(PlaybackBuf *buf, int32_t len);
int pcm_buf_pos_get(PlaybackBuf *buf);
int playback_buf_flush(PlaybackBuf *buf);