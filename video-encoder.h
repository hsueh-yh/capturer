#ifndef VIDEO_ENCODER_H_
#define VIDEO_ENCODER_H_

#include "mtndn-object.h"
#include "encoder.h"

class IRawFrameConsumer
{
public:
    virtual void onDeliverRawFrame(void *frame, int64_t captureTsMs) = 0;
};

class IEncodedFrameConsumer
{
public:
    virtual void
    onEncodedFrameDelivered(vector<uint8_t> &encodedImage,
                            int64_t captureTimestamp) = 0;
};


class VideoEncoder: public IRawFrameConsumer,
                    public FF_EncodeCompleteCallback,
                    public MtNdnComponent
{
public:
    VideoEncoder();
    virtual ~VideoEncoder();

    void init(VideoCoderParams settings);

    void onDeliverRawFrame(void *frame, int64_t captureTsMs);

    void onEncoded(const AVPacket &decodedFrame, int64_t captureTimestamp);

    void setEncodedFrameConsumer(IEncodedFrameConsumer *frameConsumer)
    { frameConsumer_ = frameConsumer; }


private:
    VideoCoderParams settings_;
    std::shared_ptr<FFEncoder> encoder_;

    IEncodedFrameConsumer *frameConsumer_ = nullptr;

    void reset();
};

#endif
