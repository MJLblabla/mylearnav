//
// Created by manjialle on 2022/1/24.
//
#include "VideoDecoder.h"


void VideoDecoder::init() {
    m_PacketQueue = new AVPacketQueue();
    m_PacketQueue->Start();
}

void VideoDecoder::unInit() {
    m_VideoRender->UnInit();
    m_VideoRender = nullptr;

    if (m_PacketQueue) {
        delete m_PacketQueue;
        m_PacketQueue = nullptr;
    }
}

int VideoDecoder::start(AVFormatContext *m_AVFormatContext) {
    // m_StartTimeStamp =-1;
    std::unique_lock <std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();

    int result = -1;
    do {
        timeBase = av_q2d(m_AVFormatContext->streams[m_StreamIdx]->time_base);
        //5.获取解码器参数
        AVCodecParameters *codecParameters = m_AVFormatContext->streams[m_StreamIdx]->codecpar;

        //6.获取解码器
        //  m_AVCodec = avcodec_find_decoder(codecParameters->codec_id);
        m_AVCodec = avcodec_find_decoder_by_name("h264_mediacodec");
        if (m_AVCodec == nullptr) {
            LOGCATE("VideoDecoder::InitFFDecoder avcodec_find_decoder fail.");
            break;
        }

        //7.创建解码器上下文
        m_AVCodecContext = avcodec_alloc_context3(m_AVCodec);
        if (avcodec_parameters_to_context(m_AVCodecContext, codecParameters) != 0) {
            LOGCATE("VideoDecoder::InitFFDecoder avcodec_parameters_to_context fail.");
            break;
        }

        AVDictionary *pAVDictionary = nullptr;
        av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);
        av_dict_set(&pAVDictionary, "stimeout", "20000000", 0);
        av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);
        av_dict_set(&pAVDictionary, "rtsp_transport", "tcp", 0);

        //8.打开解码器
        result = avcodec_open2(m_AVCodecContext, m_AVCodec, &pAVDictionary);
        if (result < 0) {
            LOGCATE("VideoDecoder::InitFFDecoder avcodec_open2 fail. result=%d", result);
            break;
        }
        result = 0;
        //创建 AVFrame 存放解码后的数据
        m_Frame = av_frame_alloc();
    } while (false);
    onDecoderReady();
    if (result == 0) {
        m_DecodeThread = new thread(decodeThreadProc, this);
    }
    return result;
}

void VideoDecoder::onDecoderReady() {
    LOGCATE("VideoDecoder::OnDecoderReady");
    m_VideoWidth = m_AVCodecContext->width;
    m_VideoHeight = m_AVCodecContext->height;

    if (m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext, MSG_DECODER_READY, 0);

    if (m_VideoRender != nullptr) {
        int dstSize[2] = {0};
        m_VideoRender->Init(m_VideoWidth, m_VideoHeight, dstSize);
        m_RenderWidth = dstSize[0];
        m_RenderHeight = dstSize[1];

        m_RGBAFrame = av_frame_alloc();
        int bufferSize = av_image_get_buffer_size(DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight,
                                                  1);
        m_FrameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(m_RGBAFrame->data, m_RGBAFrame->linesize,
                             m_FrameBuffer, DST_PIXEL_FORMAT, m_RenderWidth, m_RenderHeight, 1);

        m_SwsContext = sws_getContext(m_VideoWidth, m_VideoHeight, m_AVCodecContext->pix_fmt,
                                      m_RenderWidth, m_RenderHeight, DST_PIXEL_FORMAT,
                                      SWS_FAST_BILINEAR, NULL, NULL, NULL);
    } else {
        LOGCATE("VideoDecoder::OnDecoderReady m_VideoRender == null");
    }

}

void VideoDecoder::pause() {
    std::unique_lock <std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_PAUSE;
}

void VideoDecoder::resume() {
    std::unique_lock <std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();
}

void VideoDecoder::stop() {

    LOGCATE("VideoDecoder::Stop");
    std::unique_lock <std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_STOP;
    m_Cond.notify_all();

    if (m_DecodeThread) {
        m_DecodeThread->join();
        delete m_DecodeThread;
        m_DecodeThread = nullptr;
    }
    LOGCATE("VideoDecoder::Stop finish");

    if (m_PacketQueue) {
        m_PacketQueue->Flush();
    }
    if (m_Frame != nullptr) {
        av_frame_free(&m_Frame);
        m_Frame = nullptr;
    }

    if (m_AVCodecContext != nullptr) {
        avcodec_close(m_AVCodecContext);
        avcodec_free_context(&m_AVCodecContext);
        m_AVCodecContext = nullptr;
        m_AVCodec = nullptr;
    }
    if (m_RGBAFrame != nullptr) {
        av_frame_free(&m_RGBAFrame);
        m_RGBAFrame = nullptr;
    }

    if (m_FrameBuffer != nullptr) {
        free(m_FrameBuffer);
        m_FrameBuffer = nullptr;
    }

    if (m_SwsContext != nullptr) {
        sws_freeContext(m_SwsContext);
        m_SwsContext = nullptr;
    }
}


