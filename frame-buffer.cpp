
#include <math.h>

#include "frame-buffer.h"
#include "utils.h"
#include "name-components.h"
#include "namespacer.h"

FrameBuffer::FrameBuffer(ndn::Name basePrefix) :
    maxNdnPktSize_(ndn::Face::getMaxNdnPacketSize()-500),
    basePrefix_(basePrefix),
    bufSize_(200),
    lastPkgNo_(0),
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
FrameBuffer::appendData(const unsigned char* data, const unsigned int size)
{
    lock_guard<recursive_mutex> scopedLock(syncMutex_);
    //fwrite(frame.getFrameData(),1,frame.getDataBlockSize(),fp);

    ptr_lib::shared_ptr<DataBlock> dataBlockSlot;

    int segNum = ceil( (double)size / (double)maxNdnPktSize_ );
    unsigned int lastSegSize = size % maxNdnPktSize_;
    if( 0 == lastSegSize )
        lastSegSize = maxNdnPktSize_;

    uint8_t nalHead = data[4];
    if( nalHead == 0x67)
        lastSeqNo_ = lastPkgNo_+1;

    int currentBlockSize;
    for( int i = 0; i < segNum; ++i )
    {
        currentBlockSize = (i==segNum-1) ? lastSegSize : maxNdnPktSize_;
        dataBlockSlot = getFreeSlot();
        if( !dataBlockSlot.get() )
            LOG(ERROR) << "[FrameBuffer] RecvFrame: getFreeSlot error" << endl;


//        cout <<"Segme: " << i
//             << " size=" << currentBlockSize
//             << " data: " << (void*)frame.getBuf()+(i*maxSegBlockSize_)
//             << " ~ " << (void*)(frame.getBuf()+currentBlockSize)
//             << endl << endl;

        dataBlockSlot->fillData(data+(i*maxNdnPktSize_),currentBlockSize);

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

        ndn::Name dataPrefix(basePrefix_);
        dataPrefix.append(NdnUtils::componentFromInt(++lastPkgNo_));
        dataPrefix.append(NameComponents::NameComponentNalMetainfo);
        std::vector<uint8_t> value;
        value.push_back(nalHead);
        dataPrefix.append(value);
        activeSlots_[dataPrefix] = dataBlockSlot;

        LOG_IF(ERROR, dataBlockSlot->size()<=0)
                << "[FrameBuffer] Cached ERROR " << dataPrefix.toUri()
                << " seg:"<< i << " Num:" << segNum
                << " ( Size = " << dec << currentBlockSize<<" of " << size << " )" << endl;

        LOG(INFO) << "[FrameBuffer] Cached " << dataPrefix.toUri()
                  << " (" << i << "-" << segNum
                  << ") [" << dec << dataBlockSlot->size()<<"-" << size << "]" << endl;
        //NdnUtils::printMem("cache",dataBlock->dataPtr(),20);
    }
}

ptr_lib::shared_ptr<DataBlock>
FrameBuffer::acquireData(const ndn::Interest& interest, ndn::Name& nalType )
{
    lock_guard<recursive_mutex> scopedLock(syncMutex_);

    ptr_lib::shared_ptr<DataBlock> data;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::reverse_iterator re_iter;
    //iter = activeSlots_.find(interest.getName());

    ndn::Name name;
    for( re_iter = activeSlots_.rbegin(); re_iter != activeSlots_.rend(); ++re_iter )
    {
        name = re_iter->first;
        if( interest.getName().equals(name.getPrefix(interest.getName().size())))
        {
            nalType = name.getSubName(-2);
            break;
        }
    }
    if (re_iter!=activeSlots_.rend())   //if found
    {
        data = re_iter->second;
        //cout << "********** size:" << segBlock->size() << " &data=" << (void*)(segBlock->dataPtr()) <<  endl;
    }
    return data;
}

void
FrameBuffer::getCachedRange(FrameNumber& start, FrameNumber& end)
{
    start = startPkgNo_;
    end = lastPkgNo_;
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
}

void
FrameBuffer::initSlots()
{
    while( freeSlots_.size() <= bufSize_ )
    {
        ptr_lib::shared_ptr<DataBlock> segment;
        segment.reset(new DataBlock(maxNdnPktSize_));
        freeSlots_.push_back(segment);
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
        startPkgNo_ += 1;
//        LOG(INFO) << "[FrameBuffer] getFree Slot left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
//        LOG(INFO) << "[FrameBuffer] getFree Slot EREASE " << name.toUri()
//                  << " left = " << freeSlots_.size()
//                  << " active = " << activeSlots_.size() << endl;
    }
    return segment;
}
