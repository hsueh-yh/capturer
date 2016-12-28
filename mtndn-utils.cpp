//
//  mtndn-utils.cpp
//  mtndn
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

//#undef NDN_LOGGING

#include <stdarg.h>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>

#include "face-wrapper.h"
#include "glogger.h"

using namespace std;

using namespace boost::chrono;


/*
typedef struct _FrequencyMeter {
    double cycleDuration_;
    unsigned int nCyclesPerSec_;
    double callsPerSecond_;
    int64_t lastCheckTime_;
    unsigned int avgEstimatorId_;
} FrequencyMeter;

typedef struct _DataRateMeter {
    double cycleDuration_;
    unsigned int bytesPerCycle_;
    double nBytesPerSec_;
    int64_t lastCheckTime_;
    unsigned int avgEstimatorId_;
} DataRateMeter;

typedef struct _MeanEstimator {
    unsigned int sampleSize_;
    int nValues_;
    double startValue_;
    double prevValue_;
    double valueMean_;
    double valueMeanSq_;
    double valueVariance_;
    double currentMean_;
    double currentDeviation_;
} MeanEstimator;

typedef struct _Filter {
    double coeff_;
    double filteredValue_;
} Filter;

typedef struct _InclineEstimator {
    unsigned int avgEstimatorId_;
    unsigned int nValues_;
    double lastValue_;
    unsigned int sampleSize_;
    unsigned int skipCounter_;
} InclineEstimator;

typedef struct _SlidingAverage {
    unsigned int sampleSize_;
    int nValues_;
    double accumulatedSum_;
    double currentAverage_;
    double currentDeviation_;
    double *sample_;
} SlidingAverage;
*/

//********************************************************************************

static boost::asio::io_service *MtNdnIoService;
static ptr_lib::shared_ptr<FaceProcessor> LibraryFace;

/*
static std::vector<FrequencyMeter> freqMeters_;
static std::vector<DataRateMeter> dataMeters_;
static std::vector<MeanEstimator> meanEstimators_;
static std::vector<Filter> filters_;
static std::vector<InclineEstimator> inclineEstimators_;
static std::vector<SlidingAverage> slidingAverageEstimators_;

static VoiceEngine *VoiceEngineInstance = NULL;
*/

static boost::thread backgroundThread;
static boost::thread backgroundThread1;
static boost::thread_group backgroundThreadGroup;
static boost::shared_ptr<boost::asio::io_service::work> backgroundWork;

static ptr_lib::shared_ptr<KeyChain> DefaultKeyChain(new KeyChain());

//void initVE();
void resetThread();
void resetThread1();
void startNewThread();

//******************************************************************************
void
MtNdnUtils::setIoService(boost::asio::io_service &ioService)
{
    MtNdnIoService = &ioService;
}

boost::asio::io_service &
MtNdnUtils::getIoService()
{
    return *MtNdnIoService;
}

void
MtNdnUtils::startBackgroundThread()
{
    VLOG(LOG_DEBUG) << "startBackgroundThread "
                 << boost::this_thread::get_id() << std::endl;
    if (!MtNdnIoService)
        return;

    if (!backgroundWork.get() &&
        backgroundThread.get_id() == boost::thread::id())
    {
        backgroundWork.reset(new boost::asio::io_service::work(*MtNdnIoService));
        resetThread();
        //resetThread1();
    }
}

int
MtNdnUtils::addBackgroundThread()
{
    VLOG(LOG_DEBUG) << "addBackgroundThread "
                 << boost::this_thread::get_id() << std::endl;
    if (!MtNdnIoService)
        return -1;

    if ( !backgroundWork.get() )
    {
        backgroundWork.reset(new boost::asio::io_service::work(*MtNdnIoService));
    }
    startNewThread();

    return backgroundThreadGroup.size();
}

void
MtNdnUtils::stopBackgroundThread()
{
    if (backgroundWork.get())
    {
        backgroundWork.reset();
        (*MtNdnIoService).stop();
        backgroundThread = boost::thread();

        if (!isBackgroundThread())
            backgroundThread.try_join_for(boost::chrono::milliseconds(500));
        VLOG(LOG_DEBUG) << "stopBackgroundThread "
                     << boost::this_thread::get_id() << std::endl;
    }
}

bool
MtNdnUtils::isBackgroundThread()
{
    return (boost::this_thread::get_id() == backgroundThread.get_id());
}