void VideoDecoder::decodeThreadProc(VideoDecoder *decoder) {
    decoder->dealPackQueue();
}

void VideoDecoder::dealPackQueue() {

    AVPacket *m_Packet = av_packet_alloc();
    while (m_DecoderState != STATE_STOP) {
        LOGCATE("VideoDecoder::dealPackQueue");
        while (m_DecoderState == STATE_PAUSE) {
            std::unique_lock <std::mutex> lock(m_Mutex);
            LOGCATE("VideoDecoder::DecodingLoop waiting, m_MediaType=%d");
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
            // m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
        }
//        if(m_StartTimeStamp == -1)
//            m_StartTimeStamp = GetSysCurrentTime();

        if (m_DecoderState == STATE_STOP) {
            LOGCATE("VideoDecoder::DecodingLoop stop break thread");
            goto __EXIT;
        }

        (m_PacketQueue->GetPacketSize() == 0);

        if (m_PacketQueue->GetPacket(m_Packet) < 0) {
            goto __EXIT;
        }

        if (avcodec_send_packet(m_AVCodecContext, m_Packet) == AVERROR_EOF) {
            //解码结束
            goto __EXIT;
        }
        //一个 packet 包含多少 frame?
        int frameCount = 0;
        LOGCATE("UpdateTimeStamp   VideoDecoder m_Packet %ld",m_Packet->pts);
        while (avcodec_receive_frame(m_AVCodecContext, m_Frame) >= 0) {
            //同步
            UpdateTimeStamp();
            AVSync();
            //渲染
            LOGCATE("VideoDecoder::DecodeOnePacket 000 m_MediaType=%d");

            LOGCATE("VideoDecoder::OnFrameAvailable frame=%p", m_Frame);
            if (m_VideoRender != nullptr && m_Frame != nullptr) {
                NativeImage image;
                LOGCATE("VideoDecoder::OnFrameAvailable m_Frame[w,h]=[%d, %d],format=%d,[line0,line1,line2]=[%d, %d, %d]",
                        m_Frame->width, m_Frame->height, m_AVCodecContext->pix_fmt,
                        m_Frame->linesize[0], m_Frame->linesize[1], m_Frame->linesize[2]);
                if (m_VideoRender->GetRenderType() == VIDEO_RENDER_ANWINDOW) {
                    sws_scale(m_SwsContext, m_Frame->data, m_Frame->linesize, 0,
                              m_VideoHeight, m_RGBAFrame->data, m_RGBAFrame->linesize);

                    image.format = IMAGE_FORMAT_RGBA;
                    image.width = m_RenderWidth;
                    image.height = m_RenderHeight;
                    image.ppPlane[0] = m_RGBAFrame->data[0];
                    image.pLineSize[0] = image.width * 4;
                } else if (m_AVCodecContext->pix_fmt == AV_PIX_FMT_YUV420P ||
                           m_AVCodecContext->pix_fmt == AV_PIX_FMT_YUVJ420P) {
                    image.format = IMAGE_FORMAT_I420;
                    image.width = m_Frame->width;
                    image.height = m_Frame->height;
                    image.pLineSize[0] = m_Frame->linesize[0];
                    image.pLineSize[1] = m_Frame->linesize[1];
                    image.pLineSize[2] = m_Frame->linesize[2];
                    image.ppPlane[0] = m_Frame->data[0];
                    image.ppPlane[1] = m_Frame->data[1];
                    image.ppPlane[2] = m_Frame->data[2];
                    if (m_Frame->data[0] && m_Frame->data[1] && !m_Frame->data[2] &&
                        m_Frame->linesize[0] == m_Frame->linesize[1] &&
                        m_Frame->linesize[2] == 0) {
                        // on some android device, output of h264 mediacodec decoder is NV12 兼容某些设备可能出现的格式不匹配问题
                        image.format = IMAGE_FORMAT_NV12;
                    }
                } else if (m_AVCodecContext->pix_fmt == AV_PIX_FMT_NV12) {
                    image.format = IMAGE_FORMAT_NV12;
                    image.width = m_Frame->width;
                    image.height = m_Frame->height;
                    image.pLineSize[0] = m_Frame->linesize[0];
                    image.pLineSize[1] = m_Frame->linesize[1];
                    image.ppPlane[0] = m_Frame->data[0];
                    image.ppPlane[1] = m_Frame->data[1];
                } else if (m_AVCodecContext->pix_fmt == AV_PIX_FMT_NV21) {
                    image.format = IMAGE_FORMAT_NV21;
                    image.width = m_Frame->width;
                    image.height = m_Frame->height;
                    image.pLineSize[0] = m_Frame->linesize[0];
                    image.pLineSize[1] = m_Frame->linesize[1];
                    image.ppPlane[0] = m_Frame->data[0];
                    image.ppPlane[1] = m_Frame->data[1];
                } else if (m_AVCodecContext->pix_fmt == AV_PIX_FMT_RGBA) {
                    image.format = IMAGE_FORMAT_RGBA;
                    image.width = m_Frame->width;
                    image.height = m_Frame->height;
                    image.pLineSize[0] = m_Frame->linesize[0];
                    image.ppPlane[0] = m_Frame->data[0];
                } else {
                    sws_scale(m_SwsContext, m_Frame->data, m_Frame->linesize, 0,
                              m_VideoHeight, m_RGBAFrame->data, m_RGBAFrame->linesize);
                    image.format = IMAGE_FORMAT_RGBA;
                    image.width = m_RenderWidth;
                    image.height = m_RenderHeight;
                    image.ppPlane[0] = m_RGBAFrame->data[0];
                    image.pLineSize[0] = image.width * 4;
                }

                m_VideoRender->RenderVideoFrame(&image);
            }

            if (m_MsgContext && m_MsgCallback)
                m_MsgCallback(m_MsgContext, MSG_REQUEST_RENDER, 0);

            LOGCATE("VideoDecoder::DecodeOnePacket 0001 m_MediaType=%d");
            frameCount++;
            LOGCATE("BaseDecoder::DecodeOneFrame frameCount=%d", frameCount);
        }
        av_packet_unref(m_Packet);
    }

    __EXIT:
    av_packet_unref(m_Packet);
    free(m_Packet);
    m_Packet = nullptr;
}

