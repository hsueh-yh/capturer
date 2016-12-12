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
    Encoder():
        backup(0),
        frameNo_(-1)
    {}
    ~Encoder()
    { stop(); }

    int init(AVCodecID codec_id)
    {
        av_register_all();

        /* find the h264 video encoder */
        codec = avcodec_find_encoder(codec_id);
        if (!codec) {
            fprintf(stderr, "Codec not found\n");
            exit(1);
        }

        codeCtx = avcodec_alloc_context3(codec);
        if (!codeCtx) {
            fprintf(stderr, "Could not allocate video codec context\n");
            exit(1);
        }

        /* put sample parameters */
        codeCtx->bit_rate = 600000;
        /* resolution must be a multiple of two */
        codeCtx->width = 640;
        codeCtx->height = 480;
        /* frames per second */
        codeCtx->time_base = (AVRational){1,25};
        /* emit one intra frame every ten frames
         * check frame pict_type before passing frame
         * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
         * then gop_size is ignored and the output of encoder
         * will always be I frame irrespective to gop_size
         */
        codeCtx->gop_size = 10;
        //codeCtx->max_b_frames = 0;
        codeCtx->has_b_frames = 0; // without B frame
        codeCtx->pix_fmt = AV_PIX_FMT_YUV420P;

        if (codec_id == AV_CODEC_ID_H264)
            av_opt_set(codeCtx->priv_data, "preset", "slow", 0);

        /* open it */
        if (avcodec_open2(codeCtx, codec, NULL) < 0) {
            fprintf(stderr, "Could not open codec\n");
            exit(1);
        }

        if( backup )
        {
            file = fopen(filename, "wb");
            if (!file)
            {
                fprintf(stderr, "Could not open %s\n", filename);
                exit(1);
            }
        }


        frameYUV = av_frame_alloc();
        if (!frameYUV) {
            fprintf(stderr, "Could not allocate video frame\n");
            exit(1);
        }
        frameYUV->format = codeCtx->pix_fmt;
        frameYUV->width = codeCtx->width;
        frameYUV->height = codeCtx->height;

        ret = av_image_alloc(frameYUV->data, frameYUV->linesize,
                             codeCtx->width, codeCtx->height,
                             codeCtx->pix_fmt, 32);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw picture buffer\n");
            exit(1);
        }

    }

    int getFrame( unsigned char* inbuf, int inlen, unsigned char* outbuf, int &outlen )
    {
        return encode( inbuf, inlen, outbuf, outlen );
    }
    int getFrame( AVFrame &frame, unsigned char* outbuf, int &outlen )
    {
        return encode( frame, outbuf, outlen );
    }

    DataBlock* getExtradata()
    {
        if( codeCtx->extradata_size > 0 )
        {
            return new DataBlock(codeCtx->extradata,codeCtx->extradata_size);
        }
    }

    void stop()
    {
        if( backup )
            fclose(file);
        avcodec_close(codeCtx);
        av_free(codeCtx);
        av_freep(&frameYUV->data[0]);
        av_packet_unref(&avpkt);
    }

    int getEncodeHight()
    { return height; }

    int getEncodeWidth()
    { return width; }

    double getPacketRate()
    { return (double)(codeCtx->framerate.num)/(double)(codeCtx->framerate.den); }


private:

    int encode( unsigned char *inbuf, int inlen, unsigned char* outbuf, int &outlen )
    {
        av_init_packet(&avpkt);
        avpkt.data = NULL;    // packet data will be allocated by the encoder
        avpkt.size = 0;


        int width = codeCtx->width;
        int height = codeCtx->height;

        int a = 0, i;
        for (i = 0; i<height; i++)
        {
            memcpy(frameYUV->data[0] + i * frameYUV->linesize[0], inbuf + a, width);
            a += width;
        }
        for (i = 0; i<height / 2; i++)
        {
            memcpy(frameYUV->data[1] + i * frameYUV->linesize[1], inbuf + a, width / 2);
            a += width / 2;
        }
        for (i = 0; i<height / 2; i++)
        {
            memcpy(frameYUV->data[2] + i * frameYUV->linesize[2], inbuf + a, width / 2);
            a += width / 2;
        }

        frameYUV->pts = ++frameNo_;

        ret = avcodec_encode_video2(codeCtx, &avpkt, frameYUV, &got_output);

        if (ret < 0) {
            cout << ret << endl;
            fprintf(stderr, "Error encoding frame\n");
            //exit(1);
        }

        if (got_output) {
            //printf("Write frame %3d (size=%5d)\n", frameNo, avpkt.size);
            outlen = avpkt.size;
            memcpy(outbuf,avpkt.data,outlen);
            if(backup)
                fwrite(avpkt.data, 1, avpkt.size, file);
            av_packet_unref(&avpkt);
        }

        //flushEncoder();
    }

    int encode( AVFrame &frame, unsigned char* outbuf, int &outlen )
    {
        av_init_packet(&avpkt);
        avpkt.data = NULL;    // packet data will be allocated by the encoder
        avpkt.size = 0;

        ret = avcodec_encode_video2(codeCtx, &avpkt, &frame, &got_output);

        if (ret < 0) {
            cout << ret << endl;
            fprintf(stderr, "Error encoding frame\n");
            //exit(1);
        }

        if (got_output) {
            //printf("Write frame %3d (size=%5d)\n", frameNo, avpkt.size);
            outlen = avpkt.size;
            memcpy(outbuf,avpkt.data,outlen);
            if(backup)
                fwrite(avpkt.data, 1, avpkt.size, file);
            av_packet_unref(&avpkt);
        }

        //flushEncoder();
    }

    void flushEncoder()
    {
        for (got_output = 1; got_output; i++)
        {
            fflush(stdout);

            ret = avcodec_encode_video2(codeCtx, &avpkt, NULL, &got_output);
            if (ret < 0)
            {
                fprintf(stderr, "Error encoding frame\n");
                exit(1);
            }

            if (got_output)
            {
                printf("Write frame %3d (size=%5d)\n", i, avpkt.size);
                fwrite(avpkt.data, 1, avpkt.size, file);
            }
        }
    }

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