void
MtNdnUtils::dispatchOnBackgroundThread(boost::function<void(void)> dispatchBlock,
                                        boost::function<void(void)> onCompletion)
{
    if (backgroundWork.get())
    {
        (*MtNdnIoService).dispatch([=]{
            VLOG(LOG_TRACE) << "dispatchOnBackgroundThread "
                         << boost::this_thread::get_id() << std::endl;
            dispatchBlock();
            VLOG(LOG_TRACE) << "dispatchOnBackgroundThread done "
                         << boost::this_thread::get_id() << std::endl;
            if (onCompletion)
                onCompletion();
        });
    }
    else
    {
        throw std::runtime_error("this is not supposed to happen. bg thread is dead already");
    }
}

void
MtNdnUtils::performOnBackgroundThread(boost::function<void(void)> dispatchBlock,
                                             boost::function<void(void)> onCompletion)
{
    if (backgroundWork.get())
    {
        if (boost::this_thread::get_id() == backgroundThread.get_id())
        {
            VLOG(LOG_DEBUG) << "performOnBackgroundThread dispatch "
                         << boost::this_thread::get_id() << std::endl;
            dispatchBlock();
            VLOG(LOG_DEBUG) << "performOnBackgroundThread dispatch done "
                         << boost::this_thread::get_id() << std::endl;
            if (onCompletion)
                onCompletion();
        }
        else
        {
            boost::mutex m;
            boost::unique_lock<boost::mutex> lock(m);
            boost::condition_variable isDone;

            (*MtNdnIoService).post([dispatchBlock, onCompletion, &isDone]{
                VLOG(LOG_DEBUG) << "performOnBackgroundThread post "
                             << boost::this_thread::get_id() << std::endl;
                dispatchBlock();

                VLOG(LOG_DEBUG) << "performOnBackgroundThread post done "
                             << boost::this_thread::get_id() << std::endl;
                if (onCompletion)
                    onCompletion();
                isDone.notify_one();
            });

            isDone.wait(lock);
        }
    }
    else
    {
        throw std::runtime_error(" this is not supposed to happen. bg thread is dead already");
    }
}

void
MtNdnUtils::createLibFace(const GeneralParams &generalParams)
{
    //std::cout<<"creating libFace..." << std::endl;
    if (!LibraryFace.get() ||
        (LibraryFace.get() &&LibraryFace->getTransport()->getIsConnected() == false))
    {
        //LogWARNING(LIB_LOG) << "Creating library Face..." << std::endl;

        LibraryFace = FaceProcessor::createFaceProcessor(generalParams.host_,generalParams.portNum_,DefaultKeyChain);
                //(generalParams.host_, generalParams.portNum_, MtNdnNamespace::defaultKeyChain());

        //std::cout<<"starting libFace..." << std::endl;
        LibraryFace->startProcessing();

        //LogWARNING(LIB_LOG) << "Library Face created" << std::endl;
    }
}

ptr_lib::shared_ptr<FaceProcessor>
MtNdnUtils::getLibFace()
{
    return LibraryFace;
}

void
MtNdnUtils::destroyLibFace()
{
    if (LibraryFace.get())
    {
        //LogWARNING(LIB_LOG) << "Stopping library Face..." << std::endl;
        LibraryFace->stopProcessing();
        LibraryFace.reset();
        //LogWARNING(LIB_LOG) << "Library face stopped" << std::endl;
    }
}

//******************************************************************************

uint32_t
MtNdnUtils::generateNonceValue()
{
    uint32_t nonce = (uint32_t)std::rand();

    return nonce;
}

Blob
MtNdnUtils::nonceToBlob(const uint32_t nonceValue)
{
    uint32_t beValue = htobe32(nonceValue);
    Blob b((uint8_t *)&beValue, sizeof(uint32_t));
    return b;
}

uint32_t
MtNdnUtils::blobToNonce(const ndn::Blob &blob)
{
    if (blob.size() < sizeof(uint32_t))
        return 0;

    uint32_t beValue = *(uint32_t *)blob.buf();
    return be32toh(beValue);
}


unsigned int
MtNdnUtils::getSegmentsNumber(unsigned int segmentSize, unsigned int dataSize)
{
    return (unsigned int)ceil((float)dataSize/(float)segmentSize);
}

int
MtNdnUtils::segmentNumber(const Name::Component &segmentNoComponent)
{
    std::vector<unsigned char> bytes = *segmentNoComponent.getValue();
    int bytesLength = segmentNoComponent.getValue().size();
    int result = 0;
    unsigned int i;

    for (i = 0; i < bytesLength; ++i) {
        result *= 256.0;
        result += (int)bytes[i];
    }

    return result;
}

