#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

#include <boost/asio.hpp>

#include <ndn-cpp/transport/tcp-transport.hpp>
#include <ndn-cpp/transport/unix-transport.hpp>
#include <ndn-cpp/threadsafe-face.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "glogger.h"
#include "publisher.h"

#include "manager.h"



#define HOST_DEFAULT "localhost"
//#define HOST_DEFAULT "10.103.242.191"
#define PORT_DEFAULT 6363

using namespace std;

Manager *manager;

std::string createLogDir( const char* root )
{
    fstream rootfile;
    rootfile.open(root, ios::in);
    if(!rootfile)
    {
        std::cout << "Create log dir " << root << std::endl;
        int isCreate = mkdir(root, S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if( 0 > isCreate )
        {
            std::cout << "Create log dir failed: " << root << std::endl;
        }
    }
    else
        rootfile.close();

    // get local date
    time_t timestamp = time(NULL);
    tm* date= localtime(&timestamp);

    stringstream logDir;
    logDir << date->tm_year + 1900;
    logDir << "-";
    logDir << date->tm_mon + 1;
    logDir << "-";
    logDir << date->tm_mday;
    logDir << "_";
    logDir << date->tm_hour;
    logDir << ":";
    logDir << date->tm_min;
    logDir << ":";
    logDir << date->tm_sec;

    std::string logRoot("./logs/");
    std::string logfile(logRoot);
    logfile.append(logDir.str());

    // create log dir
    int isCreate = mkdir(logfile.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if( 0 > isCreate )
    {
        std::cout << "Create path failed: " << logfile << std::endl;
    }

    return logfile;
}

void addLocalStream(std::string transType, std::string streamName, FFCapturer *capturer)
{
    manager = &(Manager::getSharedInstance());

    GeneralParams generalParams;
    MediaStreamParams mediaStreamParams;
    VideoThreadParams videoThreadParams;
    VideoCoderParams videoCoderParams;

    generalParams.transType_ = transType;
    generalParams.host_ = HOST_DEFAULT;
    generalParams.portNum_ = PORT_DEFAULT;

    mediaStreamParams.streamName_ = streamName;
    mediaStreamParams.type_ = MediaStreamParams::MediaStreamTypeVideo;

    videoCoderParams.codecFrameRate_ = 30;
    videoCoderParams.encodeHeight_ = 480;
    videoCoderParams.encodeWidth_ = 640;
    videoCoderParams.gop_ = 10;
    videoCoderParams.maxBitrate_ = 600000;

    videoThreadParams.coderParams_ = videoCoderParams;

    //capturer = capt;

    std::string addedStreamName;
    addedStreamName = manager->addLocalStream(generalParams,videoThreadParams,
                            mediaStreamParams,capturer);
    LOG(INFO) << "addedStream " << addedStreamName << std::endl << std::endl;
}

int main(int argc, char** argv)
{
    int num = 1, i;
    string transType; // frame or stream
    for( int i = 1; i < argc; ++i )
    {
        if( strcmp(argv[i], "-n" ) == 0 )
                num = argv[i+1][0] - '0';
        if( strcmp(argv[i], "-t") == 0 )
            transType = argv[i+1];
    }

    std::string logfile = createLogDir("./logs");
    GLogger glog( argv[0], logfile.c_str() );
    std::cout << "Log to path: " << logfile << std::endl;

    FFCapturer *capturer = new FFCapturer;
    capturer->init();
    capturer->start();

    for( i = 0; i < num; ++i )
    {
        std::string streamName = "/com/monitor/location1/stream";
        stringstream ss;
        ss << i;
        streamName.append(ss.str());
        streamName.append("/video");
        //std::cout << streamName << std::endl;

        addLocalStream(transType, streamName, capturer);
    }
    std::cout << "added " << i << " stream" << std::endl;

    std::string endstr;
    std::cin>>endstr;
    while( 0!=endstr.compare("stop") && 0!=endstr.compare("STOP"))
        std::cin >> endstr;
    return 0;
}

/*
int main(int argc, char** argv)
{
    std::string streamName = "/com/monitor/location1/stream";
    if( argc == 1 )
        streamName.append("0");
    else
    {
        streamName.append(argv[1]);
    }
    //std::cout << streamName << std::endl;
    std::string logfile = createLogDir("./logs");
    GLogger glog( argv[0], logfile.c_str() );
    std::cout << "Log to path: " << logfile << std::endl;

    try {
        boost::asio::io_service ioService;

        ptr_lib::shared_ptr<ThreadsafeFace> face;
        face.reset(new ThreadsafeFace (ioService, "localhost"));

        // Use the system default key chain and certificate name to sign commands.
        KeyChain keyChain;
        face->setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());

        // Also use the default certificate name to sign data packets.
        Publisher publisher(streamName, ioService, face, keyChain,
                            keyChain.getDefaultCertificateName() );

        if( !publisher.init() )
        {
            cout << "Publisher init fail" << endl;
            return 0;
        }
        else
        {
            std::thread *captureThread =
                    new std::thread(bind(&Publisher::start,&publisher));

            captureThread->detach();

            LOG(INFO) << "ioservice start" << endl;
            boost::asio::io_service::work work(ioService);
            ioService.run();
            LOG(INFO) << "ioservice started" << endl;
        }
    }
    catch (std::exception& e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
*/
