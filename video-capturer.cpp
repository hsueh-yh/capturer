#include "video-capturer.h"

#include "glogger.h"

VideoCapturer::VideoCapturer(IExternalCapturer *externalCapturer):
    externalCapturer_(externalCapturer)
{}

VideoCapturer::~VideoCapturer()
{}

void
VideoCapturer::init(const VideoCapturerParams &params)
{
    //externalCapturer_.capturingStarted();
    //ff_capturer_.init();
    captureFrequncy_ = params.frameRate_;
    externalCapturer_->init(params);

    capturedFrameObj_ = externalCapturer_->getFrameBuf();
    //capturedFrame_ = av_frame_alloc();
}

void
VideoCapturer::start()
{
    LOG(INFO) << "VideoCapturer start" << std::endl;
//    captureThread_ = startThread([this]()->bool{
//        return process();
//    });
    externalCapturer_->start();
    isCapturing_ = true;
    //MtNdnUtils::performOnBackgroundThread(boost::bind(&VideoCapturer::process,this));
    scheduleJob(captureFrequncy_*1000,boost::bind(&VideoCapturer::process,this));
}

void
VideoCapturer::stop()
{
    isCapturing_ = false;
    externalCapturer_->stop();
}

bool
VideoCapturer::process()
{
    //LOG(INFO) << "VideoCapturer process" << std::endl;
    externalCapturer_->incomingYUV420Frame(capturedFrameObj_,capturedTsMs_);
    //ff_capturer_.getFrame(*capturedFrame_);

 /*   if( isCapturing_ )
    {
        std::cout << "process" << std::endl;
//        scheduleJob(captureFrequncy_*1000, [this]()->bool{
//            bool ret = process();
//            return ret;
//        });
        //scheduleJob(captureFrequncy_*1000,boost::bind(&VideoCapturer::process,this));
    }
*/
    consumer_->onDeliverRawFrame(capturedFrameObj_, capturedTsMs_);
    //std::cout << isCapturing_ << std::endl;
    return isCapturing_;
}
