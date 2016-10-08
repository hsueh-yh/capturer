#include <cstdlib>
#include <iostream>
#include <time.h>
#include <unistd.h>
#include <ndn-cpp/transport/tcp-transport.hpp>
#include <ndn-cpp/transport/unix-transport.hpp>

#include "logger.hpp"
#include "publisher.h"


#define HOST_DEFAULT "localhost"
//#define HOST_DEFAULT "10.103.243.176"
#define PORT_DEFAULT 6363

using namespace std;


int main(int argc, char** argv)
{
    char logPath[20] = "./logs";
    GLogger glog(argv[0],logPath);

    try {
//        std::shared_ptr<ndn::Transport::ConnectionInfo> connInfo;
//        std::shared_ptr<ndn::Transport> transport;

//        connInfo.reset(new TcpTransport::ConnectionInfo(HOST_DEFAULT, PORT_DEFAULT));
//        transport.reset(new TcpTransport());

//        Face *face1 = new Face(transport, connInfo);


        // The default Face will connect using a Unix socket, or to "localhost".
        //Face face(HOST_DEFAULT,PORT_DEFAULT);
        Face face;

        // Use the system default key chain and certificate name to sign commands.
        KeyChain keyChain;
        face.setCommandSigningInfo(keyChain, keyChain.getDefaultCertificateName());

        // Also use the default certificate name to sign data packets.
        Publisher publisher(keyChain, keyChain.getDefaultCertificateName());
        if( !publisher.init() )
        {
            cout << "Publisher init fail" << endl;
            return 0;
        }

        Name prefix = publisher.getStreamPrefix();
        cout << "Register prefix  " << prefix.toUri() << endl;
        // TODO: After we remove the registerPrefix with the deprecated OnInterest,
        // we can remove the explicit cast to OnInterestCallback (needed for boost).
        cout << face.isLocal() << endl;
        face.registerPrefix(prefix,
                            (const OnInterestCallback&)func_lib::ref(publisher),
                            func_lib::ref(publisher));


        publisher.start();
        // The main event loop.
        // Wait forever to receive one interest for the prefix.
        while ( publisher.getStatus() ) {
            face.processEvents();
            // We need to sleep for a few milliseconds so we don't use 100% of the CPU.
            usleep(100);
        }
    }
    catch (std::exception& e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}
