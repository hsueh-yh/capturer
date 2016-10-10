#include <cstdlib>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <ndn-cpp/transport/tcp-transport.hpp>
#include <ndn-cpp/transport/unix-transport.hpp>
#include <boost/asio.hpp>
#include <ndn-cpp/threadsafe-face.hpp>

#include "logger.hpp"
#include "publisher.h"


#define HOST_DEFAULT "localhost"
//#define HOST_DEFAULT "10.103.242.191"
#define PORT_DEFAULT 6363

using namespace std;


int main(int argc, char** argv)
{
    char logPath[20] = "./logs";
    GLogger glog(argv[0],logPath);

    try {
        boost::asio::io_service ioService;
        //ThreadsafeFace face( ioService, HOST_DEFAULT, PORT_DEFAULT );
//        std::shared_ptr<ndn::Transport::ConnectionInfo> connInfo;
//        std::shared_ptr<ndn::Transport> transport;

//        connInfo.reset(new TcpTransport::ConnectionInfo(HOST_DEFAULT, PORT_DEFAULT));
//        transport.reset(new TcpTransport());

//        Face *face1 = new Face(transport, connInfo);


        // The default Face will connect using a Unix socket, or to "localhost".
        //Face face(HOST_DEFAULT,PORT_DEFAULT);
        boost::shared_ptr<Face> face;
        face.reset(new ThreadsafeFace (ioService, "localhost"));

        // Use the system default key chain and certificate name to sign commands.
        KeyChain keyChain;
        face->setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());

        // Also use the default certificate name to sign data packets.
        Publisher publisher(ioService, face, keyChain, keyChain.getDefaultCertificateName());

        if( !publisher.init() )
        {
            cout << "Publisher init fail" << endl;
            return 0;
        }
        else
        {
            publisher.start();
        }
        //cout << "Publisher READY" << endl;
       // Name prefix = publisher.getStreamPrefix();

        // TODO: After we remove the registerPrefix with the deprecated OnInterest,
        // we can remove the explicit cast to OnInterestCallback (needed for boost).
//        cout << face.isLocal() << endl;
//        uint64_t registedId = 0;
//        registedId = face.registerPrefix(prefix,
//                            (const OnInterestCallback&)func_lib::ref(publisher),
//                            func_lib::ref(publisher));
//        if( registedId != 0 )
//            cout << "Register prefix  " << prefix.toUri() << " SUCCESSFUL " << endl;
//        publisher.start();


        // Keep ioService running until the Counter calls stop().
        boost::asio::io_service::work work(ioService);
        ioService.run();
    }
    catch (std::exception& e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
