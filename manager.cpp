#include "manager.h"
#include "mtndn-utils.h"
#include "video-publisher.h"

typedef std::map<std::string, ptr_lib::shared_ptr<Publisher>> PublisherMap;

static PublisherMap ActivePublishers;
static boost::asio::io_service libIoService;
static boost::asio::steady_timer recoveryCheckTimer(libIoService);

void init();
void reset();
void cleanup();


//********************************************************************************
//#pragma mark module loading
__attribute__((constructor))
static void initializer(int argc, char* *argv, char* *envp) {
    static int initialized = 0;
    if (!initialized)
    {
        initialized = 1;
        MtNdnUtils::setIoService(libIoService);
    }
}

__attribute__((destructor))
static void destructor(){
}


//******************************************************************************
//#pragma mark - construction/destruction
Manager::Manager()
{
    init();
    initialized_ = true;
    failed_ = false;
}

Manager::~Manager()
{
    cleanup();
    initialized_ = false;
}

Manager&
Manager::getSharedInstance()
{
    static Manager manager;
    return manager;
}

std::string
Manager::addLocalStream(const GeneralParams &generalParams,
                        const VideoThreadParams &videoThreadParams,
                        const MediaStreamParams &mediaStreamParams,
                        IExternalCapturer *const capturer)
{
    std::string streamPrefix = "";

    //int threadsize = MtNdnUtils::addBackgroundThread();
    //cout << "thread size " << threadsize << endl;

    MtNdnUtils::performOnBackgroundThread([&]()->void{
        MtNdnUtils::createLibFace(generalParams);
        ptr_lib::shared_ptr<FaceProcessor> face = MtNdnUtils::getLibFace();

        ptr_lib::shared_ptr<VideoPublisher> publisher;
        publisher.reset(new VideoPublisher(generalParams, capturer));

        PublisherSettings settings;
        settings.streamPrefix_ = mediaStreamParams.streamName_;
        settings.faceProcessor_ = face;
        settings.streamParams_ = mediaStreamParams;

        publisher->init(settings,&videoThreadParams);
        streamPrefix = publisher->getStreamName();

        publisher->start();
        ActivePublishers[streamPrefix] = publisher;
    });

    return streamPrefix;
}


std::string
Manager::removeLocalStream(const std::string streamPrefix)
{
    std::string logFileName = "";

    LOG(INFO) << "Removing stream " << streamPrefix << "..." << std::endl;

    PublisherMap::iterator it = ActivePublishers.find(streamPrefix);

    if (it == ActivePublishers.end())
    {
        LOG(INFO) << "Stream was not added previously" << std::endl;
    }
    else
    {
        //logFileName = it->second->getLogger()->getFileName();
        it->second->stop();
        {
            MtNdnUtils::performOnBackgroundThread([&]()->void{
                ActivePublishers.erase(it);
            });
        }

        LOG(INFO) << "Stream removed successfully" << std::endl;
    }
    LOG(INFO) << "Remove Remote Stream SUCCESS " << streamPrefix
              << std::endl;

    return logFileName;
}

//******************************************************************************
void init()
{
    //Logger::initAsyncLogging();
    //GLogger log("MtNdnLibrary","/home/xyh/workspace/MTNDN/logs");
    LOG(INFO) << "MTNDN initializing" << std::endl;
    reset();

    //reset();
}

void reset()
{
    MtNdnUtils::startBackgroundThread();
}

void cleanup()
{
    LOG(INFO) << "Stopping active consumers..." << std::endl;
    {
        for (auto publisherIt : ActivePublishers)
            publisherIt.second->stop();
        ActivePublishers.clear();
    }
    LOG(INFO) << "Active consumers cleared" << std::endl;

    MtNdnUtils::destroyLibFace();

    MtNdnUtils::stopBackgroundThread();
    LOG(INFO) << "Bye" << std::endl;
    //Logger::releaseAsyncLogging();
}

