#pragma once

#include <libavutil\opt.h>
#include <libavutil\samplefmt.h>
#include <libavutil\channel_layout.h>
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswresample\swresample.h>

/* Fix typedef in avformat.h header */
typedef enum AVSampleFormat AVSampleFormat;

typedef enum AVSampleRate
{
	AV_SAMPLE_RATE_6KHZ = 6000,
	AV_SAMPLE_RATE_8KHZ = 8000,
	AV_SAMPLE_RATE_11KHZ = 11025,
	AV_SAMPLE_RATE_16KHZ = 16000,
	AV_SAMPLE_RATE_22KHZ = 22050,
	AV_SAMPLE_RATE_32KHZ = 32000,
	AV_SAMPLE_RATE_44_1KHZ = 44100,
	AV_SAMPLE_RATE_48KHZ = 48000,
	AV_SAMPLE_RATE_64KHZ = 64000,
	AV_SAMPLE_RATE_88_2KHZ = 88200,
	AV_SAMPLE_RATE_96KHZ = 96000,
	AV_SAMPLE_RATE_176_4KHZ = 176400,
	AV_SAMPLE_RATE_192KHZ = 192000,
} AVSampleRate;

typedef struct AVSwrCtx {
	SwrContext							*swr_ctx;

	int32_t								src_sample_rate;
	int32_t								dst_sample_rate;

	AVSampleFormat						src_sample_fmt;
	AVSampleFormat						dst_sample_fmt;

	int32_t								src_nb_chnl;
	int32_t								dst_nb_chnl;

	int64_t								src_chnl_layout;
	int64_t								dst_chnl_layout;

	int64_t								src_nb_samples;
	int64_t								dst_nb_samples;
	int64_t								dst_nb_samples_max;

	uint8_t								**dst_data;
	int32_t								dst_bufsize;	/* length of dst_data in one frame */
	int32_t								dst_linesize;
} AVSwrCtx;

typedef struct AVDataCtx {
	AVFormatContext						*format_ctx;
	AVCodecContext						*codec_ctx;
	AVCodec								*codec;

	AVSwrCtx							*swr;

	AVPacket							*packet;
	AVFrame								*frame;

	int32_t								stream_idx;
	int64_t								timebase;
} AVDataCtx;


#define InitAVDataCtx avddata_init
#define FreeAVDataCtx avddata_exit
#define OpenfileAVCodec avcodec_open_file

AVDataCtx *avddata_alloc(void);
void avddata_free(AVDataCtx **avd);

int avddata_init(AVDataCtx *avd);
void avddata_exit(AVDataCtx *avd);

int avcodec_open_file(AVDataCtx *avd, const char *filepath);
int avcodec_decode_file(AVDataCtx *avd);

/* Free allocated packet and frame resources */
int avcodec_decode_unref(AVDataCtx *avd);

/**
 * Return number of samples output per channel
 */
int avswr_init(
	AVSwrCtx *swr,
	AVDataCtx *avd,
	uint64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt);


/**
 * Performance software resampling
 * src_data can be decoded frame data
 * return number of samples per channel
 *
 * @warning: invoke avswr_param_update() first
 */
int avswr_convert(AVSwrCtx *swr, uint8_t **src_data);

/**
 * Update SWR dependent parameters
 */
int avswr_param_update(AVSwrCtx *swr, int src_nb_samples);