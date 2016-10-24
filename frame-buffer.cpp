
#include <math.h>

#include "frame-buffer.h"
#include "utils.h"
#include "name-components.h"
#include "namespacer.h"

FrameBuffer::FrameBuffer() :
    maxSegmentSize_(ndn::Face::getMaxNdnPacketSize()),
    maxSegBlockSize_(ndn::Face::getMaxNdnPacketSize()-SegmentData::getHeaderSize()),
    bufSize_(200),
    isbackup(false)
{
    init();
    if( isbackup )
    {
        fp = fopen("framebuffer-out.264","wb+");
        if( fp == NULL )
            cout << " open buffer file error" << endl;
    }
}

FrameBuffer::~FrameBuffer()
{
    if( isbackup )
    {
        if(fp)
            fclose(fp);
    }
}

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
    lock_guard<recursive_mutex> scopedLock(syncMutex_);
    //fwrite(frame.getFrameData(),1,frame.getDataBlockSize(),fp);

    ptr_lib::shared_ptr<DataBlock> dataBlock;
    int size = frame.size();

    int segNum = ceil( (double)size / (double)maxSegBlockSize_ );
    int lastSegSize = size % maxSegBlockSize_;

    ptr_lib::shared_ptr<PrefixMetaInfo> frameMeta = getFreeFrameMeta();
    if( !frameMeta.get() )
    {
        LOG(ERROR) << "[FrameBuffer] recvFrame no freeFrameMeta!" << endl;
    }
    frameMeta->totalSegmentNum_ = segNum;
    frameMeta->deltaFrameNo_ = segNum;
    frameMeta->playbackNo_ = 0;
    activeFrameMeta_[framePrefix] = frameMeta;

//    cout <<"Frame: "
//         << " size=" << frame.size()
//         << " data: " << (void*)const_cast<unsigned char*>(frame.getBuf())
//         << " ~ " << (void*)frame.getFrameData()
//         << " ~ " << (void*)(frame.getBuf()+frame.size())
//         << endl << endl;

    int currentBlockSize;
    for( int i = 0; i < segNum; i++ )
    {
        currentBlockSize = (i==segNum-1) ? lastSegSize : maxSegBlockSize_;
        dataBlock = getFreeSlot();
        if( !dataBlock.get() )
            LOG(ERROR) << "[FrameBuffer] RecvFrame: getFreeSlot error" << endl;


//        cout <<"Segme: " << i
//             << " size=" << currentBlockSize
//             << " data: " << (void*)frame.getBuf()+(i*maxSegBlockSize_)
//             << " ~ " << (void*)(frame.getBuf()+currentBlockSize)
//             << endl << endl;

        dataBlock->fillData(frame.getBuf()+(i*maxSegBlockSize_),currentBlockSize);

//        cout << "Block" << i
//             << " size=" << dataBlock->size()
//             << " data: " << (void*)dataBlock->dataPtr()
//             << " ~ " << (void*)(dataBlock->dataPtr()+dataBlock->size())
//             << endl << endl;
//        cout << "Rcev frame size = " << dataBlock->size()
//             << " data: " << (void*)dataBlock->dataPtr()
//             << " ~ " << (void*)(dataBlock->dataPtr()+dataBlock->size())
//             << endl << endl << endl;
//        for( int i = 0; i< dataBlock->size(); ++i )
//            cout << hex << dataBlock->dataPtr()[i] << " " ;

        ndn::Name segPrefix(framePrefix);
        segPrefix.append(NdnRtcUtils::componentFromInt(i));
        activeSlots_[segPrefix] = dataBlock;


        LOG(INFO) << "[FrameBuffer] Cached " << segPrefix.toUri()
                  << " ( Size = " << dataBlock->size()<<" )" << endl;
    }
}

ptr_lib::shared_ptr<SegmentData>
FrameBuffer::acquireSegment(const ndn::Interest& interest)
{
    lock_guard<recursive_mutex> scopedLock(syncMutex_);

    ptr_lib::shared_ptr<SegmentData> segment;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::iterator iter;
    iter = activeSlots_.find(interest.getName());
    if (iter==activeSlots_.end())
        return segment;

    ptr_lib::shared_ptr<DataBlock> segBlock = iter->second;
    //cout << "********** size:" << segBlock->size() << " &data=" << (void*)(segBlock->dataPtr()) <<  endl;
    uint32_t interestNonce_ = NdnRtcUtils::blobToNonce(interest.getNonce());
    int64_t interestArrivalMs_ = NdnRtcUtils::microsecondTimestamp();


    segment.reset(new SegmentData(segBlock->dataPtr(),
                                   segBlock->size(),
                                   (SegmentData::SegmentMetaInfo){interestNonce_,interestArrivalMs_,0}));
    return segment;
}