int
MtNdnUtils::frameNumber(const Name::Component &frameNoComponent)
{
    return MtNdnUtils::intFromComponent(frameNoComponent);
}

int
MtNdnUtils::intFromComponent(const Name::Component &comp)
{
    std::vector<unsigned char> bytes = *comp.getValue();
    int valueLength = comp.getValue().size();
    int result = 0;
    unsigned int i;

    for (i = 0; i < valueLength; ++i) {
        unsigned char digit = bytes[i];
        if (!(digit >= '0' &&digit <= '9'))
            return -1;

        result *= 10;
        result += (unsigned int)(digit - '0');
    }

    return result;
}

Name::Component
MtNdnUtils::componentFromInt(unsigned int number)
{
    stringstream ss;

    ss << number;
    std::string frameNoStr = ss.str();

    return Name::Component((const unsigned char*)frameNoStr.c_str(),
                           frameNoStr.size());
}

// monotonic clock
int64_t
MtNdnUtils::millisecondTimestamp()
{
    milliseconds msec = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    return msec.count();
}

// monotonic clock
int64_t
MtNdnUtils::microsecondTimestamp()
{
    microseconds usec = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
    return usec.count();
}

// monotonic clock
int64_t
MtNdnUtils::nanosecondTimestamp()
{
    boost::chrono::nanoseconds nsec = boost::chrono::steady_clock::now().time_since_epoch();
    return nsec.count();
}

// system clock
double
MtNdnUtils::unixTimestamp()
{
    auto now = boost::chrono::system_clock::now().time_since_epoch();
    boost::chrono::duration<double> sec = now;
    return sec.count();
}

// system clock
int64_t
MtNdnUtils::millisecSinceEpoch()
{
    milliseconds msec = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return msec.count();
}

void
MtNdnUtils::printMem( char msg[], const unsigned char *startBuf, std::size_t size )
{
    unsigned char *buf = const_cast<unsigned char*>(startBuf);
    printf("\n[%s] size = %ld   addr:[ %p ~ %p ]\n",
           msg, size, (void*)buf, (void*)buf+size);
    printf("**********************************************************************\n");
    fflush(stdout);
    for( int i = 0; i < size; ++i )
    {
        printf("%X ",buf[i]);
        fflush(stdout);
    }
    printf("\n**********************************************************************");
    printf("\n[%s]-End\n\n",msg);
    fflush(stdout);
}


