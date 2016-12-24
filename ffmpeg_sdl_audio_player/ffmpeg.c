#include "stdafx.h"
#include "ffmpeg.h"

AVSwrCtx *avswr_alloc(void)
{
	AVSwrCtx *swr;

	swr = (AVSwrCtx *)calloc(1, sizeof(AVSwrCtx));
	if (!swr) {
		return NULL;
	}

	return swr;
}

void avswr_free(AVSwrCtx **swr)
{
	if (!*swr)
		return;

	swr_free(&(*swr)->swr_ctx);

	free(*swr);
	*swr = NULL;
}

uint16_t frame_size_detect(
	AVFormatContext *fmt_ctx,
	AVCodecContext *codec_ctx,
	int stream_idx
	)
{
	AVPacket *packet;
	AVFrame *frame;
	uint16_t frame_size;
	int i;
#define FRAME_MAX_TRY (10)

	packet = av_packet_alloc();
	if (!packet)
		return -ENOMEM;

	frame = av_frame_alloc();
	if (!frame)
		return -ENOMEM;

	av_init_packet(packet);

	for (i = 0, frame_size = 0; i < FRAME_MAX_TRY; i++) {
		if (av_read_frame(fmt_ctx, packet) >= 0) {
			if (packet->stream_index == stream_idx) {
				avcodec_send_packet(codec_ctx, packet);
				avcodec_receive_frame(codec_ctx, frame);
			}

			av_packet_unref(packet);

			pr_console("%s: %d %d\n", __func__, i, frame->nb_samples);

			if (frame->nb_samples > frame_size)
				frame_size = (uint16_t)frame->nb_samples;
		}
	}

	pr_console("%s: frame_size: %d\n", __func__, frame_size);

	av_frame_free(&frame);
	av_packet_free(&packet);

	av_seek_frame(fmt_ctx, stream_idx, 0, 0);

	return frame_size;
}

int avswr_init(
	AVSwrCtx *swr,
	AVDataCtx *avd,
	uint64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	int ret;

	if (!swr)
		return -EINVAL;

	if (!avd)
		return -EINVAL;

	swr->swr_ctx = swr_alloc();
	if (!swr->swr_ctx) {
		return -ENOMEM;
	}

	swr->src_sample_rate = avd->codec_ctx->sample_rate;
	swr->src_sample_fmt = avd->codec_ctx->sample_fmt;
	swr->src_nb_samples = avd->codec_ctx->frame_size ? 
		avd->codec_ctx->frame_size : 
		frame_size_detect(avd->format_ctx, avd->codec_ctx, avd->stream_idx);
	swr->src_nb_chnl = avd->codec_ctx->channels;
	swr->src_chnl_layout = avd->codec_ctx->channel_layout ? 
		avd->codec_ctx->channel_layout : 
		av_get_default_channel_layout(avd->codec_ctx->channels);

	swr->dst_sample_rate = target_sample_rate;
	swr->dst_sample_fmt = target_sample_fmt;
	swr->dst_nb_chnl = 0;	/* set below */
	swr->dst_chnl_layout = target_chnl_layout;

	av_opt_set_int(swr->swr_ctx, "in_sample_rate", swr->src_sample_rate, 0);
	av_opt_set_sample_fmt(swr->swr_ctx, "in_sample_fmt", swr->src_sample_fmt, 0);
	av_opt_set_int(swr->swr_ctx, "in_channel_layout", swr->src_chnl_layout, 0);

	av_opt_set_int(swr->swr_ctx, "out_sample_rate", swr->dst_sample_rate, 0);
	av_opt_set_sample_fmt(swr->swr_ctx, "out_sample_fmt", swr->dst_sample_fmt, 0);
	av_opt_set_int(swr->swr_ctx, "out_channel_layout", swr->dst_chnl_layout, 0);

	if (swr_init(swr->swr_ctx)) {
		return -ENODEV;
	}

	/* compute the number of converted samples: buffering is avoided
	 * ensuring that the output buffer will contain at least all the
	 * converted input samples */
	swr->dst_nb_samples = av_rescale_rnd(
		swr->src_nb_samples,
		swr->dst_sample_rate,
		swr->src_sample_rate,
		AV_ROUND_UP);

	swr->dst_nb_samples_max = swr->dst_nb_samples;

	/* buffer is going to be directly written to a raw audio file, no align */
	swr->dst_nb_chnl = av_get_channel_layout_nb_channels(swr->dst_chnl_layout);
	ret = av_samples_alloc_array_and_samples(
		&(swr->dst_data),
		&swr->dst_linesize,
		swr->dst_nb_chnl,
		(int32_t)swr->dst_nb_samples,
		swr->dst_sample_fmt,
		0);

	if (ret < 0) {
		return -ENODATA;
	}

	return 0;
}

int avddata_init(AVDataCtx *avd)
{
	if (!avd)
		return -EINVAL;

	memset(avd, 0x00, sizeof(AVDataCtx));

	av_register_all();
	avformat_network_init();

	avd->format_ctx = avformat_alloc_context();
	if (!avd->format_ctx) {
		return -ENOMEM;
	}

	avd->packet = av_packet_alloc();
	if (!avd->packet) {
		return -ENOMEM;
	}

	av_init_packet(avd->packet);

	avd->frame = av_frame_alloc();
	if (!avd->frame) {
		return -ENOMEM;
	}

	avd->swr = avswr_alloc();
	if (!avd->swr) {
		return -ENOMEM;
	}

	return 0;
}

