/**
 * This file test the ffmpeg audio decoding
 * Produce a PCM audio stream into a file with given format
 * 
 *		Author: cocafe <cocafehj@gmail.com>
 */

#include "..\stdafx.h"
#include "..\ffmpeg.h"
#include "..\debug.h"

#include <test.h>

int test_ffmpeg_decode(const char *url_open, const char *url_output)
{
	AVDataCtx *avd;
	FILE *out;
	int ret = 0;

	avd = avdata_alloc();
	if (!avd)
		return -ENOMEM;

	if (avdata_init(avd)) {
		goto err_init;
	}

	if (avcodec_open_file(avd, url_open)) {
		goto err_open;
	}

	out = fopen(url_output, "wb");
	if (!out) {
		goto err_open;
	}

	do {
		ret = avcodec_decode_file(avd);
		if (ret == -EOF) {
			pr_console("%s: end of file reached\n", __func__);
			break;
		} else if (ret < 0) {
			pr_console("%s: error occurred (%d)\n", __func__, ret);
			goto err_decode;
		}

		pr_console("%s: pts: (%d/%d) nb_samples: %d pkt_sz: %d\n",
			__func__,
			avd->packet->pts,
			avd->format_ctx->streams[avd->stream_idx]->duration,
			avd->frame->nb_samples,
			avd->packet->size);

		fwrite(
			avd->frame->buf[avd->stream_idx]->data,
			avd->frame->buf[avd->stream_idx]->size,
			1,
			out);

		avcodec_decode_unref(avd);

	} while (1);


err_decode:
	fflush(out);
	fclose(out);

err_open:
	avdata_exit(avd);
	avdata_free(&avd);

err_init:
	return 0;
}

int test_ffmpeg_swr(
	const char *url_open, 
	const char *url_output,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	AVDataCtx *avd;
	FILE *out;
	int ret = 0;

	avd = avdata_alloc();
	if (!avd)
		return -ENOMEM;

	if (avdata_init(avd)) {
		goto err_init;
	}

	if (avcodec_open_file(avd, url_open)) {
		goto err_open;
	}

	if (avswr_init(
		avd->swr, 
		avd, 
		target_chnl_layout,
		target_sample_rate, 
		target_sample_fmt)) {
		goto err_swr_init;
	}

	out = fopen(url_output, "wb");
	if (!out) {
		goto err_open;
	}

	do {
		ret = avcodec_decode_file(avd);
		if (ret < 0) {
			if (avd->format_ctx->pb->eof_reached)
				pr_console("%s: eof reached\n", __func__);
			else
				pr_console("%s: error occurred during frame decode(%#04x)\n", __func__, ret);

			goto err_proc;
		}

		pr_console("pts: (%lu/%lu) nb_samples: %d pkt_sz: %d swr_nb_samples: %d swr_bufsz: %d\n",
			avd->frame->pts,
			avd->format_ctx->streams[avd->stream_idx]->duration,
			avd->frame->nb_samples,
			avd->frame->pkt_size,
			avd->swr->dst_nb_samples,
			avd->swr->dst_bufsize);

		/* 
		 * WORKAROUND: 
		 * fix tiny noise and error read at the end of output file
		 * the duration estimate method might return an incorrect duration
		 * then cause the decoder reads over ahead, skip the last frame
		 * 
		 * FIXME:
		 * can we change the duration estimate method? (X)
		 * 
		 * SOLUTION:
		 * program crashes at the end of file caused by changed src_nb_samples
		 * at the end of file, fixed
		 */
		//if ((avd->frame->pts + avd->frame->nb_samples) 
		//	>= avd->format_ctx->streams[avd->stream_idx]->duration)
		//	continue;

		avswr_param_update(avd->swr, avd->frame->nb_samples);

		/* swr resample */
		ret = avswr_convert(avd->swr, avd->frame->data);
		if (ret < 0) {
			pr_console("%s: error occurred in swr (%#04x)\n", __func__, ret);
			goto err_proc;
		}

		fwrite(
			avd->swr->dst_data[avd->stream_idx],
			avd->swr->dst_bufsize,
			1,
			out);

		avcodec_decode_unref(avd);
	} while (1);


err_proc:
	fflush(out);
	fclose(out);

err_swr_init:

err_open:
	avdata_exit(avd);
	avdata_free(&avd);

err_init:
	return 0;
}