//******************************************************************************
/*
 *
//******************************************************************************
unsigned int MtNdnUtils::setupFrequencyMeter(unsigned int granularity)
{
    FrequencyMeter meter = {1000./(double)granularity, 0, 0., 0, 0};
    meter.avgEstimatorId_ = setupSlidingAverageEstimator(granularity);

    freqMeters_.push_back(meter);

    return freqMeters_.size()-1;
}

void MtNdnUtils::frequencyMeterTick(unsigned int meterId)
{
    if (meterId >= freqMeters_.size())
        return;

    FrequencyMeter &meter = freqMeters_[meterId];
    int64_t now = millisecondTimestamp();
    int64_t delta = now - meter.lastCheckTime_;

    meter.nCyclesPerSec_++;

    if (delta >= meter.cycleDuration_)
        if (meter.lastCheckTime_ >= 0)
        {
            if (meter.lastCheckTime_ > 0)
            {
                meter.callsPerSecond_ = 1000.*(double)meter.nCyclesPerSec_/(double)(delta);
                meter.nCyclesPerSec_ = 0;
                slidingAverageEstimatorNewValue(meter.avgEstimatorId_, meter.callsPerSecond_);
            }

            meter.lastCheckTime_ = now;
        }
}

double MtNdnUtils::currentFrequencyMeterValue(unsigned int meterId)
{
    if (meterId >= freqMeters_.size())
        return 0.;

    FrequencyMeter &meter = freqMeters_[meterId];

    return currentSlidingAverageValue(meter.avgEstimatorId_);
}

void MtNdnUtils::releaseFrequencyMeter(unsigned int meterId)
{
    if (meterId >= freqMeters_.size())
        return;

    // do nothing
}

//******************************************************************************
unsigned int MtNdnUtils::setupDataRateMeter(unsigned int granularity)
{
    DataRateMeter meter = {1000./(double)granularity, 0, 0., 0, 0};
    meter.avgEstimatorId_ = MtNdnUtils::setupSlidingAverageEstimator(10);

    dataMeters_.push_back(meter);

    return dataMeters_.size()-1;
}

void MtNdnUtils::dataRateMeterMoreData(unsigned int meterId,
                                        unsigned int dataSize)
{
    if (meterId >= dataMeters_.size())
        return ;

    DataRateMeter &meter = dataMeters_[meterId];
    int64_t now = millisecondTimestamp();
    int64_t delta = now - meter.lastCheckTime_;

    meter.bytesPerCycle_ += dataSize;

    if (delta >= meter.cycleDuration_)
        if (meter.lastCheckTime_ >= 0)
        {
            if (meter.lastCheckTime_ > 0)
            {
                meter.nBytesPerSec_ = 1000.*meter.bytesPerCycle_/delta;
                meter.bytesPerCycle_ = 0;
                slidingAverageEstimatorNewValue(meter.avgEstimatorId_, meter.nBytesPerSec_);
            }

            meter.lastCheckTime_ = now;
        }
}

double MtNdnUtils::currentDataRateMeterValue(unsigned int meterId)
{
    if (meterId >= dataMeters_.size())
        return 0.;

    DataRateMeter &meter = dataMeters_[meterId];
    return currentSlidingAverageValue(meter.avgEstimatorId_);
}

void MtNdnUtils::releaseDataRateMeter(unsigned int meterId)
{
    if (meterId >- dataMeters_.size())
        return;

    // nothing here yet
}

//******************************************************************************
unsigned int MtNdnUtils::setupMeanEstimator(unsigned int sampleSize,
                                             double startValue)
{
    MeanEstimator meanEstimator = {sampleSize, 0, startValue, startValue, startValue, 0., 0., startValue, 0.};

    meanEstimators_.push_back(meanEstimator);

    return meanEstimators_.size()-1;
}

void MtNdnUtils::meanEstimatorNewValue(unsigned int estimatorId, double value)
{
    if (estimatorId >= meanEstimators_.size())
        return ;

    MeanEstimator &estimator = meanEstimators_[estimatorId];

    estimator.nValues_++;

    if (estimator.nValues_ > 1)
    {
        double delta = (value - estimator.valueMean_);
        estimator.valueMean_ += (delta/(double)estimator.nValues_);
        estimator.valueMeanSq_ += (delta*delta);

        if (estimator.sampleSize_ == 0 ||
            estimator.nValues_ % estimator.sampleSize_ == 0)
        {
            estimator.currentMean_ = estimator.valueMean_;

            if (estimator.nValues_ >= 2)
            {
                double variance = estimator.valueMeanSq_/(double)(estimator.nValues_-1);
                estimator.currentDeviation_ = sqrt(variance);
            }

            // flush nValues if sample size specified
            if (estimator.sampleSize_)
                estimator.nValues_ = 0;
        }
    }
    else
    {
        estimator.currentMean_ = value;
        estimator.valueMean_ = value;
        estimator.valueMeanSq_ = 0;
        estimator.valueVariance_ = 0;
    }

    estimator.prevValue_ = value;
}

double MtNdnUtils::currentMeanEstimation(unsigned int estimatorId)
{
    if (estimatorId >= meanEstimators_.size())
        return 0;

    MeanEstimator &estimator = meanEstimators_[estimatorId];

    return estimator.currentMean_;
}

double MtNdnUtils::currentDeviationEstimation(unsigned int estimatorId)
{
    if (estimatorId >= meanEstimators_.size())
        return 0;

    MeanEstimator &estimator = meanEstimators_[estimatorId];

    return estimator.currentDeviation_;
}

void MtNdnUtils::releaseMeanEstimator(unsigned int estimatorId)
{
    if (estimatorId >= meanEstimators_.size())
        return ;

    // nothing
}

void MtNdnUtils::resetMeanEstimator(unsigned int estimatorId)
{
    if (estimatorId >= meanEstimators_.size())
        return ;

    MeanEstimator &estimator = meanEstimators_[estimatorId];

    estimator = (MeanEstimator){estimator.sampleSize_, 0, estimator.startValue_,
        estimator.startValue_, estimator.startValue_, 0., 0.,
        estimator.startValue_, 0.};
}

//******************************************************************************
unsigned int MtNdnUtils::setupSlidingAverageEstimator(unsigned int sampleSize)
{
    SlidingAverage slidingAverage;

    slidingAverage.sampleSize_ = (sampleSize == 0)?2:sampleSize;
    slidingAverage.nValues_ = 0;
    slidingAverage.accumulatedSum_ = 0.;
    slidingAverage.currentAverage_ = 0.;
    slidingAverage.currentDeviation_ = 0.;

    slidingAverage.sample_ = (double*)malloc(sizeof(double)*sampleSize);
    memset(slidingAverage.sample_, 0, sizeof(double)*sampleSize);

    slidingAverageEstimators_.push_back(slidingAverage);

    return slidingAverageEstimators_.size()-1;
}

double MtNdnUtils::slidingAverageEstimatorNewValue(unsigned int estimatorId, double value)
{
    if (estimatorId >= slidingAverageEstimators_.size())
        return 0.;

    double oldValue = 0;
    SlidingAverage &estimator = slidingAverageEstimators_[estimatorId];

    oldValue = estimator.sample_[estimator.nValues_%estimator.sampleSize_];
    estimator.sample_[estimator.nValues_%estimator.sampleSize_] = value;
    estimator.nValues_++;

    if (estimator.nValues_ >= estimator.sampleSize_)
    {
        estimator.currentAverage_ = (estimator.accumulatedSum_+value)/estimator.sampleSize_;

        estimator.accumulatedSum_ += value - estimator.sample_[estimator.nValues_%estimator.sampleSize_];

        estimator.currentDeviation_ = 0.;
        for (int i = 0; i < estimator.sampleSize_; i++)
            estimator.currentDeviation_ += (estimator.sample_[i]-estimator.currentAverage_)*(estimator.sample_[i]-estimator.currentAverage_);
        estimator.currentDeviation_ = sqrt(estimator.currentDeviation_/(double)estimator.nValues_);
    }
    else
        estimator.accumulatedSum_ += value;

    return oldValue;
}

double MtNdnUtils::currentSlidingAverageValue(unsigned int estimatorId)
{
    if (estimatorId >= slidingAverageEstimators_.size())
        return 0;

    SlidingAverage &estimator = slidingAverageEstimators_[estimatorId];
    return estimator.currentAverage_;
}

double MtNdnUtils::currentSlidingDeviationValue(unsigned int estimatorId)
{
    if (estimatorId >= slidingAverageEstimators_.size())
        return 0;

    SlidingAverage &estimator = slidingAverageEstimators_[estimatorId];
    return estimator.currentDeviation_;
}

void MtNdnUtils::resetSlidingAverageEstimator(unsigned int estimatorID)
{
    if (estimatorID >= slidingAverageEstimators_.size())
        return ;

    SlidingAverage &estimator = slidingAverageEstimators_[estimatorID];
    estimator.nValues_ = 0;
    estimator.accumulatedSum_ = 0.;
    estimator.currentAverage_ = 0.;
    estimator.currentDeviation_ = 0.;
    memset(estimator.sample_, 0, sizeof(double)*estimator.sampleSize_);
}

void MtNdnUtils::releaseAverageEstimator(unsigned int estimatorID)
{
    // nothing
}

//******************************************************************************
unsigned int
MtNdnUtils::setupFilter(double coeff)
{
    Filter filter = {coeff, 0.};
    filters_.push_back(filter);
    return filters_.size()-1;
}

void
MtNdnUtils::filterNewValue(unsigned int filterId, double value)
{
    if (filterId < filters_.size())
    {
        Filter &filter = filters_[filterId];

        if (filter.filteredValue_ == 0)
            filter.filteredValue_ = value;
        else
            filter.filteredValue_ += (value-filter.filteredValue_)*filter.coeff_;
    }
}

double
MtNdnUtils::currentFilteredValue(unsigned int filterId)
{
    if (filterId < filters_.size())
        return filters_[filterId].filteredValue_;

    return 0.;
}

void
MtNdnUtils::releaseFilter(unsigned int filterId)
{
    // do nothing
}

//******************************************************************************
unsigned int
MtNdnUtils::setupInclineEstimator(unsigned int sampleSize)
{
    InclineEstimator ie;
    ie.sampleSize_ = sampleSize;
    ie.nValues_ = 0;
    ie.avgEstimatorId_ = MtNdnUtils::setupSlidingAverageEstimator(sampleSize);
    ie.lastValue_ = 0.;
    ie.skipCounter_ = 0;
    inclineEstimators_.push_back(ie);

    return inclineEstimators_.size()-1;
}

void
MtNdnUtils::inclineEstimatorNewValue(unsigned int estimatorId, double value)
{
    if (estimatorId < inclineEstimators_.size())
    {
        InclineEstimator &ie = inclineEstimators_[estimatorId];

        if (ie.nValues_ == 0)
            ie.lastValue_ = value;
        else
        {
            double incline = (value-ie.lastValue_);

            slidingAverageEstimatorNewValue(ie.avgEstimatorId_, incline);
            ie.lastValue_ = value;
        }

        ie.nValues_++;
    }
}

double
MtNdnUtils::currentIncline(unsigned int estimatorId)
{
    if (estimatorId < inclineEstimators_.size())
    {
        InclineEstimator &ie = inclineEstimators_[estimatorId];
        return currentSlidingAverageValue(ie.avgEstimatorId_);
    }

    return 0.;
}

void MtNdnUtils::releaseInclineEstimator(unsigned int estimatorId)
{
    // tbd
}

//******************************************************************************
string MtNdnUtils::stringFromFrameType(const WebRtcVideoFrameType &frameType)
{
    switch (frameType) {
        case webrtc::kDeltaFrame:
            return "DELTA";
        case webrtc::kKeyFrame:
            return "KEY";
        case webrtc::kAltRefFrame:
            return "ALT-REF";
        case webrtc::kGoldenFrame:
            return "GOLDEN";
        case webrtc::kSkipFrame:
            return "SKIP";
        default:
            return "UNKNOWN";
    }
}

unsigned int MtNdnUtils::toFrames(unsigned int intervalMs,
                                   double fps)
{
    return (unsigned int)ceil(fps*(double)intervalMs/1000.);
}

unsigned int MtNdnUtils::toTimeMs(unsigned int frames,
                                   double fps)
{
    return (unsigned int)ceil((double)frames/fps*1000.);
}
*/

