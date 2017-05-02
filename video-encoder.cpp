#include "video-encoder.h"
#include "glogger.h"

VideoEncoder::VideoEncoder():
    encoder_(new FFEncoder)
{}

VideoEncoder::~VideoEncoder()
{}

void
VideoEncoder::init(VideoCoderParams settings)
{
    reset();
    settings_ = settings;
}

void
VideoEncoder::onDeliverRawFrame(void *frame, int64_t captureTsMs)
{
    //std::cout << "VideoEncoder onDeliverRawFrame" << std::endl;
    encoder_->encode(frame, captureTsMs);
}

void
VideoEncoder::onEncoded(const AVPacket &decodedFrame, int64_t captureTimestamp)
{
    if( frameConsumer_ )
    {
        //LOG(INFO) << "VideoEncoder onEncoded" << std::endl;
        vector<uint8_t> frame(decodedFrame.data,decodedFrame.data+decodedFrame.size);
        frameConsumer_->onEncodedFrameDelivered(frame, captureTimestamp);
    }
}

void
VideoEncoder::reset()
{
    if( encoder_.get() )
    {
        encoder_->RegisterEncodeCompleteCallback(this);
        encoder_->init();
    }
}
