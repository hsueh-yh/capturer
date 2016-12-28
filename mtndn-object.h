//
//  mtndn-mtndn-object.h
//  mtndn
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev 
//  Created: 8/21/13
//

#define BOOST_THREAD_USE_LIB

#ifndef _object__
#define _object__

#include <boost/thread.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sstream>

#include "mtndn-utils.h"
#include "simple-log.h"


class IMtNdnComponentCallback {
public:
    virtual ~IMtNdnComponentCallback(){}
    virtual void onError(const char *errorMessage,
                            const int errorCode = -1) = 0;
};
        
class MtNdnComponent :
        public ndnlog::new_api::ILoggingObject,
        public IMtNdnComponentCallback,
        public boost::enable_shared_from_this<MtNdnComponent>
{
public:
    // construction/desctruction
    MtNdnComponent(boost::asio::io_service &ioservice = MtNdnUtils::getIoService());

    MtNdnComponent(IMtNdnComponentCallback *callback);

    virtual ~MtNdnComponent();


    virtual void registerCallback(IMtNdnComponentCallback *callback)
    { callback_ = callback; }

    virtual void deregisterCallback()
    { callback_ = NULL; }

    //////////////////////////////////////////////////////////////////////
    // IMtNdnComponentCallback
    virtual void onError(const char *errorMessage, const int errorCode);

    //////////////////////////////////////////////////////////////////////
    // ILoggingObject

    virtual std::string
    getDescription() const;

    virtual bool
    isLoggingEnabled() const
    { return true; }

protected:
    boost::atomic<bool> isJobScheduled_;
    boost::mutex callbackMutex_;
    boost::recursive_mutex jobMutex_;
    IMtNdnComponentCallback *callback_ = nullptr;

    int componentId_;

    // protected methods go here
    bool hasCallback() { return callback_ != NULL; }
            
    boost::thread
    startThread(boost::function<bool()> threadFunc);
    void
    stopThread(boost::thread &thread);

    void
    scheduleJob(const unsigned int usecInterval,
                boost::function<bool()> jobCallback);
    void
    stopJob();

private:
    bool isTimerCancelled_;
    boost::asio::steady_timer watchdogTimer_;
};

#endif /*defined(_object__)*/