std::string MtNdnUtils::getFullLogPath(const /*new_api::*/GeneralParams &generalParams,
                                  const std::string &fileName)
{
    static char logPath[PATH_MAX];
    return ((generalParams.logPath_ == "")  ? std::string(getwd(logPath))
                                            : generalParams.logPath_) + "/logs/" + fileName;
}


std::string MtNdnUtils::formatString(const char *format, ...)
{
    std::string str = "";

    if (format)
    {
        char *stringBuf = (char*)malloc(256);
        memset((void*)stringBuf, 0, 256);

        va_list args;

        va_start(args, format);
        vsprintf(stringBuf, format, args);
        va_end(args);

        str = string(stringBuf);
        free(stringBuf);
    }

    return str;
}

//******************************************************************************

static bool ThreadRecovery = false;
void resetThread()
{
    backgroundThread = boost::thread([](){
        try
        {
            if (ThreadRecovery)
            {
                ThreadRecovery = false;
            }
            VLOG(LOG_DEBUG) << "resetThread0 "
                         << boost::this_thread::get_id() << std::endl;

            MtNdnIoService->run();
            VLOG(LOG_DEBUG) << "resetThread0 ioservice.run OVER "
                         << boost::this_thread::get_id() << std::endl;
        }
        catch (std::exception &e) // fatal
        {
            MtNdnIoService->reset();
            //MtNdnManager::getSharedInstance().fatalException(e);
            ThreadRecovery = true;
            resetThread();
        }
    });

}

