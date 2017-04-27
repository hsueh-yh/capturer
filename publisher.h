#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <boost/asio.hpp>
#include <ndn-cpp/threadsafe-face.hpp>

#include "common.h"
#include "name-components.h"
#include "capturer.h"
#include "encoder.h"
#include "frame-buffer.h"
#include "face-wrapper.h"
#include "video-encoder.h"


using namespace ndn;

class FrameBuffer;

class PublisherSettings
{
public:
    std::string streamPrefix_;
    MediaStreamParams streamParams_;
    ptr_lib::shared_ptr<FaceProcessor> faceProcessor_;
};


class Publisher : public MtNdnComponent
        //: public ptr_lib::enable_shared_from_this<Publisher>
{
public:

    Publisher( const GeneralParams &generalParams );
    virtual ~Publisher ();


    virtual int
    init(const PublisherSettings &settings, const MediaThreadParams* videoThreadParams);

    virtual int
    start();

    virtual int
    stop();

    ptr_lib::shared_ptr<FaceWrapper>
    getFace()
    { return face_; }

    const std::string&
    getStreamName()
    { return settings_.streamPrefix_; }

    bool
    isCached( FrameNumber frameNo )
    { return (frameNo >= cachedBegin_ && frameNo <= cachedEnd_) ? true : false; }


    //////////////////////////////////////////////////////////////
    void
    onInterest( const ptr_lib::shared_ptr<const Name>& prefix,
                const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
                uint64_t interestFilterId,
                const ptr_lib::shared_ptr<const InterestFilter>& filter);
    // onInterest.
//    void operator()
//                    (const ptr_lib::shared_ptr<const Name>& prefix,
//                    const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
//                    uint64_t interestFilterId,
//                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

    virtual void
    onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix);

    virtual void
    onRegisterSuccess(const ptr_lib::shared_ptr<const Name>& prefix,
                           uint64_t registeredPrefixId);


protected:

    GeneralParams generalParams_;
    PublisherSettings settings_;

    ptr_lib::shared_ptr<FaceWrapper> face_;
    ptr_lib::shared_ptr<FrameBuffer> frameBuffer_;

    uint64_t    registedId_;

    FrameNumber cacheSize_,
                cachedBegin_,
                cachedEnd_,
                currentFrameNo_;
    int         requestCounter_,
                responseCount_;

    bool isBackupYUV, isBackup264;
    FILE *fp_yuv, *fp_264;


    virtual void processInterest(
            const ptr_lib::shared_ptr<const Name>& prefix,
            const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
            uint64_t interestFilterId,
            const ptr_lib::shared_ptr<const InterestFilter>& filter) = 0;

};


#endif // PUBLISHER_H
