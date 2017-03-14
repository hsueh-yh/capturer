#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>
#include <map>
#include <mutex>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/face.hpp>

#include "frame-data.h"
#include "publisher.h"
#include "glogger.h"

using namespace ndn;

class Publisher;

class FrameBuffer
{
public:

    FrameBuffer(std::string basePrefix);
    ~FrameBuffer();

    void
    init( int frameNumbers = 100 );

    void
    appendData(const unsigned char* data, const unsigned int size, int64_t millisecondTimestamp);

    ptr_lib::shared_ptr<DataBlock>
    acquireData(const ndn::Name& prefix , Name &nalType);

    unsigned int
    getLastPktNo()
    { return lastSeqNo_; }

    void
    getCachedRange(FrameNumber& start, FrameNumber& end);

private:

    std::string     basePrefix_;

    std::vector<ptr_lib::shared_ptr<DataBlock> >
                    freeSlots_;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >
                    activeSlots_;
    ndn::Name       lastPkgName_;
    unsigned int    lastPkgNo_,
                    lastSeqNo_,
                    startPkgNo_;

    int             maxNdnPktSize_;

    unsigned char*  buf_;
    int             bufSize_;

    mutable ptr_lib::recursive_mutex
                    syncMutex_,
                    metaMutex_;

    FILE*           fp;
    bool            isbackup;


    ///////////////////////////////////////////////////////
    void
    reset();

    void
    initSlots();

    ptr_lib::shared_ptr<DataBlock>
    getFreeSlot();

};


#endif // FRAMEBUFFER_H
