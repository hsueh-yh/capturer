#ifndef ENCODER_HPP
#define ENCODER_HPP


#include <iostream>
#include "frame-data.h"

extern "C"
{
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}

using namespace std;

class Encoder
{
public:
    Encoder();
    ~Encoder();

    int init(AVCodecID codec_id);

    int getFrame( unsigned char* inbuf, int inlen, unsigned char* outbuf, int &outlen );

    int getFrame( AVFrame &frame, unsigned char* outbuf, int &outlen );

    DataBlock* getExtradata();

    void stop();

    int getEncodeHight();

    int getEncodeWidth();

    double getPacketRate();


private:

    int encode( unsigned char *inbuf, int inlen, unsigned char* outbuf, int &outlen );

    int encode( AVFrame &frame, unsigned char* outbuf, int &outlen );

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

};


#endif // ENCODER_HPP