void VideoDecoder::UpdateTimeStamp() {

   // std::unique_lock <std::mutex> lock(m_Mutex);
    if (m_Frame->pkt_dts != AV_NOPTS_VALUE) {
        m_CurTimeStamp = m_Frame->pkt_dts;
    } else if (m_Frame->pts != AV_NOPTS_VALUE) {
        m_CurTimeStamp = m_Frame->pts;
    } else {
        m_CurTimeStamp = 0;
    }
    m_CurTimeStamp = (int64_t)(m_CurTimeStamp * timeBase * 1000);
    LOGCATE("UpdateTimeStamp   VideoDecoder %ld %ld %ld",m_Frame->pts,m_Frame->pkt_dts,m_CurTimeStamp);
    // m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
//    if(m_SeekPosition > 0 && m_SeekSuccess)
//    {
//        m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
//        m_SeekPosition = 0;
//        m_SeekSuccess = false;
//    }
}

void VideoDecoder::AVSync() {
    LOGCATE("VideoDecoder::AVSync");

    if (m_AVSyncCallback != nullptr) {
        //视频向音频同步,传进来的 m_AVSyncCallback 用于获取音频时间戳
        long elapsedTime = m_AVSyncCallback(m_AVDecoderContext);
        LOGCATE("VideoDecoder::AVSync m_CurTimeStamp=%ld, elapsedTime=%ld", m_CurTimeStamp,
                elapsedTime);

        if (m_CurTimeStamp > elapsedTime) {
            //休眠时间
            auto sleepTime = static_cast<unsigned int>(m_CurTimeStamp - elapsedTime);//ms
            av_usleep(sleepTime * 1000);
        }
    }
//    long curSysTime = GetSysCurrentTime();
//    //基于系统时钟计算从开始播放流逝的时间
//    long elapsedTime = curSysTime - m_StartTimeStamp;

//    if(m_MsgContext && m_MsgCallback)
//        m_MsgCallback(m_MsgContext, MSG_DECODING_TIME, m_CurTimeStamp * 1.0f / 1000);

//    long delay = 0;
//
//    //向系统时钟同步
//    if(m_CurTimeStamp > elapsedTime) {
//        //休眠时间
//        auto sleepTime = static_cast<unsigned int>(m_CurTimeStamp - elapsedTime);//ms
//        //限制休眠时间不能过长
//        sleepTime = sleepTime > DELAY_THRESHOLD ? DELAY_THRESHOLD :  sleepTime;
//        av_usleep(sleepTime * 1000);
//    }
//    delay = elapsedTime - m_CurTimeStamp;
}

void VideoDecoder::clearCache() {
    std::unique_lock <std::mutex> vBufLock(m_Mutex);
    m_PacketQueue->Flush();
    avcodec_flush_buffers(m_AVCodecContext);
    vBufLock.unlock();
}