int test_ffmpeg_swr_buffered(
	const char *url_open,
	const char *url_output,
	int64_t target_chnl_layout,
	int32_t target_sample_rate,
	AVSampleFormat target_sample_fmt)
{
	AVDataCtx *avd;
	FILE *out;
	int ret = 0;

	/* buffer */
	uint8_t *buffer_data;
	int32_t buffer_pos;
	int32_t buffer_count;
	int32_t buffer_size;

	avd = avdata_alloc();
	if (!avd)
		return -ENOMEM;

	if (avdata_init(avd)) {
		goto err_init;
	}

	if (avcodec_open_file(avd, url_open)) {
		goto err_open;
	}

	if (avswr_init(
		avd->swr,
		avd,
		target_chnl_layout,
		target_sample_rate,
		target_sample_fmt)) {
		goto err_swr_init;
	}

	out = fopen(url_output, "wb");
	if (!out) {
		goto err_open;
	}

	/* buffer */
	buffer_pos = 0;
	buffer_count = 0;
	buffer_size = avd->swr->dst_linesize * 5;
	buffer_data = (uint8_t *)malloc(buffer_size);
	memset(buffer_data, 0xEE, buffer_size);

	do {
		ret = avcodec_decode_file(avd);
		if (ret < 0) {
			if (avd->format_ctx->pb->eof_reached)
				pr_console("%s: eof reached\n", __func__);
			else
				pr_console("%s: error occurred or eof (%#04x)\n", __func__, ret);

			if (buffer_pos)
				goto out_buffer;
			else
				goto out_proc;
		}

		pr_console("pts: (%lu/%lu) nb_samples: %d pkt_sz: %d swr_nb_samples: %d swr_bufsz: %d\n",
			avd->frame->pts,
			avd->format_ctx->streams[avd->stream_idx]->duration,
			avd->frame->nb_samples,
			avd->frame->pkt_size,
			avd->swr->dst_nb_samples,
			avd->swr->dst_bufsize);

		avswr_param_update(avd->swr, avd->frame->nb_samples);

		/* swr resample */
		ret = avswr_convert(avd->swr, avd->frame->data);
		if (ret < ret) {
			pr_console("%s: error occurred in swr (%#04x)\n", __func__, ret);
			goto err_proc;
		}

		/* buffer */
		if (avd->swr->dst_bufsize > buffer_size) {
			/* write size is overflow */
			break; /* error */
		}

												/* == */
		if ((buffer_pos + avd->swr->dst_bufsize) > buffer_size) {
			/* flush all buffer before writing new buffer
			 */
			pr_console("buffer: buffered %d times %d bytes\n", buffer_count, buffer_pos);
			fwrite(buffer_data, buffer_pos/* buffer_size */, 1, out);
			memset(buffer_data, 0x00, buffer_size);
			buffer_pos = 0;
			buffer_count = 0;
		}

		memcpy(buffer_data + buffer_pos, avd->swr->dst_data[avd->stream_idx], avd->swr->dst_bufsize);
		buffer_pos += avd->swr->dst_bufsize;
		buffer_count++;

		avcodec_decode_unref(avd);
	} while (1);

out_buffer:
	if (avd->format_ctx->pb->eof_reached && buffer_pos) {
		/* flush rest valid buffer */
		fwrite(buffer_data, buffer_pos, 1, out);
		memset(buffer_data, 0x00, buffer_size);
		buffer_pos = 0;
		buffer_count = 0;
	}


err_proc:
out_proc:
	fflush(out);
	fclose(out);
	free(buffer_data);

err_swr_init:

err_open:
	avdata_exit(avd);
	avdata_free(&avd);

err_init:
	return 0;
}