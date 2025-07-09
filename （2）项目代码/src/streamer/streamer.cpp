//
// Created by Yuan on 2023/11/23.
//
#include "streamer.h"

#include <string>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <cerrno>
#include <mutex>
#include <algorithm>


int udp_socket_1;
int udp_socket_2;
uint8_t udp_send[1400];
namespace streamer
{

#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P

static int encode_and_write_frame(AVCodecContext *codec_ctx, AVFormatContext *fmt_ctx, AVFrame *frame)
{
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending frame to codec context!\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            break;
        }

        // RTP 数据包大小过大会导致丢包，确保数据包小于 1460 字节
        if (pkt.size > 1460) {
            // 分片或调整数据包大小（这里的处理仅为示例，具体方式取决于你的需求）
            fprintf(stderr, "NAL size %d exceeds 1460, fragmentation might be needed\n", pkt.size);
        }

        av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
    }

    return 0;
}

static int set_options_and_open_encoder(AVFormatContext *fctx, AVStream *stream, AVCodecContext *codec_ctx, AVCodec *codec,
                                        std::string codec_profile, double width, double height,
                                        int fps, int bitrate, AVCodecID codec_id)
{
    const AVRational dst_fps = {fps, 1};

    codec_ctx->codec_tag = 0;
    codec_ctx->codec_id = codec_id;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->gop_size = 30;
    codec_ctx->keyint_min = 8;  // 设置关键帧间隔
    codec_ctx->max_b_frames = 0;  // 禁用 B 帧，减小编码延迟
    codec_ctx->pix_fmt = STREAM_PIX_FMT;
    codec_ctx->framerate = dst_fps;
    codec_ctx->time_base = av_inv_q(dst_fps);
    codec_ctx->bit_rate = bitrate;
    codec_ctx->profile = FF_PROFILE_H264_HIGH;  // 使用 H264 高级配置
    codec_ctx->level = 41;  // 可以根据实际需求设置

    if (fctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    stream->time_base = codec_ctx->time_base;

    int ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0)
    {
        fprintf(stderr, "Could not initialize stream codec parameters!\n");
        return 1;
    }

    AVDictionary *codec_options = nullptr;
    av_dict_set(&codec_options, "preset", "ultrafast", 0);
    av_dict_set(&codec_options, "tune", "zerolatency", 0);  // 优化实时性

    ret = avcodec_open2(codec_ctx, codec, &codec_options);
    if (ret < 0)
    {
        fprintf(stderr, "Could not open video encoder!\n");
        return 1;
    }
    av_dict_free(&codec_options);
    return 0;
}

Streamer::Streamer()
{
    format_ctx = nullptr;
    out_codec = nullptr;
    out_stream = nullptr;
    out_codec_ctx = nullptr;
    rtmp_server_conn = false;
    av_register_all();
    inv_stream_timebase = 30.0;
    network_init_ok = !avformat_network_init();
}

void Streamer::cleanup()
{
    if(out_codec_ctx) {
        avcodec_close(out_codec_ctx);
        avcodec_free_context(&out_codec_ctx);
    }

    if(format_ctx) {
        if(format_ctx->pb) {
            avio_close(format_ctx->pb);
        }
        avformat_free_context(format_ctx);
        format_ctx = nullptr;
    }
}

Streamer::~Streamer()
{
    cleanup();
    avformat_network_deinit();
}

void Streamer::stream_frame(const uint8_t *data)
{
    if(can_stream()) {
        const int stride[] = {static_cast<int>(config.src_width * 3)};
        sws_scale(scaler.ctx, &data, stride, 0, config.src_height, picture.frame->data, picture.frame->linesize);
        picture.frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
        encode_and_write_frame(out_codec_ctx, format_ctx, picture.frame);
    }
}

void Streamer::stream_frame(const uint8_t *data, int64_t frame_duration)
{
    if(can_stream()) {
        const int stride[] = {static_cast<int>(config.src_width * 3)};
        sws_scale(scaler.ctx, &data, stride, 0, config.src_height, picture.frame->data, picture.frame->linesize);
        picture.frame->pts += frame_duration; // 时间戳设置
        encode_and_write_frame(out_codec_ctx, format_ctx, picture.frame);
    }
}

void Streamer::enable_av_debug_log()
{
    av_log_set_level(AV_LOG_DEBUG);
}

int Streamer::init(const StreamerConfig &streamer_config)
{
    init_ok = false;
    cleanup();

    config = streamer_config;

    if(!network_init_ok) {
        return 1;
    }

    // 创建输出上下文，设置输出格式为 RTP
    avformat_alloc_output_context2(&format_ctx, nullptr, "rtp", "udp://192.168.31.237:1234");
    if(!format_ctx) {
        return 1;
    }

    // 使用 UDP 协议
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        int avopen_ret = avio_open2(&format_ctx->pb, "udp://192.168.31.237:1234",
                                     AVIO_FLAG_WRITE, nullptr, nullptr);  // 配置为 UDP 地址（例如 udp://127.0.0.1:5004）
        if (avopen_ret < 0)  {
            fprintf(stderr, "failed to open stream output context, stream will not work\n");
            return 1;
        }
        rtmp_server_conn = true;
    }

    // 选择编码器：H.264
    AVCodecID codec_id = AV_CODEC_ID_H264;
    out_codec = avcodec_find_encoder(codec_id);
    if (!(out_codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return 1;
    }

    // 创建视频流
    out_stream = avformat_new_stream(format_ctx, out_codec);
    if (!out_stream) {
        fprintf(stderr, "Could not allocate stream\n");
        return 1;
    }

    out_codec_ctx = avcodec_alloc_context3(out_codec);

    // 设置编码器选项
    if (set_options_and_open_encoder(format_ctx, out_stream, out_codec_ctx, out_codec, config.profile,
                                    config.dst_width, config.dst_height, config.fps, config.bitrate, codec_id)) {
        return 1;
    }

    // 复制编码器数据
    out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;
    out_stream->codecpar->extradata = static_cast<uint8_t*>(av_mallocz(out_codec_ctx->extradata_size));
    memcpy(out_stream->codecpar->extradata, out_codec_ctx->extradata, out_codec_ctx->extradata_size);

    av_dump_format(format_ctx, 0, "udp://192.168.31.237:1234", 1);

    // 初始化图像缓冲区和缩放器
    picture.init(out_codec_ctx->pix_fmt, config.dst_width, config.dst_height);
    scaler.init(out_codec_ctx, config.src_width, config.src_height, config.dst_width, config.dst_height, SWS_BILINEAR);

    // 写入文件头
    if (avformat_write_header(format_ctx, nullptr) < 0)
    {
        fprintf(stderr, "Could not write header!\n");
        return 1;
    }

    inv_stream_timebase = (double)out_stream->time_base.den/(double)out_stream->time_base.num;

    init_ok = true;
    return 0;
}

} // namespace streamer
