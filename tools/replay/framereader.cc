#include "tools/replay/framereader.h"

#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include "common/util.h"
#include "third_party/libyuv/include/libyuv.h"
#include "tools/replay/util.h"

#ifdef __APPLE__
#define HW_DEVICE_TYPE AV_HWDEVICE_TYPE_VIDEOTOOLBOX
#define HW_PIX_FMT AV_PIX_FMT_VIDEOTOOLBOX
#else
#define HW_DEVICE_TYPE AV_HWDEVICE_TYPE_CUDA
#define HW_PIX_FMT AV_PIX_FMT_CUDA
#endif

// Rockchip MPP hardware decoding (e.g. RK3588 VPU) is only meaningful on Linux.
// It outputs AV_PIX_FMT_DRM_PRIME frames that are read back by mmapping the
// underlying dma-buf file descriptors.
#ifdef __linux__
#include <sys/mman.h>

#include <libavutil/hwcontext_drm.h>

#define RKMPP_ENABLED 1
#endif

namespace {

enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
  enum AVPixelFormat *hw_pix_fmt = reinterpret_cast<enum AVPixelFormat *>(ctx->opaque);
  for (const enum AVPixelFormat *p = pix_fmts; *p != -1; p++) {
    if (*p == *hw_pix_fmt) return *p;
  }
  rWarning("Please run replay with the --no-hw-decoder flag!");
  *hw_pix_fmt = AV_PIX_FMT_NONE;
  return AV_PIX_FMT_YUV420P;
}

struct DecoderManager {
  VideoDecoder *acquire(CameraType type, AVCodecParameters *codecpar, bool hw_decoder) {
    auto key = std::tuple(type, codecpar->width, codecpar->height);
    std::unique_lock lock(mutex_);
    if (auto it = decoders_.find(key); it != decoders_.end()) {
      return it->second.get();
    }

    auto decoder = std::make_unique<VideoDecoder>();
    if (!decoder->open(codecpar, hw_decoder)) {
      decoder.reset(nullptr);
    }
    decoders_[key] = std::move(decoder);
    return decoders_[key].get();
  }

  std::mutex mutex_;
  std::map<std::tuple<CameraType, int, int>, std::unique_ptr<VideoDecoder>> decoders_;
};

DecoderManager decoder_manager;

}  // namespace

FrameReader::FrameReader() {
  av_log_set_level(AV_LOG_QUIET);
}

FrameReader::~FrameReader() {
  if (input_ctx) avformat_close_input(&input_ctx);
}

bool FrameReader::load(CameraType type, const std::string &url, bool no_hw_decoder, std::atomic<bool> *abort, bool local_cache, int chunk_size, int retries) {
  auto local_file_path = url.find("https://") == 0 ? cacheFilePath(url) : url;
  if (!util::file_exists(local_file_path)) {
    FileReader f(local_cache, chunk_size, retries);
    if (f.read(url, abort).empty()) {
      return false;
    }
  }
  return loadFromFile(type, local_file_path, no_hw_decoder, abort);
}

bool FrameReader::loadFromFile(CameraType type, const std::string &file, bool no_hw_decoder, std::atomic<bool> *abort) {
  if (avformat_open_input(&input_ctx, file.c_str(), nullptr, nullptr) != 0 ||
      avformat_find_stream_info(input_ctx, nullptr) < 0) {
    rError("Failed to open input file or find video stream");
    return false;
  }
  input_ctx->probesize = 10 * 1024 * 1024;  // 10MB

  decoder_ = decoder_manager.acquire(type, input_ctx->streams[0]->codecpar, !no_hw_decoder);
  if (!decoder_) {
    return false;
  }
  width = decoder_->width;
  height = decoder_->height;

  AVPacket pkt;
  packets_info.reserve(60 * 20);  // 20fps, one minute
  while (!(abort && *abort) && av_read_frame(input_ctx, &pkt) == 0) {
    packets_info.emplace_back(PacketInfo{.flags = pkt.flags, .pos = pkt.pos});
    av_packet_unref(&pkt);
  }
  avio_seek(input_ctx->pb, 0, SEEK_SET);
  return !packets_info.empty();
}

bool FrameReader::get(int idx, VisionBuf *buf) {
  if (!buf || idx < 0 || idx >= packets_info.size()) {
    return false;
  }
  return decoder_->decode(this, idx, buf);
}

// class VideoDecoder

