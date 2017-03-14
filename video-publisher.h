#ifndef VIDEOPUBLISHER_H
#define VIDEOPUBLISHER_H

#include "publisher.h"
#include "video-capturer.h"


class VideoPublisher: public Publisher,
                      public IEncodedFrameConsumer
        //: public ptr_lib::enable_shared_from_this<VideoPublisher>
{
public:

    class VideoRequest
    {
    public:
        VideoRequest() :
            isValid(false), requestCounter_(0)
        {}

        void newRequest( const ptr_lib::shared_ptr<const Interest>& interest )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            ++requestCounter_;
            startMsTs = MtNdnUtils::millisecSinceEpoch();
            finishTime = interest->getInterestLifetimeMilliseconds() + startMsTs;
        }

        bool isvalid()
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            return isValid;
        }
        bool setvalide( bool valid )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            isValid = valid;
        }

        int64_t getstartTsMs()
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            return startMsTs;
        }

        void setstartTsMs( int64_t ts )
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            startMsTs = ts;
        }

        int64_t getFinishTsMs()
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            return finishTime;
        }
        void setFinishTsMs(int64_t ts)
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            finishTime = ts;
        }

        int getRequestCount()
        {
            ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(syncMutex_);
            return requestCounter_;
        }

    private:
        bool isValid;
        int64_t startMsTs;
        int64_t finishTime;
        int requestCounter_;

        ptr_lib::recursive_mutex syncMutex_;
    };

    VideoPublisher(const GeneralParams &generalParams,
                    IExternalCapturer* const externalCapturer);

    ~VideoPublisher ();

    int
    init(const PublisherSettings &settings, const MediaThreadParams* videoThreadParams);

    int
    start();

    int
    stop();

    void
    onEncodedFrameDelivered(vector<uint8_t> &encodedImage,
                            int64_t captureTimestamp);

    void
    onInterest( const ptr_lib::shared_ptr<const Name>& prefix,
                const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
                uint64_t interestFilterId,
                const ptr_lib::shared_ptr<const InterestFilter>& filter);

protected:

    ptr_lib::shared_ptr<VideoCapturer> videoCapturer_;
    ptr_lib::shared_ptr<VideoEncoder> videoEncoder_;

    VideoThreadParams videoThreadParams_;

    VideoRequest req_;

    /////////////////////////////////////////////////////////////////
    void
    processRequest(const ptr_lib::shared_ptr<const Name>& prefix,
                    const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
                    uint64_t interestFilterId,
                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

    void
    processRequest1(const ptr_lib::shared_ptr<const Name>& prefix,
                    const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
                    uint64_t interestFilterId,
                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

};


#endif // VIDEOPUBLISHER_H
