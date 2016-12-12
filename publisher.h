#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <boost/asio.hpp>
#include <ndn-cpp/threadsafe-face.hpp>

#include "common.h"
#include "name-components.h"
#include "capturer.hpp"
#include "encoder.hpp"
#include "frame-buffer.h"

using namespace ndn;

class FrameBuffer;

class Publisher //: public ptr_lib::enable_shared_from_this<Publisher>
{
public:

    Publisher (boost::asio::io_service& ioService,
                ptr_lib::shared_ptr<ThreadsafeFace> face,
                KeyChain &keyChain, const Name& certificateName );

    ~Publisher ();

    bool init ();

    int start();

    int stop();

    int getStatus()
    { return stat; }

    bool isCached( FrameNumber frameNo )
    { return (frameNo >= cachedBegin_ && frameNo <= cachedEnd_) ? true : false; }

    void onInterest(const ptr_lib::shared_ptr<const Name>& prefix,
                    const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
                    uint64_t interestFilterId,
                    const ptr_lib::shared_ptr<const InterestFilter>& filter);
    // onInterest.
//    void operator()
//                    (const ptr_lib::shared_ptr<const Name>& prefix,
//                    const ptr_lib::shared_ptr<const Interest>& interest, Face& face,
//                    uint64_t interestFilterId,
//                    const ptr_lib::shared_ptr<const InterestFilter>& filter);

    void onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix);
    // onRegisterFailed.
    //void operator()(const ptr_lib::shared_ptr<const Name>& prefix);
    void onRegisterSuccess(const ptr_lib::shared_ptr<const Name>& prefix,
                           uint64_t registeredPrefixId);


    ndn::Name getStreamVideoPrefix()
    { return streamPrefix_.append(NameComponents::NameComponentStreamFrameVideo); }

    ndn::Name getStreamPrefix()
    { return streamPrefix_; }

    std::string getLocation()
    { return location_; }

    int getStreamIdx()
    { return stream_; }


private:

    void excuteCapture();

    void view();

    ndn::Name       streamPrefix_;
    std::string     location_;
    int             stream_;

    KeyChain        keyChain_;
    Name            certificateName_;
    ptr_lib::shared_ptr<ThreadsafeFace>       face_;
    boost::asio::io_service& ioService_;
    uint64_t        registedId_;

    int             requestCounter_,
                    responseCount_;

    int stat;

    ptr_lib::shared_ptr<FrameBuffer> frameBuffer_;

    Capturer capturer;
    Encoder encoder;

    int width = 640, height = 480;

    unsigned char* outbufYUV = NULL;
    int outlenYUV = 0;
    unsigned char* outbuf264 = NULL;
    int outlen264 = 0;

    AVFrame *frame;

    bool isBackupYUV, isBackup264;
    FILE *fp_yuv, *fp_264;

    uint64_t    cacheSize_,
                cachedBegin_,
                cachedEnd_;

    FrameNumber currentFrameNo_;
};


#endif // PUBLISHER_H
