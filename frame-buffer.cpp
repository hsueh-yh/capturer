
#include <math.h>

#include "frame-buffer.h"
#include "utils.h"
#include "name-components.h"
//#include "namespacer.h"

FrameBuffer::FrameBuffer() :
    maxSegmentSize_(ndn::Face::getMaxNdnPacketSize()),
    maxSegBlockSize_(ndn::Face::getMaxNdnPacketSize()-SegmentData::getHeaderSize()),
    bufSize_(200)
{
    init();
}

FrameBuffer::~FrameBuffer()
{}

void
FrameBuffer::init(int frameNumbers)
{
    if( bufSize_ < frameNumbers )
        bufSize_ = frameNumbers;
    reset();
    initSlots();
}

void
FrameBuffer::recvFrame( FrameData &frame, ndn::Name framePrefix )
{
    boost::shared_ptr<DataBlock> segment;
    int size = frame.size();

    int segNum = ceil( (double)size / (double)maxSegBlockSize_ );
    int lastSegSize = size % maxSegBlockSize_;

    int currentBlockSize;
    for( int i = 0; i < segNum; i++ )
    {
        currentBlockSize = (i==segNum-1) ? lastSegSize : maxSegBlockSize_;
        segment = getFreeSlot();
        segment->fillData(frame.getData()+(i*maxSegBlockSize_),currentBlockSize);

        ndn::Name segPrefix(framePrefix);
        segPrefix.append(NdnRtcUtils::componentFromInt(i));
        activeSlots_[segPrefix] = segment;
        LOG(INFO) << "[FrameBuffer] Cached " << segPrefix.toUri()
                  << " ( Size = " << currentBlockSize<<" )" << endl;
    }
}

boost::shared_ptr<SegmentData>
FrameBuffer::acquireSegment(const ndn::Interest& interest)
{
    boost::shared_ptr<SegmentData> segment;
    std::map<ndn::Name, boost::shared_ptr<DataBlock> >::iterator iter;
    iter = activeSlots_.find(interest.getName());
    if (iter==activeSlots_.end())
        return segment;
    boost::shared_ptr<DataBlock> segBlock = iter->second;
    uint32_t interestNonce_ = NdnRtcUtils::blobToNonce(interest.getNonce());
    int64_t interestArrivalMs_ = NdnRtcUtils::microsecondTimestamp();


    segment.reset(new SegmentData(segBlock->data_,
                                   segBlock->length_,
                                   (SegmentData::SegmentMetaInfo){interestNonce_,interestArrivalMs_,0}));
    return segment;
}


//protected functions
//**************************************************
void
FrameBuffer::reset()
{
    boost::shared_ptr<DataBlock> segment;
    std::map<ndn::Name, boost::shared_ptr<DataBlock> >::iterator it;
    for( it = activeSlots_.begin(); it != activeSlots_.end(); ++it )
    {
        segment = it->second;
        freeSlots_.push_back(segment);
    }
    activeSlots_.clear();
}

void
FrameBuffer::initSlots()
{
    while( freeSlots_.size() < bufSize_ )
    {
        boost::shared_ptr<DataBlock> segment;
        segment.reset(new DataBlock(maxSegmentSize_));
        freeSlots_.push_back(segment);
    }
}

boost::shared_ptr<DataBlock>
FrameBuffer::getFreeSlot()
{
    boost::shared_ptr<DataBlock> segment;
    if( freeSlots_.size() )
    {
        segment = freeSlots_.at(freeSlots_.size()-1);
        freeSlots_.pop_back();
    }
    else
    {
        std::map<ndn::Name, boost::shared_ptr<DataBlock> >::iterator it;
        it = activeSlots_.begin();
        segment = (--it)->second;
        activeSlots_.erase(it);
    }
    return segment;
}
