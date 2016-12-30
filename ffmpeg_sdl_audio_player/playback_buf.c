/**
 * Internal Audio Buffer
 *
 *		Author: cocafe <cocafehj@gmail.com>
 */
 
#include "playback_buf.h"

PlaybackBuf *playback_buf_alloc(void)
{
	PlaybackBuf *buf;

	buf = (PlaybackBuf *)calloc(1, sizeof(PlaybackBuf));
	if (!buf)
		return NULL;

	return buf;
}

void playback_buf_free(PlaybackBuf **buf)
{
	if (!*buf)
		return;

	free(*buf);

	*buf = NULL;
}

int playback_buf_init(PlaybackBuf *buf, int32_t per_size, int32_t multiple)
{
	if (!buf)
		return -EINVAL;

	buf->buf_pos = 0;
	buf->buf_len = 0;
	buf->buf_size = per_size * multiple;
	buf->buf_data = (uint8_t *)calloc(1, buf->buf_size);

	buf->buf_per_size = per_size;
	buf->buf_per_count = multiple;
	buf->buf_per_samples = (int32_t *)calloc(multiple, sizeof(int32_t));

	return 0;
}

void playback_buf_deinit(PlaybackBuf *buf)
{
	if (!buf)
		return;

	free(buf->buf_data);
	free(buf->buf_per_samples);
}

int playback_buf_write(PlaybackBuf *buf, uint8_t *data, int32_t size, int32_t nb_samples)
{
	int idx;

	if (!buf)
		return -EINVAL;

	if (size != buf->buf_per_size)
		return -EINVAL;

	/* store nb_samples */
	idx = buf->buf_pos / buf->buf_per_size;
	buf->buf_per_samples[idx] = nb_samples;

	memcpy(buf->buf_data + buf->buf_pos, data, size);

	/* move to next frame buffer */
	buf->buf_pos += size;
	/* mark valid length */
	buf->buf_len = buf->buf_pos;

	return 0;
}

int playback_buf_copy(PlaybackBuf *dst, PlaybackBuf *src, int32_t size)
{
	if (!dst || !src)
		return -EINVAL;

	if (size > dst->buf_size)
		return -EINVAL;

	if (size > src->buf_size)
		return -EINVAL;

	memcpy(dst->buf_per_samples, src->buf_per_samples, src->buf_per_count * sizeof(int32_t));

	memcpy(dst->buf_data, src->buf_data, size);
	dst->buf_len = size;

	return 0;
}

int playback_buf_is_full(PlaybackBuf *buf)
{
	return (buf->buf_pos >= buf->buf_size);
}

int playback_buf_try_full(PlaybackBuf *buf, int32_t write_size)
{
	return (buf->buf_pos + write_size > buf->buf_size);
}

int playback_buf_len_set(PlaybackBuf *buf, int32_t len)
{
	if (!buf)
		return -EINVAL;

	buf->buf_len = len;

	return 0;
}

int playback_buf_pos_reset(PlaybackBuf *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buf_pos = 0;

	return 0;
}

int playback_buf_pos_get(PlaybackBuf *buf)
{
	return buf->buf_pos;
}

int playback_buf_flush(PlaybackBuf *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buf_len = 0;
	buf->buf_pos = 0;
	memset(buf->buf_data, 0x00, buf->buf_size);
	memset(buf->buf_per_samples, 0x00, buf->buf_per_count * sizeof(int32_t));

	return 0;
}