VideoDecoder::VideoDecoder() {
  av_frame_ = av_frame_alloc();
  hw_frame_ = av_frame_alloc();
}

VideoDecoder::~VideoDecoder() {
  if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
  if (decoder_ctx) avcodec_free_context(&decoder_ctx);
  av_frame_free(&av_frame_);
  av_frame_free(&hw_frame_);
}

bool VideoDecoder::open(AVCodecParameters *codecpar, bool hw_decoder) {
  // Prefer Rockchip MPP hardware decoding on Linux (e.g. RK3588 VPU).
  if (hw_decoder && initRkmppDecoder(codecpar)) {
    return true;
  }

  const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
  if (!decoder) return false;

  decoder_ctx = avcodec_alloc_context3(decoder);
  if (!decoder_ctx || avcodec_parameters_to_context(decoder_ctx, codecpar) != 0) {
    rError("Failed to allocate or initialize codec context");
    return false;
  }
  width = (decoder_ctx->width + 3) & ~3;
  height = decoder_ctx->height;

  if (hw_decoder && !initHardwareDecoder(HW_DEVICE_TYPE)) {
    rWarning("No device with hardware decoder found. fallback to CPU decoding.");
  }

  if (avcodec_open2(decoder_ctx, decoder, nullptr) < 0) {
    rError("Failed to open codec");
    return false;
  }
  return true;
}

bool VideoDecoder::initHardwareDecoder(AVHWDeviceType hw_device_type) {
  const AVCodecHWConfig *config = nullptr;
  for (int i = 0; (config = avcodec_get_hw_config(decoder_ctx->codec, i)) != nullptr; i++) {
    if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == hw_device_type) {
      hw_pix_fmt = config->pix_fmt;
      break;
    }
  }
  if (!config) {
    rWarning("Hardware configuration not found");
    return false;
  }

  int ret = av_hwdevice_ctx_create(&hw_device_ctx, hw_device_type, nullptr, nullptr, 0);
  if (ret < 0) {
    hw_pix_fmt = AV_PIX_FMT_NONE;
    rWarning("Failed to create specified HW device %d.", ret);
    return false;
  }

  decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
  decoder_ctx->opaque = &hw_pix_fmt;
  decoder_ctx->get_format = get_hw_format;
  return true;
}

bool VideoDecoder::decode(FrameReader *reader, int idx, VisionBuf *buf) {
  int from_idx = idx;
  if (idx != reader->prev_idx + 1) {
    // seeking to the nearest key frame
    for (int i = idx; i >= 0; --i) {
      if (reader->packets_info[i].flags & AV_PKT_FLAG_KEY) {
        from_idx = i;
        break;
      }
    }

    auto pos = reader->packets_info[from_idx].pos;
    int ret = avformat_seek_file(reader->input_ctx, 0, pos, pos, pos, AVSEEK_FLAG_BYTE);
    if (ret < 0) {
      rError("Failed to seek to byte position %lld: %d", pos, AVERROR(ret));
      return false;
    }
    avcodec_flush_buffers(decoder_ctx);
  }
  reader->prev_idx = idx;

  bool result = false;
  AVPacket pkt;
  for (int i = from_idx; i <= idx; ++i) {
    if (av_read_frame(reader->input_ctx, &pkt) == 0) {
      AVFrame *f = decodeFrame(&pkt);
      if (f && i == idx) {
        result = copyBuffer(f, buf);
      }
      av_packet_unref(&pkt);
    }
  }
  return result;
}

AVFrame *VideoDecoder::decodeFrame(AVPacket *pkt) {
  int ret = avcodec_send_packet(decoder_ctx, pkt);
  if (ret < 0) {
    rError("Error sending a packet for decoding: %d", ret);
    return nullptr;
  }

  ret = avcodec_receive_frame(decoder_ctx, av_frame_);
  if (ret != 0) {
    rError("avcodec_receive_frame error: %d", ret);
    return nullptr;
  }

  // rkmpp produces DRM_PRIME (dma-buf) frames; readback happens in copyBuffer().
  if (av_frame_->format == AV_PIX_FMT_DRM_PRIME) {
    return av_frame_;
  }

  if (av_frame_->format == hw_pix_fmt && av_hwframe_transfer_data(hw_frame_, av_frame_, 0) < 0) {
    rError("error transferring frame data from GPU to CPU");
    return nullptr;
  }
  return (av_frame_->format == hw_pix_fmt) ? hw_frame_ : av_frame_;
}