void resetThread1()
{
    backgroundThread1 = boost::thread([](){
        try
        {
            if (ThreadRecovery)
            {
                ThreadRecovery = false;
            }
            VLOG(LOG_DEBUG) << "resetThread1 "
                         << boost::this_thread::get_id() << std::endl;

            MtNdnIoService->run();
            VLOG(LOG_DEBUG) << "resetThread1 ioservice.run OVER "
                         << boost::this_thread::get_id() << std::endl;
        }
        catch (std::exception &e) // fatal
        {
            MtNdnIoService->reset();
            //MtNdnManager::getSharedInstance().fatalException(e);
            ThreadRecovery = true;
            resetThread1();
        }
    });

}

void startNewThread()
{
    boost::thread *bkgThread = new boost::thread([](){
        try
        {
            if (ThreadRecovery)
            {
                ThreadRecovery = false;
            }
            VLOG(LOG_DEBUG) << "startNewThread "
                         << boost::this_thread::get_id() << std::endl;

            MtNdnIoService->run();
            VLOG(LOG_DEBUG) << "startNewThread ioservice.run OVER "
                         << boost::this_thread::get_id() << std::endl;
        }
        catch (std::exception &e) // fatal
        {
            MtNdnIoService->reset();
            //MtNdnManager::getSharedInstance().fatalException(e);
            ThreadRecovery = true;
            startNewThread();
        }
    });

    backgroundThreadGroup.add_thread(bkgThread);

}

