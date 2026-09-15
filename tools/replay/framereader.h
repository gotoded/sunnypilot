#pragma once

#include <map>
#include <string>
#include <vector>

#include "msgq/visionipc/visionbuf.h"
#include "tools/replay/filereader.h"
#include "tools/replay/util.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoDecoder;

class FrameReader {
public:
  FrameReader();
  ~FrameReader();
  bool load(CameraType type, const std::string &url, bool no_hw_decoder = false, std::atomic<bool> *abort = nullptr, bool local_cache = false,
            int chunk_size = -1, int retries = 0);
  bool loadFromFile(CameraType type, const std::string &file, bool no_hw_decoder = false, std::atomic<bool> *abort = nullptr);
  bool get(int idx, VisionBuf *buf);
  size_t getFrameCount() const { return packets_info.size(); }

  int width = 0, height = 0;

  VideoDecoder *decoder_ = nullptr;
  AVFormatContext *input_ctx = nullptr;
  int prev_idx = -1;
  struct PacketInfo {
    int flags;
    int64_t pos;
  };
  std::vector<PacketInfo> packets_info;

  // Decoded-frame cache keyed by presentation index. Keeps the async RKMPP
  // pipeline running ahead of the playhead instead of blocking per frame.
  std::map<int, AVFrame *> decoded_cache;
  int next_feed_idx = 0;
  int next_output_idx = 0;
  // Force a decoder flush on the first decode so a shared VideoDecoder doesn't
  // carry leftover state from the previous segment's stream.
  bool needs_flush = true;
};


class VideoDecoder {
public:
  VideoDecoder();
  ~VideoDecoder();
  bool open(AVCodecParameters *codecpar, bool hw_decoder);
  bool decode(FrameReader *reader, int idx, VisionBuf *buf);
  int width = 0, height = 0;

private:
  bool initHardwareDecoder(AVHWDeviceType hw_device_type);
  bool initRkmppDecoder(const AVCodecParameters *codecpar);
  AVFrame *convertToSoftwareFrame(AVFrame *f);
  bool copyBuffer(AVFrame *f, VisionBuf *buf);
  bool copyDrmPrimeBuffer(AVFrame *f, VisionBuf *buf);

  AVFrame *av_frame_, *hw_frame_;
  AVCodecContext *decoder_ctx = nullptr;
  AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
  AVBufferRef *hw_device_ctx = nullptr;
};
