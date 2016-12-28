//
//  mtndn-object.cpp
//  mtndn
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//  Created: 8/21/13
//

#include <stdarg.h>
#include <boost/thread/lock_guard.hpp>
#define BOOST_ASIO_DISABLE_STD_CHRONO
#include <boost/chrono.hpp>


#include "mtndn-object.h"
#include "mtndn-utils.h"
#include "glogger.h"

//using namespace std;

using namespace boost;

static int componentIdCounter_ = 0;

//******************************************************************************
/**
  *@name MtNdnComponent class
 */
MtNdnComponent::MtNdnComponent(boost::asio::io_service &ioservice):
    watchdogTimer_(ioservice/*MtNdnUtils::getIoService()*/),
    isJobScheduled_(false),
    isTimerCancelled_(false),
    componentId_(componentIdCounter_++)
{}

MtNdnComponent::MtNdnComponent(IMtNdnComponentCallback *callback):
    callback_(callback),
    watchdogTimer_(MtNdnUtils::getIoService()),
    isJobScheduled_(false),
    isTimerCancelled_(false)
{}

MtNdnComponent::~MtNdnComponent()
{
    try {
        stopJob();
    }
    catch (...) {
    }
}


void
MtNdnComponent::onError(const char *errorMessage, const int errorCode)
{
    if (hasCallback())
    {
        lock_guard<mutex> scopedLock(callbackMutex_);
        callback_->onError(errorMessage, errorCode);
    }
    else
    {
        std::cout << "error" << std::endl;
        //LogErrorC << "error occurred: " << string(errorMessage) << endl;
        //if (logger_) logger_->flush();
    }
}

std::string
MtNdnComponent::getDescription() const
{
    if (description_ == "")
    {
        std::stringstream ss;
        ss << "MtNdnObject"
           /*<< std::hex << this*/;
        return ss.str();
    }

    return description_;
}

thread
MtNdnComponent::startThread(boost::function<bool ()> threadFunc)
{
    thread threadObject = thread([threadFunc](){
        bool result = false;
        do {
            try
            {
                this_thread::interruption_point();
                {
                    this_thread::disable_interruption di;
                    result = threadFunc();
                }
            }
            catch (thread_interrupted &interruption)
            {
                result = false;
            }
        } while (result);
    });
    
    return threadObject;
}

void
MtNdnComponent::stopThread(thread &threadObject)
{
    threadObject.interrupt();
    
    if (threadObject.joinable())
    {
        bool res = threadObject.try_join_for(boost::chrono::milliseconds(500));
                                             
        if (!res)
            threadObject.detach();
    }
}


void
MtNdnComponent::scheduleJob(const unsigned int usecInterval,
                                  boost::function<bool()> jobCallback)
{
    boost::lock_guard<boost::recursive_mutex> scopedLock(this->jobMutex_);

    int64_t startTs = MtNdnUtils::microsecondTimestamp();
    VLOG(LOG_WARN) << getDescription() << " start job " << startTs << std::endl;
    watchdogTimer_.expires_from_now(boost::chrono::microseconds(usecInterval));
    isJobScheduled_ = true;
    isTimerCancelled_ = false;
    
    watchdogTimer_.async_wait([this, startTs, usecInterval, jobCallback](const boost::system::error_code &code){
        if (code != boost::asio::error::operation_aborted)
        {
            if (!isTimerCancelled_)
            {
                isJobScheduled_ = false;
                boost::lock_guard<boost::recursive_mutex> scopedLock(this->jobMutex_);
                bool res = jobCallback();
                int64_t cmpltTs = MtNdnUtils::microsecondTimestamp();
                VLOG(LOG_WARN) << getDescription() << " cmplt job " << cmpltTs << " taking " << cmpltTs-startTs << std::endl;
                if (res)
                    this->scheduleJob(usecInterval, jobCallback);
            }
        }
    });

}

void
MtNdnComponent::stopJob()
{
    MtNdnUtils::performOnBackgroundThread([this]()->void{
        boost::lock_guard<boost::recursive_mutex> scopedLock(jobMutex_);
        watchdogTimer_.cancel();
        isTimerCancelled_ = true;
    });
    
    isJobScheduled_ = false;
}

