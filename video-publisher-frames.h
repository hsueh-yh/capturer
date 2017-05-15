#ifndef VIDEOPUBLISHER_H
#define VIDEOPUBLISHER_H

#include "publisher.h"
#include "video-capturer.h"


class VideoPublisherFrames: public Publisher,
                      public IEncodedFrameConsumer
        //: public ptr_lib::enable_shared_from_this<VideoPublisher>
{
public:

    VideoPublisherFrames(const GeneralParams &generalParams,
                    IExternalCapturer* const externalCapturer);

    ~VideoPublisherFrames();

    int
    init(const PublisherSettings &settings/*, const MediaThreadParams* videoThreadParams*/);

    int
    start();

    int
    stop();

    void
    onEncodedFrameDelivered(vector<uint8_t> &encodedImage,
                            int64_t captureTimestamp);

protected:

    ptr_lib::shared_ptr<VideoCapturer> videoCapturer_;
    ptr_lib::shared_ptr<VideoEncoder> videoEncoder_;

    VideoThreadParams videoThreadParams_;

    /////////////////////////////////////////////////////////////////
    void
    processInterest(const ptr_lib::shared_ptr<const Name>& prefix,
                    const ptr_lib::shared_ptr<const Interest>& interest,
                    Face& face, uint64_t interestFilterId,
                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

};


#endif // VIDEOPUBLISHER_H
