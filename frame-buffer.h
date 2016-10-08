#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <boost/thread/mutex.hpp>
#include <vector>
#include <map>

#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>

#include "frame-data.h"
#include "publisher.h"

class Publisher;

class FrameBuffer
{
public:

    FrameBuffer();
    ~FrameBuffer();

    void init( int frameNumbers = 200 );

    void recvFrame( FrameData &frame, unsigned int frameNo );

    boost::shared_ptr<SegmentData>
    acquireSegment( const ndn::Interest& interest );


private:

    void reset();

    void initSlots();

    boost::shared_ptr<DataBlock> getFreeSlot();

    boost::shared_ptr<Publisher> publisher_;

    std::vector<boost::shared_ptr<DataBlock> > freeSlots_;
    //std::map<ndn::Name, boost::shared_ptr<SegmentData> > activeSlots_;
    std::map<ndn::Name, boost::shared_ptr<DataBlock> > activeSlots_;

    int     maxSegBlockSize_,
            maxSegmentSize_;

    unsigned char *buf_;
    int     bufSize_;

};


#endif // FRAMEBUFFER_H