ptr_lib::shared_ptr<PrefixMetaInfo>
FrameBuffer::acquireFrameMeta(const ndn::Interest& interest)
{
    lock_guard<recursive_mutex> scopedLock(metaMutex_);

    ptr_lib::shared_ptr<PrefixMetaInfo> frameMeta;
    std::map<ndn::Name, ptr_lib::shared_ptr<PrefixMetaInfo> >::iterator iter;

    iter = activeFrameMeta_.find(interest.getName().getPrefix(-1));
    if ( iter != activeFrameMeta_.end() )
        frameMeta = iter->second;

    return frameMeta;
}

void
FrameBuffer::getCachedRange(FrameNumber& start, FrameNumber& end)
{
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::iterator istart, iend;
    istart = activeSlots_.begin();
    iend= activeSlots_.end();
    iend--;
    Namespacer::getFrameNumber(istart->first,start);
    Namespacer::getFrameNumber(iend->first,end);
}

//protected functions
//**************************************************
void
FrameBuffer::reset()
{
    ptr_lib::shared_ptr<DataBlock> segment;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::iterator it;
    for( it = activeSlots_.begin(); it != activeSlots_.end(); ++it )
    {
        segment = it->second;
        freeSlots_.push_back(segment);
    }
    activeSlots_.clear();


    ptr_lib::shared_ptr<PrefixMetaInfo> frameMeta;
    std::map<ndn::Name, ptr_lib::shared_ptr<PrefixMetaInfo> >::iterator iter;
    for( iter = activeFrameMeta_.begin(); iter != activeFrameMeta_.end(); ++iter )
    {
        frameMeta = iter->second;
        freeFrameMeta_.push_back(frameMeta);
    }
    activeFrameMeta_.clear();
}

void
FrameBuffer::initSlots()
{
    while( freeSlots_.size() <= bufSize_ )
    {
        ptr_lib::shared_ptr<DataBlock> segment;
        segment.reset(new DataBlock(maxSegmentSize_));
        freeSlots_.push_back(segment);
    }

    while( freeFrameMeta_.size() <= bufSize_ )
    {
        ptr_lib::shared_ptr<PrefixMetaInfo> frameMeta;
        frameMeta.reset(new PrefixMetaInfo);
        freeFrameMeta_.push_back(frameMeta);
    }

    LOG(INFO) << "[FrameBuffer] init Slots " << bufSize_
              << &(*this) << endl;
}

ptr_lib::shared_ptr<DataBlock>
FrameBuffer::getFreeSlot()
{
    ptr_lib::shared_ptr<DataBlock> segment;
    if( freeSlots_.size() )
    {
        segment = freeSlots_.at(freeSlots_.size()-1);
        freeSlots_.pop_back();
//        LOG(INFO) << "[FrameBuffer] getFree Slot left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
    }
    else
    {
        std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::iterator it;
        it = activeSlots_.begin();
        segment = (it)->second;
        ndn::Name name(it->first);
        activeSlots_.erase(it);
//        LOG(INFO) << "[FrameBuffer] getFree Slot left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
//        LOG(INFO) << "[FrameBuffer] getFree Slot EREASE " << name.toUri()
//                  << " left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
    }
    return segment;
}

ptr_lib::shared_ptr<PrefixMetaInfo>
FrameBuffer::getFreeFrameMeta()
{
    ptr_lib::shared_ptr<PrefixMetaInfo> frameMeta;
    if( freeFrameMeta_.size() )
    {
        frameMeta = freeFrameMeta_.at(freeFrameMeta_.size()-1);
        freeFrameMeta_.pop_back();
//        LOG(INFO) << "[FrameBuffer] getFree Slot left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
    }
    else
    {
        std::map<ndn::Name, ptr_lib::shared_ptr<PrefixMetaInfo> >::iterator it;
        it = activeFrameMeta_.begin();
        frameMeta = (it)->second;
        ndn::Name name(it->first);
        activeFrameMeta_.erase(it);
//        LOG(INFO) << "[FrameBuffer] getFree Slot left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
//        LOG(INFO) << "[FrameBuffer] getFree Slot EREASE " << name.toUri()
//                  << " left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
    }
    return frameMeta;
}
