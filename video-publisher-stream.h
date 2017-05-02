#ifndef VideoPublisherStream_H
#define VideoPublisherStream_H

#include "publisher.h"
#include "video-capturer.h"


class VideoPublisherStream: public Publisher,
                      public IEncodedFrameConsumer
        //: public ptr_lib::enable_shared_from_this<VideoPublisherStream>
{
public:

    class VideoRequest
    {
    public:
        VideoRequest() :
            state(false), requestCounter_(0)
        {}

        void
        newRequest( const ptr_lib::shared_ptr<const Interest>& interest )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            ++requestCounter_;
            startMsTs = MtNdnUtils::millisecSinceEpoch();
            finishTime = interest->getInterestLifetimeMilliseconds() + startMsTs;
        }

        bool
        isStreaming()
        {
            return state;
        }

        bool
        setState( bool valid )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            state = valid;
        }

        int64_t
        getstartTsMs()
        {
            return startMsTs;
        }

        void
        setstartTsMs( int64_t ts )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            startMsTs = ts;
        }

        int64_t
        getFinishTsMs()
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            return finishTime;
        }

        void
        setFinishTsMs(int64_t ts)
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            finishTime = ts;
        }

        int
        getRequestCount()
        {
            return requestCounter_;
        }

    private:
        bool state;
        int64_t startMsTs;
        int64_t finishTime;
        int requestCounter_;

        ptr_lib::recursive_mutex syncMutex_;
    };

    VideoPublisherStream(const GeneralParams &generalParams,
                    IExternalCapturer* const externalCapturer);

    ~VideoPublisherStream ();

    int
    init(const PublisherSettings &settings, const MediaThreadParams* videoThreadParams);

    int
    start();

    int
    stop();

    void onEncodedFrameDelivered(vector<uint8_t> &encodedImage,
                            int64_t captureTimestamp);

protected:

    ptr_lib::shared_ptr<VideoCapturer> videoCapturer_;
    ptr_lib::shared_ptr<VideoEncoder> videoEncoder_;

    VideoThreadParams videoThreadParams_;

    VideoRequest req_;

    /////////////////////////////////////////////////////////////////
    void
    processInterest(const ptr_lib::shared_ptr<const Name>& prefix,
                    const ptr_lib::shared_ptr<const Interest>& interest,
                    Face& face, uint64_t interestFilterId,
                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

private:
    void pushFrame(const Name &reqName );

};


#endif // VideoPublisherStream_H
