#include "capturer.h"
#include "utils.h"

using namespace std;

Capturer::Capturer():
//     out_buffer(NULL),
    videoindex(-1),
    audioindex(-1),
    devFormat("video4linux2"),
    devURL("/dev/video0"),
    backup(false)
{}

Capturer::~Capturer()
{ stop(); }

int
Capturer::init()
{
    openDevice();
    initEncoder();
}

int
Capturer::start()
{
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    //av_dict_set(); av_opt_find();

    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
        printf("Could not open codec.\n");
        return -1;
    }

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    out_buffer=(unsigned char *) av_malloc(
                av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                         pCodecCtx->width, pCodecCtx->height, 1)
                );
    if( out_buffer ==NULL )
    {
        cout << "outbuf alloc error" << endl;
        return -1;
    }
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,
                         out_buffer, AV_PIX_FMT_YUV420P,
                         pCodecCtx->width, pCodecCtx->height,1);

    if(backup)
    {
        fp_yuv=fopen("output.yuv","wb+");
        if( !fp_yuv )
        {
            printf("open file error\n");
        }
        else
            printf("open file success\n");
    }
}

int
Capturer::getFrame(unsigned char *outbuf, int &outlen, int64_t &millisecondTimestamp)
{
    return capture( outbuf, outlen, millisecondTimestamp );
}
int
Capturer::getFrame( AVFrame &frame )
{
    return capture( frame );
}

int
Capturer::stop()
{
    sws_freeContext(img_convert_ctx);

    if(backup)
        fclose(fp_yuv);

    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}


//********************************************************
bool
Capturer::openDevice()
{
    avdevice_register_all();    //Register Device

    // find and open device
    ifmt=av_find_input_format(devFormat.c_str());
    if( ifmt == NULL )
    {
        cout << "find input format error" << endl;
        return false;
    }

    pFormatCtx = avformat_alloc_context();
    if( pFormatCtx == NULL )
    {
        cout << "avformat_alloc_context error" << endl;
        return false;
    }

    if(avformat_open_input(&pFormatCtx,devURL.c_str(),ifmt,NULL)!=0){
        cout << "Couldn't open input stream linux." << endl;
        return false;
    }

    // find stream info
    if(avformat_find_stream_info(pFormatCtx,NULL)<0)
    {
        cout << "Couldn't find stream information." << endl;
        return false;
    }

    // get video stream index
    for(i=0; i<pFormatCtx->nb_streams; i++)
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            //break;
        }
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            audioindex = i;
        }
    }
    if(videoindex==-1)
    {
        cout << "Couldn't find a video stream." << endl;
        return false;
    }
}

bool
Capturer::initEncoder()
{
    av_register_all();
    avformat_network_init();

    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);

    if(pCodec==NULL)
    {
        cout << "Codec not found." << endl;
        return false;
    }

}

int
Capturer::capture( unsigned char *outbuf, int &outlen, int64_t &millisecondTimestamp )
{
    int ret, got_picture;

    if(av_read_frame(pFormatCtx, packet)>=0)
    {
        millisecondTimestamp = NdnUtils::millisecondTimestamp();
        if(packet->stream_index==videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0)
            {
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture)
            {
                sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);
                int width = pCodecCtx->width;
                int height = pCodecCtx->height;

                memset(outbuf, 0, height * width * 3 / 2);

                // reverse yuv
                ///*
                int a = 0, i;
                for (i = 0; i<height; i++)
                {
                    memcpy(outbuf + a, pFrameYUV->data[0] + i * pFrameYUV->linesize[0], width);
                    a += width;
                }
                for (i = 0; i<height / 2; i++)
                {
                    memcpy(outbuf + a, pFrameYUV->data[1] + i * pFrameYUV->linesize[1], width / 2);
                    a += width / 2;
                }
                for (i = 0; i<height / 2; i++)
                {
                    memcpy(outbuf + a, pFrameYUV->data[2] + i * pFrameYUV->linesize[2], width / 2);
                    a += width / 2;
                }
                //*/

                outlen = width*height * 3 / 2;


                if( backup)
                {
                    saveFrame(pFrameYUV, pCodecCtx->width, pCodecCtx->height);
                }

            }//if(got_picture)

        }//if(packet->stream_index==videoindex)
        else
        {
            outlen = 0;
        }

        av_packet_unref(packet);

    }//if(av_read_frame(pFormatCtx, packet)>=0)

    return 0;
}

int
Capturer::capture( AVFrame &ourFrame )
{
    int ret, got_picture;

    if(av_read_frame(pFormatCtx, packet)>=0)
    {
        if(packet->stream_index==videoindex)
        {
            ret = avcodec_decode_video2(pCodecCtx, &ourFrame, &got_picture, packet);
            if(ret < 0)
            {
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture)
            {
                if( backup)
                {
                    saveFrame(&ourFrame, pCodecCtx->width, pCodecCtx->height);
                }

            }//if(got_picture)

        }//if(packet->stream_index==videoindex)

        av_packet_unref(packet);

    }//if(av_read_frame(pFormatCtx, packet)>=0)

    return 0;
}

void
Capturer::saveFrame(AVFrame *pFrameYUV, int width, int height)
{
    if( flg)
    {
        cout << &pFrameYUV->data << " "
         << &out_buffer << " " << endl;
        flg--;
    }
    int y_size = width * height;
    fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
    fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
    fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
}
