#ifndef _VIDEOCAPTURE_H_
#define _VIDEOCAPTURE_H_

#include "video-encoder.h"
#include "interfaces.h"
#include "mtndn-object.h"
#include "capturer.h"


class VideoCapturer : public MtNdnComponent
{
public:
    VideoCapturer(IExternalCapturer* externalCapturer);
    ~VideoCapturer();

    void init(const VideoCapturerParams &params );

    void start();

    void stop();

    void registerRawFrameConsumer(IRawFrameConsumer *consumer)
    { consumer_ = consumer; }

    void unRegisterRawFrameConsumer()
    { consumer_ = nullptr; }


private:
    bool isCapturing_ = false;
    IRawFrameConsumer *consumer_;
    IExternalCapturer *externalCapturer_;

    boost::thread captureThread_;
    //FFCapturer ff_capturer_;

    void *capturedFrameObj_;
    int64_t capturedTsMs_;
    unsigned int captureFrequncy_ = 30;


    bool process();
};

#endif //_VIDEOCAPTURE_H_
