#ifndef ENCODER_HPP
#define ENCODER_HPP


#include <iostream>
#include "frame-data.h"
#include "interfaces.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif

using namespace std;


class FF_EncodeCompleteCallback
{
public:
    virtual void onEncoded(const AVPacket &decodedFrame, int64_t captureTimestamp) = 0;
};

class FFEncoder
{
public:
    FFEncoder();
    ~FFEncoder();

    int init(VideoCoderParams& params, AVCodecID codec_id = AV_CODEC_ID_H264);

    void RegisterEncodeCompleteCallback(FF_EncodeCompleteCallback *callback)
    { callback_ = callback; }

    int getFrame( unsigned char* inbuf, int inlen, unsigned char* outbuf, int &outlen );

    int getFrame( AVFrame &frame, unsigned char* outbuf, int &outlen );

    DataBlock* getExtradata();

    void stop();

    int getEncodeHight();

    int getEncodeWidth();

    double getPacketRate();

    void onDeliverFrame(const AVFrame &frame);

    int encode(void *pframe, int64_t captureTimestamp);

private:

    int encode( unsigned char *inbuf, int inlen, unsigned char* outbuf, int &outlen );

    void flushEncoder();

    AVCodec *codec;
    AVCodecContext *codeCtx= NULL;
    AVFrame *frameYUV;
    AVPacket avpkt;
    int i, ret, x, y, got_output;
    FILE *file;
    //AVFrame *frame;
    //AVPacket avpkt;

    int width = 640, height = 480;

    const char* filename = "output-encoder.264";
    int backup;

    unsigned int frameNo_;

    FF_EncodeCompleteCallback *callback_ = nullptr;

};


#endif // ENCODER_HPP
