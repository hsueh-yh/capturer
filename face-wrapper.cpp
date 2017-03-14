//
//  face-wrapper.cpp
//  mtndn
//
//  Created by Peter Gusev on 2/11/14.
//  Copyright 2013-2015 Regents of the University of California
//

#define USE_TS_FACE

#include <unistd.h>
#include <ndn-cpp/threadsafe-face.hpp>
#include <ndn-cpp/security/key-chain.hpp>

#ifdef USE_TS_FACE
#include <ndn-cpp/transport/async-tcp-transport.hpp>
#include <ndn-cpp/transport/async-unix-transport.hpp>
#else
#include <ndn-cpp/transport/tcp-transport.hpp>
#include <ndn-cpp/transport/unix-transport.hpp>
#endif

#include "glogger.h"

#include "face-wrapper.h"
//#include "mtndn-object.h"
//#include "mtndn-utils.h"

using namespace ndn;

//******************************************************************************
FaceWrapper::FaceWrapper(ptr_lib::shared_ptr<Face> &face):
face_(face)
{
}

//******************************************************************************
uint64_t FaceWrapper::expressInterest(const Interest &interest,
                                      const OnData &onData,
                                      const OnTimeout &onTimeout,
                                      WireFormat &wireFormat)
{
    uint64_t iid = 0;

    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    iid = face_->expressInterest(interest, onData, onTimeout, wireFormat);

    return iid;
}

void
FaceWrapper::removePendingInterest(uint64_t interestId)
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    face_->removePendingInterest(interestId);
}

uint64_t FaceWrapper::registerPrefix(const Name &prefix,
                                     const OnInterestCallback &onInterest,
                                     const OnRegisterFailed &onRegisterFailed,
                                     const ForwardingFlags &flags,
                                     WireFormat &wireFormat)
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    return face_->registerPrefix(prefix, onInterest, onRegisterFailed,
                                flags, wireFormat);
}

void
FaceWrapper::unregisterPrefix(uint64_t prefixId)
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    face_->removeRegisteredPrefix(prefixId);
}

void
FaceWrapper::setCommandSigningInfo(KeyChain &keyChain,
                                   const Name &certificateName)
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    face_->setCommandSigningInfo(keyChain, certificateName);
}

void FaceWrapper::processEvents()
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    face_->processEvents();
}

void FaceWrapper::shutdown()
{
    ptr_lib::lock_guard<ptr_lib::recursive_mutex> scopedLock(faceMutex_);
    face_->shutdown();
}

//******************************************************************************


//******************************************************************************
//******************************************************************************
static std::string getUnixSocketFilePathForLocalhost()
{
    std::string filePath = "/var/run/nfd.sock";
    if (access(filePath.c_str(), 0/*R_OK*/) == 0)
        return filePath;
    else {
        filePath = "/tmp/.ndnd.sock";
        if (access(filePath.c_str(), 0/*R_OK*/) == 0)
            return filePath;
        else
            return "";
    }
}

static ptr_lib::shared_ptr<ndn::Transport> getDefaultTransport(boost::asio::io_service &ioService)
{
    if (getUnixSocketFilePathForLocalhost() == "")
#ifdef USE_TS_FACE
        return ptr_lib::make_shared<AsyncTcpTransport>(ioService);
#else
    return ptr_lib::make_shared<TcpTransport>();
#endif
    else
#ifdef USE_TS_FACE
        return ptr_lib::make_shared<AsyncUnixTransport>(ioService);
#else
        return ptr_lib::make_shared<UnixTransport>();
#endif
}

static ptr_lib::shared_ptr<ndn::Transport::ConnectionInfo> getDefaultConnectionInfo()
{
    std::string filePath = getUnixSocketFilePathForLocalhost();
    if (filePath == "")
#ifdef USE_TS_FACE
        return ptr_lib::make_shared<AsyncTcpTransport::ConnectionInfo>("localhost");
#else
        return ptr_lib::make_shared<TcpTransport::ConnectionInfo>("localhost");
#endif
    else
#ifdef USE_TS_FACE
        return ptr_lib::shared_ptr<AsyncUnixTransport::ConnectionInfo>
        (new AsyncUnixTransport::ConnectionInfo(filePath.c_str()));
#else
    return ptr_lib::shared_ptr<UnixTransport::ConnectionInfo>
    (new UnixTransport::ConnectionInfo(filePath.c_str()));

#endif
}