bool VideoDecoder::copyBuffer(AVFrame *f, VisionBuf *buf) {
  if (f->format == AV_PIX_FMT_DRM_PRIME) {
    return copyDrmPrimeBuffer(f, buf);
  }

  if (hw_pix_fmt == HW_PIX_FMT) {
    for (int i = 0; i < height/2; i++) {
      memcpy(buf->y + (i*2 + 0)*buf->stride, f->data[0] + (i*2 + 0)*f->linesize[0], width);
      memcpy(buf->y + (i*2 + 1)*buf->stride, f->data[0] + (i*2 + 1)*f->linesize[0], width);
      memcpy(buf->uv + i*buf->stride, f->data[1] + i*f->linesize[1], width);
    }
  } else {
    libyuv::I420ToNV12(f->data[0], f->linesize[0],
                       f->data[1], f->linesize[1],
                       f->data[2], f->linesize[2],
                       buf->y, buf->stride,
                       buf->uv, buf->stride,
                       width, height);
  }
  return true;
}

bool VideoDecoder::initRkmppDecoder(const AVCodecParameters *codecpar) {
#ifdef RKMPP_ENABLED
  const char *name = nullptr;
  switch (codecpar->codec_id) {
    case AV_CODEC_ID_HEVC: name = "hevc_rkmpp"; break;
    case AV_CODEC_ID_H264: name = "h264_rkmpp"; break;
    default:
      rWarning("rkmpp: unsupported codec id %d", codecpar->codec_id);
      return false;
  }

  const AVCodec *decoder = avcodec_find_decoder_by_name(name);
  if (!decoder) {
    rWarning("rkmpp: decoder '%s' not found in this FFmpeg build", name);
    return false;
  }

  AVCodecContext *ctx = avcodec_alloc_context3(decoder);
  if (!ctx || avcodec_parameters_to_context(ctx, codecpar) != 0) {
    if (ctx) avcodec_free_context(&ctx);
    rWarning("rkmpp: failed to allocate or init codec context for %s", name);
    return false;
  }

  if (avcodec_open2(ctx, decoder, nullptr) < 0) {
    avcodec_free_context(&ctx);
    rWarning("rkmpp: avcodec_open2 failed for %s", name);
    return false;
  }

  decoder_ctx = ctx;
  width = (decoder_ctx->width + 3) & ~3;
  height = (decoder_ctx->height + 3) & ~3;
  rInfo("Using Rockchip MPP hardware decoder (%s), %dx%d", name, width, height);
  return true;
#else
  (void)codecpar;
  return false;
#endif
}

bool VideoDecoder::copyDrmPrimeBuffer(AVFrame *f, VisionBuf *buf) {
#ifdef RKMPP_ENABLED
  auto *desc = (const AVDRMFrameDescriptor *)f->data[0];
  if (desc == nullptr || desc->nb_layers <= 0) {
    rError("Invalid DRM_PRIME frame descriptor");
    return false;
  }

  const AVDRMLayerDescriptor *layer = &desc->layers[0];
  // MPP output is NV12: plane0 = Y, plane1 = interleaved UV.
  if (layer->nb_planes < 2) {
    rWarning("Unexpected MPP frame with %d planes (expected 2 for NV12)", layer->nb_planes);
    return false;
  }

  auto copyPlane = [&](int plane_idx, uint8_t *dst, int rows) {
    const AVDRMPlaneDescriptor *plane = &layer->planes[plane_idx];
    const AVDRMObjectDescriptor *obj = &desc->objects[plane->object_index];

    void *map = mmap(nullptr, (size_t)obj->size, PROT_READ, MAP_SHARED, obj->fd, 0);
    if (map == MAP_FAILED) {
      rError("Failed to mmap MPP dma-buf (fd=%d, size=%ld)", obj->fd, (long)obj->size);
      return false;
    }

    const uint8_t *src = (const uint8_t *)map + plane->offset;
    for (int row = 0; row < rows; row++) {
      memcpy(dst + row * buf->stride, src + row * plane->pitch, width);
    }
    munmap(map, obj->size);
    return true;
  };

  if (!copyPlane(0, buf->y, height)) return false;
  if (!copyPlane(1, buf->uv, height / 2)) return false;
  return true;
#else
  (void)f;
  (void)buf;
  return false;
#endif
}
