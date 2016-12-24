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

int playback_buf_init(PlaybackBuf *buf, int32_t buf_size)
{
	if (!buf)
		return -EINVAL;

	buf->buf_pos = 0;
	buf->buf_len = 0;
	buf->buf_size = buf_size;
	buf->buf_data = (uint8_t *)calloc(1, buf->buf_size);

	return 0;
}

void playback_buf_deinit(PlaybackBuf *buf)
{
	if (!buf)
		return;

	free(buf->buf_data);
}

int playback_buf_write(PlaybackBuf *buf, uint8_t *data, int32_t size)
{
	memcpy(buf->buf_data + buf->buf_pos, data, size);
	buf->buf_pos += size;
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

int pcm_buf_len_set(PlaybackBuf *buf, int32_t len)
{
	if (!buf)
		return -EINVAL;

	buf->buf_len = len;

	return 0;
}

int pcm_buf_pos_reset(PlaybackBuf *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buf_pos = 0;

	return 0;
}

int pcm_buf_pos_get(PlaybackBuf *buf)
{
	return buf->buf_pos;
}

int playback_buf_flush(PlaybackBuf *buf)
{
	if (!buf)
		return -EINVAL;

	buf->buf_len = 1;
	buf->buf_pos = 0;
	memset(buf->buf_data, 0x00, buf->buf_size);

	return 0;
}