int
FaceProcessor::setupFaceAndTransport(const std::string host, const int port,
                                     ptr_lib::shared_ptr<FaceWrapper> &face,
                                     ptr_lib::shared_ptr<ndn::Transport> &transport)
{
    ptr_lib::shared_ptr<ndn::Transport::ConnectionInfo> connInfo;
    ptr_lib::shared_ptr<Face> ndnFace;

    if (host == "127.0.0.1" || host == "0.0.0.0" || host == "localhost")
    {
        transport = getDefaultTransport(MtNdnUtils::getIoService());
#ifdef USE_TS_FACE
        ndnFace.reset(new ThreadsafeFace(MtNdnUtils::getIoService(), transport, getDefaultConnectionInfo()));
#else
        ndnFace.reset(new Face(transport, getDefaultConnectionInfo()));
#endif
    }
    else
    {
#ifdef USE_TS_FACE
        connInfo.reset(new AsyncTcpTransport::ConnectionInfo(host.c_str(), port));
        transport.reset(new AsyncTcpTransport(MtNdnUtils::getIoService()));
        ndnFace.reset(new ThreadsafeFace(MtNdnUtils::getIoService(), transport, connInfo));
#else
        connInfo.reset(new TcpTransport::ConnectionInfo(host.c_str(), port));
        transport.reset(new TcpTransport());
        ndnFace.reset(new Face(transport, connInfo));
#endif
    }

    face.reset(new FaceWrapper(ndnFace));

    return RESULT_OK;
}

ptr_lib::shared_ptr<FaceProcessor>
FaceProcessor::createFaceProcessor(const std::string host, const int port,
                                   const ptr_lib::shared_ptr<ndn::KeyChain> &keyChain,
                                   const ptr_lib::shared_ptr<Name> &certificateName)
{
    ptr_lib::shared_ptr<FaceWrapper> face;
    ptr_lib::shared_ptr<ndn::Transport> transport;
    ptr_lib::shared_ptr<FaceProcessor> fp;

    //std::cout << "setup Face And Transport..." << std::endl;
    FaceProcessor::setupFaceAndTransport(host, port, face, transport);

    //std::cout << "setup keyChain..." << std::endl;
    if (keyChain.get())
    {
        if (certificateName.get())
            face->setCommandSigningInfo(*keyChain, *certificateName);
        else
            face->setCommandSigningInfo(*keyChain, keyChain->getDefaultCertificateName());
    }
    //std::cout << face.use_count() << std::endl;
    //MtNdnComponent *cp = new MtNdnComponent();
    //std::cout << "MtNdnComponent" << std::endl;
//    std::cout << "new FaceProcessor..." << std::endl;
    FaceProcessor *f = new FaceProcessor(face);
    //std::cout << "reset FaceProcessor..." << std::endl;
    fp.reset(f);
    //fp.reset(new FaceProcessor(face));
    //std::cout << "set Transport..." << std::endl;
    fp->setTransport(transport);
    //std::cout << "create FaceProcessor done." << std::endl;

    return fp;
}

//******************************************************************************
FaceProcessor::FaceProcessor(const ptr_lib::shared_ptr<FaceWrapper> &faceWrapper):
                            isProcessing_(false),
                            usecInterval_(10000),
                            faceWrapper_(faceWrapper)
{
    /*
    std::stringstream ss;
    ss << componentId_;
    setDescription(ss.str());
    setDescription("FaceProcessor");
    LOG(INFO) << getDescription() << std::endl;
    */
    //std::cout << "new FaceProcessor..." << std::endl;
}

FaceProcessor::~FaceProcessor()
{
    stopProcessing();
    faceWrapper_->shutdown();
    transport_.reset();

    std::cout << /*description_ <<*/ "Face processor dtor" << std::endl;
}

//******************************************************************************
int
FaceProcessor::startProcessing(unsigned int usecInterval)
{
    if (!isProcessing_)
    {
        usecInterval_ = usecInterval;
        isProcessing_ = true;

#ifndef USE_TS_FACE

        scheduleJob (usecInterval, [this]()->bool{
            try
            {
                faceWrapper_->processEvents();
                LOG(INFO) << "faceProcessor" << std::endl;
            }
            catch (std::exception &exception)
            {
                // do nothing
            }
            return this->isProcessing_;
        });
#endif
    }
    return RESULT_OK;
}

void
FaceProcessor::stopProcessing()
{
    if (isProcessing_)
    {
        isProcessing_ = false;
#ifndef USE_TS_FACE
        stopJob();
#endif
    }
}
