#pragma once

#include "stdafx.h"

typedef struct PlaybackBuf {
	uint8_t							*buf_data;
	int32_t							buf_pos;
	int32_t							buf_len;
	int32_t							buf_size;
} PlaybackBuf;