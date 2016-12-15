#ifndef CAPTURER_HPP
#define CAPTURER_HPP

#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;


//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif


class Capturer
{
public:


    Capturer();
    ~Capturer();

    int init();

    int start();

    int getFrame(unsigned char *outbuf, int &outlen, int64_t &millisecondTimestamp);

    int getFrame( AVFrame &frame );

    int stop();


//private:

    AVFormatContext	*pFormatCtx;
    int				i, videoindex, audioindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;

    AVFrame	*pFrame;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;

    unsigned char *out_buffer;
    AVFrame *pFrameYUV;


    FILE *fp_yuv;
    bool backup;

    int flg =1;


    //device
    AVInputFormat *ifmt;
    string devFormat;
    string devURL;

    bool openDevice();
    bool initEncoder();
    int capture(unsigned char *outbuf, int &outlen , int64_t &millisecondTimestamp);
    int capture( AVFrame &ourFrame );
    void saveFrame(AVFrame *pFrameYUV, int width, int height);
};



#endif // CAPTURER_HPP
