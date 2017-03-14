//
//  face-wrapper.h
//  Thread-safe wrapper for NDN face
//

#ifndef __face_wrapper__
#define __face_wrapper__


#include <thread>
#include <mutex>
#include <ndn-cpp/face.hpp>

// use boost (ioservice) only to run threadSafeFace
#include <boost/asio/steady_timer.hpp>
#include <boost/atomic.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <chrono>

#include "mtndn-object.h"
#include "mtndn-utils.h"
#include "mtndn-defines.h"

//namespace mtndn {

using namespace ndn;

/**
  *Thread-safe wrapper for NDN face class
 */
class FaceWrapper //: public ndnlog::new_api::ILoggingObject
{
public:
    FaceWrapper(){}
    FaceWrapper(ptr_lib::shared_ptr<Face> &face_);
    ~FaceWrapper(){}

    void
    setFace(ptr_lib::shared_ptr<Face> face) { face_ = face; }
    ptr_lib::shared_ptr<Face>
    getFace() { return face_; }

    uint64_t
    expressInterest(const Interest &interest,
                    const OnData &onData,
                    const OnTimeout &onTimeout = OnTimeout(),
                    WireFormat &wireFormat = *WireFormat::getDefaultWireFormat());

    void
    removePendingInterest(uint64_t interestId);

    uint64_t
    registerPrefix(const Name &prefix,
                    const OnInterestCallback &onInterest,
                    const OnRegisterFailed &onRegisterFailed,
                    const ForwardingFlags &flags = ForwardingFlags(),
                    WireFormat &wireFormat = *WireFormat::getDefaultWireFormat());

    void
    unregisterPrefix(uint64_t prefixId);

    void
    setCommandSigningInfo(KeyChain &keyChain, const Name &certificateName);
    void
    processEvents();
    void
    shutdown();

    /**
         *Synchronizes with the face's critical section. Can be used in
         *situations when API requires calls from one thread (e.g. adding to
         *memory cache and calling processEvents should be on one thread)
        */
    void
    synchronizeStart() { faceMutex_.lock(); }
    void
    synchronizeStop() { faceMutex_.unlock(); }

private:
    ptr_lib::shared_ptr<Face> face_;
    ptr_lib::recursive_mutex faceMutex_;
};

class FaceProcessor : public MtNdnComponent
{
public:
    FaceProcessor(const ptr_lib::shared_ptr<FaceWrapper> &faceWrapper);
    ~FaceProcessor();

    int
    startProcessing(unsigned int usecInterval = 10000);

    void
    stopProcessing();

    void
    setProcessingInterval(unsigned int usecInterval)
    { usecInterval_ = usecInterval; }

    ptr_lib::shared_ptr<FaceWrapper>
    getFaceWrapper()
    { return faceWrapper_; }

    ptr_lib::shared_ptr<Transport>
    getTransport()
    { return transport_; }

    void
    setTransport(ptr_lib::shared_ptr<Transport> &transport)
    { transport_ = transport; }

    static int
    setupFaceAndTransport(const std::string host, const int port,
                            ptr_lib::shared_ptr<FaceWrapper> &face,
                            ptr_lib::shared_ptr<Transport> &transport);

    static ptr_lib::shared_ptr<FaceProcessor>
    createFaceProcessor(const std::string host, const int port,
                        const ptr_lib::shared_ptr<ndn::KeyChain> &keyChain = ptr_lib::shared_ptr<ndn::KeyChain>(),
                        const ptr_lib::shared_ptr<Name> &certificateName = ptr_lib::shared_ptr<Name>());


protected:

    ptr_lib::atomic<bool> isJobScheduled_;
    ptr_lib::recursive_mutex jobMutex_;

private:
    bool isProcessing_;
    unsigned int usecInterval_;
    ptr_lib::shared_ptr<FaceWrapper> faceWrapper_;
    ptr_lib::shared_ptr<Transport> transport_;
    ptr_lib::thread processEventsThread_;


    bool isTimerCancelled_;
};
//}

#endif /*defined(__face_wrapper__) */
