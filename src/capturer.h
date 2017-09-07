#ifndef CAPTURER_HPP
#define CAPTURER_HPP

#include <stdio.h>
#include <iostream>
#include <string>

#include "interfaces.h"

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

#include <vector>
#include <mutex>

class FFCapturer: public IExternalCapturer
{
public:


    FFCapturer();
    ~FFCapturer();

    int init(const VideoCapturerParams &params );

    int start();

    int stop();

    void* getFrameBuf();

    int incomingYUV420Frame(void* frameObj, int64_t &captureTsMs);


    int getFrame(unsigned char *outbuf, int &outlen, int64_t &millisecondTimestamp);

    int getFrame( AVFrame &frame );


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
    std::vector<AVFrame*> avframesMap_;

    int width_, height_;

    FILE *fp_yuv;
    bool backup;

    int flg =1;
    std::recursive_mutex r_mutex_;

    //device
    AVInputFormat *ifmt;
    string devFormat;
    string devURL;

    bool openDevice();
    bool initEncoder();
    int capture(unsigned char *outbuf, int &outlen , int64_t &millisecondTimestamp);
    int capture( void* frameObj, int64_t &captureTsMs );
    void saveFrame(AVFrame *pFrameYUV, int width, int height);
};



#endif // CAPTURER_HPP
