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

#include "logger.hpp"
#include "publisher.h"



#define HOST_DEFAULT "localhost"
//#define HOST_DEFAULT "10.103.242.191"
#define PORT_DEFAULT 6363

using namespace std;

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
