#include <cstring>
#include <cstdlib>
#include <LogUtil.h>
#include <unistd.h>
#include "RTMPPush.h"

RTMPPush::RTMPPush() {
    //拿到 RTMP 实例
    this->mRtmp = RTMP_Alloc();
}

RTMPPush::~RTMPPush() {
    if (!mRtmp)
        return;

    RTMP_Free(mRtmp);
    mRtmp = nullptr;
}

void RTMPPush::start(const char *url, ConnectCallBack callBack) {
    isConnect = false;
    strcpy(mRtmpUrl, url);
    //rtmp init
    RTMP_Init(mRtmp);
    //设置地址
    int ret = RTMP_SetupURL(mRtmp, mRtmpUrl);
    if (!ret) {
        LOGCATE("RTMPPush::start  失败");
        callBack(ret);
        return;
    }
    LOGCATE("RTMPPush::start  result is 成功");
    //设置超时时间
    mRtmp->Link.timeout = 3;
    RTMP_EnableWrite(mRtmp);
    new thread(connect, callBack, this);

}

void RTMPPush::connect(const ConnectCallBack &callBack, RTMPPush *push) {
    //开始链接
    int ret = RTMP_Connect(push->mRtmp, 0);
    if (!ret) {
        LOGCATE("RTMPPush::connect result is 失败");
        callBack(ret);
        return;
    } else {
        LOGCATE("RTMPPush::connect  result is 成功");
    }
    //seek 到某一处
    ret = RTMP_ConnectStream(push->mRtmp, 0);
    if (!ret) {
        //  callBack(ret);
        // return;
    }
    LOGCATE("RTMPPush::connect  RTMP_ConnectStream ret %d", ret);
    //记录一个开始时间
    push->mStartTime = RTMP_GetTime();
    push->isConnect = true;
    push->m_Exit = false;
    push->pushThread = new thread(startThread, push);
    callBack(1);
}


void RTMPPush::startThread(RTMPPush *push) {
    push->loopPush();
}

void RTMPPush::loopPush() {

    while (!m_Exit) {

        while (rtmpPackQueue.Empty() && !m_Exit) {
            usleep(10 * 1000);
        }

        RTMPPacket *packet = rtmpPackQueue.Pop();
        if (packet == nullptr) {
            continue;
        }
        if (mRtmp == nullptr) {
            return;
        }
        int result = RTMP_SendPacket(mRtmp, packet, 1);
        if (!result) {
            LOGCATE("RTMPPush::loopPush result is  失败 %d", result);
        } else {
            LOGCATE("RTMPPush::loopPush result is 成功 %d", result);
        }

        RTMPPacket_Free(packet);
        free(packet);
        packet = NULL;
    }
}

void RTMPPush::stop() {
    RTMP_Close(mRtmp);
    isConnect = false;
    m_Exit = true;

    if (pushThread) {
        pushThread->join();
        delete pushThread;
        pushThread = nullptr;
    }
    while (!rtmpPackQueue.Empty()) {
        RTMPPacket *packet = rtmpPackQueue.Pop();
        if(packet!= nullptr){
            RTMPPacket_Free(packet);
            free(packet);
            packet = NULL;
        }
    }
}

void RTMPPush::pushSpsPps(uint8_t *sps, int sps_len, uint8_t *pps, int pps_len) {

    int bodysize = sps_len + pps_len + 16;
    RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(packet, bodysize);
    RTMPPacket_Reset(packet);

    char *body = packet->m_body;

    int i = 0;

    body[i++] = 0x17; //帧类型, 1 字节

    body[i++] = 0x00; //数据类型, 1 字节

    //00 00 00 合成时间, 3 字节
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    // 版本信息, 1 字节
    body[i++] = 0x01;

    //64 00 32 编码规则, 3 字节
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];

    body[i++] = 0xFF;//NALU 长度, 1 字节

    body[i++] = 0xE1; //sps 个数

    //00 19 SPS 长度, 2 字节
    body[i++] = (sps_len >> 8) & 0xff; //取高八位写入地址
    body[i++] = sps_len & 0xff; //取低八位写入高地址中

    //sps 数据部分
    memcpy(&body[i], sps, sps_len);

    i += sps_len;

    body[i++] = 0x01;
    body[i++] = (pps_len >> 8) & 0xff;
    body[i++] = pps_len & 0xff;
    memcpy(&body[i], pps, pps_len);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodysize;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = mRtmp->m_stream_id;

    rtmpPackQueue.Push(packet);

}

void RTMPPush::pushVideoData(uint8_t *video, int len, int type) {

    if(type==7 || type==8){
        return;
    }
    int data_len = len;
    int bodysize = data_len + 9;
    RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(packet, bodysize);
    RTMPPacket_Reset(packet);

    char *body = packet->m_body;
    int i = 0;

    // 帧类型数据 : 分为两部分;
    // 前 4 位表示帧类型, 1 表示关键帧, 2 表示普通帧
    // 后 4 位表示编码类型, 7 表示 AVC 视频编码

    if (type==5) {
        body[i++] = 0x17;
    } else {
        body[i++] = 0x27;
    }//1

    // 数据类型, 00 表示 AVC 序列头
    body[i++] = 0x01; //2

    // 合成时间, 一般设置 00 00 00
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;  //5

   // 长度
    body[i++] = (data_len >> 24) & 0xff;
    body[i++] = (data_len >> 16) & 0xff;
    body[i++] = (data_len >> 8) & 0xff;
    body[i++] = data_len & 0xff;     //9位


    memcpy(&body[i], video, data_len); //数据部分

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodysize;
    packet->m_nTimeStamp = RTMP_GetTime() - mStartTime;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = mRtmp->m_stream_id;


    rtmpPackQueue.Push(packet);
}

void RTMPPush::pushAudioData(uint8_t *audio, int len, int type) {


    int data_len = len;
    int bodysize = data_len + 2;
    RTMPPacket *packet = static_cast<RTMPPacket *>(malloc(sizeof(RTMPPacket)));
    RTMPPacket_Alloc(packet, bodysize);
    RTMPPacket_Reset(packet);
    char *body = packet->m_body;

    body[0] = 0xAF;
    body[1] = 0x01;

    memcpy(&body[2], audio, data_len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = bodysize;
    packet->m_nTimeStamp = RTMP_GetTime() - mStartTime;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nChannel = 0x05;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = mRtmp->m_stream_id;

    rtmpPackQueue.Push(packet);

}