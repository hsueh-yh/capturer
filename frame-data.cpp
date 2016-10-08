#include "frame-data.h"


//*****************************************************
//  MediaData
//*****************************************************

/**
  * @brief MediaData
  * @param
  */
BaseData::BaseData(unsigned int length, const unsigned char *rawData)
{
    copyFromRaw( length, rawData );
}

BaseData::BaseData(const BaseData &mediaData)
{
    copyFromRaw( mediaData.size(), mediaData.getData() );
}

BaseData::~BaseData()
{
    if( isDataCopied_ && data_ )
        free(data_);
}

void
BaseData::copyFromRaw(unsigned int dataLength, const unsigned char *rawData)
{
    length_ = dataLength;
    data_ = (unsigned char*) malloc ( length_ );
    memcpy( data_,rawData,length_ );
    isDataCopied_ = true;
}


//*****************************************************
//  SegmentData
//*****************************************************

/**
  * @brief Constructor
  * @param SegmentData, segment data block (without segment header)
  * @param dataSize, segment data block size
  * @param metadata, segment header, default {0,0,0}
  */
SegmentData::SegmentData( const unsigned char *segmentData,
                          const unsigned int dataSize,
                          SegmentMetaInfo metadata )
{
    unsigned int headerSize = getHeaderSize();
    length_ = dataSize + headerSize;
    isDataCopied_ = true;
    data_ = ( unsigned char* ) malloc ( length_ );
    memcpy( data_ + headerSize, segmentData, dataSize );
    *(SegmentHeader*)(data_) = metadata;
}


SegmentData::SegmentData(const unsigned int dataSize)
{
    if( dataSize >= getHeaderSize() )
    {
        data_ = ( unsigned char* ) malloc ( dataSize );
        isDataCopied_ = true;
        length_ = 0;
    }
}

int
SegmentData::fillSegmentData( unsigned char* data, unsigned int length )
{
    if( length > 0 && data )
    {
        length_ = length + getHeaderSize();
        memcpy(data_+getHeaderSize(),data,length);
    }
}

/**
  * @brief initialize a raw data to SegmentData
  * @param dataLength, rawData block length ( with header )
  * @param rawData, header + data block
  */
int SegmentData::initFromRawData(unsigned int dataLength,
                    const unsigned char* rawData)
{
    if( dataLength > getHeaderSize() && rawData )
    {
        length_ = dataLength;
        data_ = const_cast<unsigned char*>(rawData);
        return true;
    }
    return false;
}

int
SegmentData::segmentDataFromRaw(unsigned int dataLength,
                               const unsigned char* rawData,
                               SegmentData &segmentData)
{
    segmentData.initFromRawData(dataLength, rawData);
}


//*****************************************************
//  FrameData
//*****************************************************

FrameData::FrameData(unsigned char *data,
                     unsigned int length,
                     FrameMetadata *metaData)
{
    if( length > 0 && data && metaData )
    {
        unsigned int metaSize = getHeaderSize();
        length_ = length + metaSize;
        isDataCopied_ = true;
        data_ = (unsigned char*) malloc ( length_ );
        memcpy(data_,metaData,metaSize);
        memcpy(data_+metaSize, data, length);
    }
}

int
FrameData::initFromRawData(unsigned int dataLength, const unsigned char *rawData)
{
    if( dataLength > getHeaderSize() && rawData )
    {
        length_ = dataLength;
        data_ = const_cast<unsigned char*>(rawData);
        return true;
    }
    return false;
}
