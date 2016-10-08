//
//  ndnrtc-utils.h
//  ndnrtc
//
//  Copyright 2013 Regents of the University of California
//  For licensing details see the LICENSE file.
//
//  Author:  Peter Gusev
//

#ifndef _utils_
#define _utils_


#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/security/key-chain.hpp>

#define STR(exp) (#exp)

using namespace boost;
using namespace ndn;


class FaceProcessor;

class NdnRtcUtils {

public:

    //*****************************************************
    //  interest
    //*****************************************************

    static Name getBasePrefix() { return Name("/com/monitor/local/"); }

    static Name prefixfromFrameNo( int frameNo )
    {
        Name tmp = getBasePrefix();
        tmp.append(componentFromInt(frameNo));
        return tmp;
    }
    static Name prefixfromSegmentationNo( int frameNo, int segNo )
    {
        Name tmp = getBasePrefix();
        tmp.append(componentFromInt(frameNo));
        tmp.append(componentFromInt(segNo));
        return tmp;
    }


//	static unsigned int getSegmentsNumber(unsigned int segmentSize, unsigned int dataSize);

	static double timestamp() {
		return time(NULL) * 1000.0;
	}

	static int64_t millisecondTimestamp();
	static int64_t microsecondTimestamp();
	static int64_t nanosecondTimestamp();
	static double unixTimestamp();
	static int64_t millisecSinceEpoch();


    unsigned int getSegmentsNumber(unsigned int segmentSize, unsigned int dataSize);
	static int frameNumber(const Name::Component &segmentComponent);
	static int segmentNumber(const Name::Component &segmentComponent);

	static int intFromComponent(const Name::Component &comp);
	static Name::Component componentFromInt(unsigned int number);

    static uint32_t generateNonceValue();
    static Blob nonceToBlob(const uint32_t nonceValue);
    static uint32_t blobToNonce(const Blob &blob);

};

#endif /* defined(_utils_) */
