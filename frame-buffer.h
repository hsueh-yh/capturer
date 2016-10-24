#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>
#include <map>
#include <mutex>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>

#include "frame-data.h"
#include "publisher.h"
#include "logger.hpp"

using namespace ndn;

class Publisher;

class FrameBuffer
{
public:

    FrameBuffer();
    ~FrameBuffer();

    void init( int frameNumbers = 100 );

    void recvFrame(FrameData &frame, ndn::Name framePrefix );

    ptr_lib::shared_ptr<SegmentData>
    acquireSegment( const ndn::Interest& interest );

    ptr_lib::shared_ptr<PrefixMetaInfo>
    acquireFrameMeta(const ndn::Interest& interest);

    void getCachedRange(FrameNumber& start, FrameNumber& end);

private:

    void reset();

    void initSlots();

    ptr_lib::shared_ptr<DataBlock> getFreeSlot();

    ptr_lib::shared_ptr<PrefixMetaInfo> getFreeFrameMeta();

    ptr_lib::shared_ptr<Publisher> publisher_;

    std::vector<ptr_lib::shared_ptr<DataBlock> > freeSlots_;
    //std::map<ndn::Name, ptr_lib::shared_ptr<SegmentData> > activeSlots_;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> > activeSlots_;
    std::vector<ptr_lib::shared_ptr<PrefixMetaInfo> > freeFrameMeta_;
    std::map<ndn::Name, ptr_lib::shared_ptr<PrefixMetaInfo> > activeFrameMeta_;

    int     maxSegBlockSize_,
            maxSegmentSize_;

    unsigned char *buf_;
    int     bufSize_;

    mutable ptr_lib::recursive_mutex syncMutex_, metaMutex_;

    FILE *fp;
    bool isbackup;

};


#endif // FRAMEBUFFER_H
