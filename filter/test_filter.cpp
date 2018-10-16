//
// Created by guowei on 2018/10/9.
//

#include <iostream>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

const char* filterDescription = "scale=78:24,transpose=cclock";

static AVFormatContext* formatContext;
static AVCodecContext* codecContext;

static AVFilterContext* bufferSinkContext;
static AVFilterContext* bufferSourceContext;
static AVFilterGraph* filterGraph;

static int videoStreamIndex = -1;
static int64_t lastPts = AV_NOPTS_VALUE;

static int OpenInputFile(const char* fileName) {
  int ret;
  AVCodec* codec;

  ret = avformat_open_input(&formatContext, fileName, NULL, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return  ret;
  }

  ret = avformat_find_stream_info(formatContext, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return  ret;
  }

  ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannnot find a video stream in the input file\n");
    return 0;
  }

  videoStreamIndex = ret;
  av_log(NULL, AV_LOG_INFO, "videoStreamIndex:%d", videoStreamIndex);

  codecContext = avcodec_alloc_context3(codec);
  if (!codecContext) {
    return AVERROR(ENOMEM);
  }

  avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamIndex]->codecpar);

  ret = avcodec_open2(codecContext, codec, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
    return ret;
  }

  return 0;

}

static int InitFilters(const char* filterDescription)
{
  char args[512];
  int ret = 0;

  const AVFilter* bufferSource = avfilter_get_by_name("buffer");
  const AVFilter* bufferSink = avfilter_get_by_name("buffersink");

  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs = avfilter_inout_alloc();

  AVRational timeBase = formatContext->streams[videoStreamIndex]->time_base;
  enum AVPixelFormat pixelFormat[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE};

  filterGraph = avfilter_graph_alloc();

  if (!inputs || !outputs || !filterGraph) {
    ret = AVERROR(ENOMEM);
    goto end;
  }

  snprintf(args, sizeof(args),
      "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      codecContext->width,
      codecContext->height,
      codecContext->pix_fmt,
      timeBase.num,
      timeBase.den,
      codecContext->sample_aspect_ratio.num,
      codecContext->sample_aspect_ratio.den);

  av_log(NULL, AV_LOG_INFO, "args:%s", args);

  ret = avfilter_graph_create_filter(&bufferSourceContext, bufferSource,
      "in", args, NULL, filterGraph);

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
    goto end;
  }

  ret = avfilter_graph_create_filter(&bufferSinkContext, bufferSink,
      "out", NULL, NULL, filterGraph);

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot create sink source\n");
    goto end;
  }

  ret = av_opt_set_int_list(bufferSinkContext, "pix_fmts", pixelFormat,
      AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot set ouput pixel format\n");
    goto end;
  }

  outputs->name = av_strdup("in");
  outputs->filter_ctx = bufferSourceContext;
  outputs->pad_idx = 0;
  outputs->next = NULL;

  inputs->name = av_strdup("out");
  inputs->filter_ctx = bufferSinkContext;
  inputs->pad_idx = 0;
  inputs->next = NULL;

  ret = avfilter_graph_parse_ptr(filterGraph, filterDescription, &inputs, &outputs, NULL);
  if (ret < 0) {
    goto end;
  }

  ret = avfilter_graph_config(filterGraph, NULL);
  if (ret < 0) {
    goto end;
  }


end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);
  return ret;

}

static void DisplayFrame(const AVFrame* frame, AVRational timeBase)
{
  uint8_t* p0;
  uint8_t* p;
  int64_t delay;

  if (frame->pts != AV_NOPTS_VALUE) {
    if (lastPts != AV_NOPTS_VALUE) {
      delay = av_rescale_q(frame->pts - lastPts, timeBase, AV_TIME_BASE_Q);
      if (delay > 0 && delay < 1000000) {
        usleep(delay);
      }
    }
    lastPts = frame->pts;
  }

  p0 = frame->data[0];
  puts("\033c");

  for (int y = 0; y < frame->height; ++y) {
    p = p0;
    for (int x = 0; x < frame->width; ++x) {
      putchar(" .-+#"[*(p++) / 52]);
    }
    putchar('\n');
    p0 += frame->linesize[0];
  }

  fflush(stdout);

}

int main(int argc, char* argv[])
{

  int ret;
  AVPacket packet;
  AVFrame* frame;
  AVFrame* filterFrame;

  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " file" << std::endl;
    return 0;
  }

  frame = av_frame_alloc();
  filterFrame = av_frame_alloc();

  if (!frame || !filterFrame) {
    std::cout << "Could not allocate frame" << std::endl;
    return 0;
  }

  ret = OpenInputFile(argv[1]);
  if (ret < 0) {
    goto end;
  }

  ret = InitFilters(filterDescription);
  if (ret < 0) {
    goto end;
  }

  while (1) {

    ret = av_read_frame(formatContext, &packet);
    if (ret < 0) {
      break;
    }

    if (packet.stream_index == videoStreamIndex) {

      ret = avcodec_send_packet(codecContext, &packet);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "ERROR while sending a packet to the decoder\n");
        break;
      }

      while (ret >= 0) {

        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        } else if (ret < 0) {
          av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
          goto end;
        }

        frame->pts = frame->best_effort_timestamp;

        if (av_buffersrc_add_frame_flags(bufferSourceContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
          av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
          break;
        }

        while (1) {
          ret = av_buffersink_get_frame(bufferSinkContext, filterFrame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
          } else if (ret < 0) {
            goto end;
          }

          DisplayFrame(filterFrame, bufferSinkContext->inputs[0]->time_base);
          av_frame_unref(filterFrame);
        }

        av_frame_unref(frame);
      }
    }
    av_packet_unref(&packet);
  }

end:

  avfilter_graph_free(&filterGraph);
  avcodec_free_context(&codecContext);
  avformat_close_input(&formatContext);
  av_frame_free(&frame);
  av_frame_free(&filterFrame);

  if (ret < 0 && ret != AVERROR_EOF) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, ret);
    std::cout << "Error occurred:" << str << std::endl;
  }

  return 0;
}