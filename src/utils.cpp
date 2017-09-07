//
//  ndnrtc-utils.cpp
//  ndnrtc
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
#include "utils.h"
//#include "face-wrapper.h"

using namespace std;

using namespace boost::chrono;



uint32_t NdnUtils::generateNonceValue()
{
    uint32_t nonce = (uint32_t)std::rand();

    return nonce;
}

Blob NdnUtils::nonceToBlob(const uint32_t nonceValue)
{
    uint32_t beValue = htobe32(nonceValue);
    Blob b((uint8_t *)&beValue, sizeof(uint32_t));
    return b;
}

uint32_t NdnUtils::blobToNonce(const ndn::Blob &blob)
{
    if (blob.size() < sizeof(uint32_t))
        return 0;

    uint32_t beValue = *(uint32_t *)blob.buf();
    return be32toh(beValue);
}


unsigned int NdnUtils::getSegmentsNumber(unsigned int segmentSize, unsigned int dataSize)
{
    return (unsigned int)ceil((float)dataSize/(float)segmentSize);
}

int NdnUtils::segmentNumber(const Name::Component &segmentNoComponent)
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

int NdnUtils::frameNumber(const Name::Component &frameNoComponent)
{
    return NdnUtils::intFromComponent(frameNoComponent);
}

int NdnUtils::intFromComponent(const Name::Component &comp)
{
    std::vector<unsigned char> bytes = *comp.getValue();
    int valueLength = comp.getValue().size();
    int result = 0;
    unsigned int i;

    for (i = 0; i < valueLength; ++i) {
        unsigned char digit = bytes[i];
        if (!(digit >= '0' && digit <= '9'))
            return -1;

        result *= 10;
        result += (unsigned int)(digit - '0');
    }

    return result;
}

Name::Component NdnUtils::componentFromInt(unsigned int number)
{
    stringstream ss;

    ss << number;
    std::string frameNoStr = ss.str();

    return Name::Component((const unsigned char*)frameNoStr.c_str(),
                           frameNoStr.size());
}

// monotonic clock
int64_t NdnUtils::millisecondTimestamp()
{
    milliseconds msec = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    return msec.count();
}

// monotonic clock
int64_t NdnUtils::microsecondTimestamp()
{
    microseconds usec = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
    return usec.count();
}

// monotonic clock
int64_t NdnUtils::nanosecondTimestamp()
{
    boost::chrono::nanoseconds nsec = boost::chrono::steady_clock::now().time_since_epoch();
    return nsec.count();
}

// system clock
double NdnUtils::unixTimestamp()
{
    auto now = boost::chrono::system_clock::now().time_since_epoch();
    boost::chrono::duration<double> sec = now;
    return sec.count();
}

// system clock
int64_t NdnUtils::millisecSinceEpoch()
{
    milliseconds msec = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return msec.count();
}

void NdnUtils::printMem( char msg[], const unsigned char* startBuf, std::size_t size )
{
    unsigned char* buf = const_cast<unsigned char*>(startBuf);
    printf("\n[%s] size = %ld   addr:[ %p ~ %p ]\n",
           msg, size, (void*)buf, (void*)(buf+size));
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

