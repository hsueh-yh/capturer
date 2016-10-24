
#include <math.h>

#include "frame-buffer.h"
#include "utils.h"
#include "name-components.h"
#include "namespacer.h"

FrameBuffer::FrameBuffer(ndn::Name basePrefix) :
    maxSegmentSize_(ndn::Face::getMaxNdnPacketSize()),
    maxSegBlockSize_(ndn::Face::getMaxNdnPacketSize()-SegmentData::getHeaderSize()),
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

    ptr_lib::shared_ptr<DataBlock> dataBlock;

    int segNum = ceil( (double)size / (double)maxSegBlockSize_ );
    int lastSegSize = size % maxSegBlockSize_;

    uint8_t nalHead = data[4];

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

        dataBlock->fillData(data+(i*maxSegBlockSize_),currentBlockSize);

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
        dataPrefix.append(NdnRtcUtils::componentFromInt(++lastPkgNo_));
        std::vector<uint8_t> value;
        value.push_back(nalHead);
        dataPrefix.append(value);
        activeSlots_[dataPrefix] = dataBlock;


        LOG(INFO) << "[FrameBuffer] Cached " << dataPrefix.toUri()
                  << " ( Size = " << dataBlock->size()<<" )" << endl;
    }
}

ptr_lib::shared_ptr<DataBlock>
FrameBuffer::acquireData(const ndn::Interest& interest)
{
    lock_guard<recursive_mutex> scopedLock(syncMutex_);

    ptr_lib::shared_ptr<DataBlock> data;
    std::map<ndn::Name, ptr_lib::shared_ptr<DataBlock> >::reverse_iterator re_iter;
    //iter = activeSlots_.find(interest.getName());

    ptr_lib::shared_ptr<ndn::Name> name;
    for( re_iter = activeSlots_.rbegin(); re_iter != activeSlots_.rend(); ++re_iter )
    {
        name = re_iter->first;
        if( name->getPrefix(name->size()-2).equals(interest.getName()))
            break;
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
