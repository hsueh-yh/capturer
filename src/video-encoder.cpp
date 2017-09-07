#include "video-encoder.h"
#include "glogger.h"

VideoEncoder::VideoEncoder():
    ff_encoder_(new FFEncoder)
{}

VideoEncoder::~VideoEncoder()
{}

void
VideoEncoder::init(const VideoCoderParams& settings)
{
    reset(settings);
}

void
VideoEncoder::onDeliverRawFrame(void *frame, int64_t captureTsMs)
{
    VLOG(LOG_TRACE) << "[VideoEncoder]\tonDeliverRawFrame" << std::endl;
    //std::cout << "VideoEncoder onDeliverRawFrame" << std::endl;
    ff_encoder_->encode(frame, captureTsMs);
}

void
VideoEncoder::onEncoded(const AVPacket &decodedFrame, int64_t captureTimestamp)
{
    if( frameConsumer_ )
    {
        VLOG(LOG_TRACE) << "[VideoEncoder]\tonEncoded" << std::endl;
        vector<uint8_t> frame(decodedFrame.data,decodedFrame.data+decodedFrame.size);
        frameConsumer_->onEncodedFrameDelivered(frame, captureTimestamp);
    }
}

void
VideoEncoder::reset(const VideoCoderParams &settings)
{
    settings_ = settings;
    if( ff_encoder_.get() )
    {
        ff_encoder_->RegisterEncodeCompleteCallback(this);
        ff_encoder_->init(settings_);
    }

    VLOG(LOG_TRACE) << "VideoEncoder Settings: " << endl
                   << settings_ << endl;
}