void avddata_exit(AVDataCtx *avd)
{
	swr_free(&avd->swr->swr_ctx);
	avswr_free(&avd->swr);
	avformat_network_deinit();
}

AVDataCtx *avddata_alloc(void)
{
	AVDataCtx *avd;

	avd = (AVDataCtx *)calloc(1, sizeof(AVDataCtx));
	if (!avd)
		return NULL;

	return avd;
}

void avddata_free(AVDataCtx **avd)
{
	if (!*avd)
		return;

	free(*avd);

	*avd = NULL;
}

int avcodec_open_file(AVDataCtx *avd, const char *filepath)
{
	int ret;

	if (!avd)
		return -EINVAL;

	if (!filepath)
		return -EINVAL;

	/* TODO: Fix WCHAR */
	if (avformat_open_input(&avd->format_ctx, filepath, NULL, NULL) < 0) {
		return -ENODATA;
	}

	if (avformat_find_stream_info(avd->format_ctx, NULL) < 0) {
		return -ENODATA;
	}

	ret = av_find_best_stream(avd->format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &avd->codec, 0);
	if (ret < 0) {
		return -ENODATA;
	}

	avd->stream_idx = ret;

	avd->codec_ctx = avd->format_ctx->streams[avd->stream_idx]->codec;
	av_opt_set_int(avd->codec_ctx, "refcounted_frames", 1, 0);

	if (avcodec_open2(avd->codec_ctx, avd->codec, NULL) < 0) {
		return -ENODEV;
	}

	pr_console("%s:\n", __func__);
	pr_console("stream_idx: %d\n", avd->stream_idx);
	pr_console("bit rate: %3d\n", avd->format_ctx->bit_rate);
	pr_console("sample rate: %d\n", avd->codec_ctx->sample_rate);
	pr_console("decoder name: %s\n", avd->codec_ctx->codec->long_name);
	pr_console("channels: %d\n", avd->codec_ctx->channels);
	pr_console("duration: %d\n", avd->format_ctx->duration);

	return 0;
}

int avcodec_decode_file(AVDataCtx *avd)
{
	int ret = 0;

	if (!avd)
		return -EINVAL;

	ret = av_read_frame(avd->format_ctx, avd->packet);
	if (ret < 0) {
		pr_console("%s: av_read_frame() err return\n", __func__);
		return ret;
	}

	if (avd->packet->stream_index == avd->stream_idx) {
		ret = avcodec_send_packet(avd->codec_ctx, avd->packet);
		if (ret) {
			pr_console("%s: avcodec_send_packet() err return\n", __func__);
			return ret;
		}

		ret = avcodec_receive_frame(avd->codec_ctx, avd->frame);
		if (ret) {
			pr_console("%s: avcodec_receive_frame() err return\n", __func__);
			return ret;
		}
	}

	return ret;
}

int avcodec_decode_unref(AVDataCtx *avd)
{
	if (!avd)
		return -EINVAL;

	av_frame_unref(avd->frame);
	av_packet_unref(avd->packet);

	return 0;
}

/* 
 * SWR param src_nb_samples sometimes will change
 * We need to re-compute the SWR parameters
 */
int avswr_param_update(AVSwrCtx *swr, int src_nb_samples)
{
	if (!swr)
		return -EINVAL;

	swr->src_nb_samples = src_nb_samples;

	return 0;
}

int avswr_convert(AVSwrCtx *swr, uint8_t **src_data)
{
	int out_nb_samples;
	int ret;

	if (!swr)
		return -EINVAL;

	/* compute destination number of samples */
	swr->dst_nb_samples = av_rescale_rnd(
		swr_get_delay(swr->swr_ctx, swr->src_sample_rate) + swr->src_nb_samples,
		swr->dst_sample_rate,
		swr->src_sample_rate,
		AV_ROUND_UP);

	if (swr->dst_nb_samples > swr->dst_nb_samples_max) {
		av_freep(&swr->dst_data[0]);
		ret = av_samples_alloc(
			swr->dst_data,
			&swr->dst_linesize,
			swr->dst_nb_chnl,
			(int32_t)swr->dst_nb_samples,
			swr->dst_sample_fmt,
			0);
		if (ret < 0)
			return -ENOMEM;

		swr->dst_nb_samples_max = swr->dst_nb_samples;
	}

	/* convert to destination format */
	/* return number of samples output per channel */
	out_nb_samples = swr_convert(
		swr->swr_ctx,
		swr->dst_data,
		(int32_t)swr->dst_nb_samples,
		src_data,
		(int32_t)swr->src_nb_samples);	/* we ignored that src_nb_sampels will change! */

	if (out_nb_samples < 0) {
		return -EFAULT;
	}

	/* compute destination buffer size */
	swr->dst_bufsize = av_samples_get_buffer_size(
		&swr->dst_linesize, 
		swr->dst_nb_chnl,
		out_nb_samples, 
		swr->dst_sample_fmt, 
		0);
	if (swr->dst_bufsize < 0) {
		return -EFAULT;
	}

	return out_nb_samples;
}