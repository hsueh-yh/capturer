#include <arpa/inet.h>

#include "publisher.h"
#include "name-components.h"
#include "namespacer.h"
#include "logger.hpp"
#include "utils.h"

using namespace std::placeholders;

#define REQUEST_FIRST_FRAME_ 0

Publisher::Publisher (boost::asio::io_service &ioService,
                      ptr_lib::shared_ptr<ThreadsafeFace> face,
                      KeyChain &keyChain, const Name& certificateName ) :
    streamPrefix_("/com/monitor/location1/stream0"),
    ioService_(ioService),
    face_(face),
    keyChain_ ( keyChain ),
    certificateName_ ( certificateName ),
    registedId_(0),
    responseCount_ ( 0 ),
    requestCounter_(0),
    fp_yuv(NULL),
    fp_264(NULL),
    isBackupYUV(false),
    isBackup264(false),
    cacheSize_(200),
    cachedBegin_(0),
    cachedEnd_(0),
    currentFrameNo_(0)

{
    if( isBackupYUV )
    {
        fp_yuv = fopen ( "backup.yuv", "wb+" );
        if ( fp_yuv == NULL )
            LOG(INFO) << "Open file backup.yuv error!" << endl;
    }
    if( isBackup264 )
    {
        fp_264 = fopen ( "backup.264", "wb+" );
        if ( fp_264 == NULL )
            LOG(INFO) << "Open file backup.264 error!" << endl;
    }

    LOG(INFO) << "Publisher Constructor DONE." << endl;
}


Publisher::~Publisher ()
{
    if( stat != 0 )
        stop();
    if( isBackup264 && fp_264 )
        fclose ( fp_264 );
    if( isBackupYUV && fp_yuv )
        fclose ( fp_yuv );

    LOG(INFO) << "Publisher Destructor DONE." << endl;
}


bool
Publisher::init()
{
    frameBuffer_.reset(new FrameBuffer(getStreamVideoPrefix()));
    frameBuffer_->init(100);    //default 100
    LOG(INFO) << "Register prefix " << streamPrefix_.toUri() << endl;
    registedId_ = face_->registerPrefix
          ("/com/monitor/location1" /*streamPrefix_*/,
           (const OnInterestCallback&)bind(&Publisher::onInterest, this, _1, _2, _3, _4, _5),
           //onRegisterFailed);
           (const OnRegisterFailed&)bind(&Publisher::onRegisterFailed, this, _1),
           (const OnRegisterSuccess&)bind(&Publisher::onRegisterSuccess, this, _1,_2) );

    if( registedId_ != 0 )
    {
        LOG(INFO) << "Register prefix SUCCESS ( ID = " << registedId_ << " )" << endl;
    }
    else
    {
        LOG(INFO) << "Register prefix FAILED ( ID = " << registedId_ << " )" << endl;
    }

    LOG(INFO) << "Capturer initialize " << endl;
    capturer.init();
    capturer.start();

    LOG(INFO) << "Encoder initialize " << endl;
    encoder.init(AV_CODEC_ID_H264);
    outbufYUV = (unsigned char*) malloc (width*height*3/2);

    outbuf264 = (unsigned char*) malloc (width*height*3/2);

    stat = 1;
    LOG(INFO) << "Publisher initialize DONE" << endl;
}

int
Publisher::start()
{
    unsigned int i = -1;
    LOG(INFO) << "[Publisher] Start " << std::endl;
    while( 1 )
    {
        excuteCapture( /*++currentFrameNo_*/ );
        usleep(30000);
    }
    stop();
}

int
Publisher::stop()
{
    LOG(INFO) << "[Publisher] Stop" << std::endl;
    face_->removeRegisteredPrefix(registedId_);
    ioService_.stop();

    if(isBackupYUV)
        fclose(fp_yuv);
    if (isBackup264)
        fclose(fp_264);

    encoder.stop();
    capturer.stop();

    stat = 0;
}


// onInterest.
void Publisher::onInterest
                      ( const ptr_lib::shared_ptr<const Name>& prefix,
                        const ptr_lib::shared_ptr<const Interest>& interest,
                        Face& face,
                        uint64_t interestFilterId,
                        const ptr_lib::shared_ptr<const InterestFilter>& filter )
{
    ++requestCounter_;
    //cout << "Got an interest..." << endl;

    Name requestName(interest->getName());
    Name responseName(streamPrefix_);
    responseName.append(requestName.getSubName(responseName.size()));

    LOG(INFO) << "Request : " << requestName.toUri() << endl;

    ndn::Name metaName(streamPrefix_);
    metaName.append(NameComponents::NameComponentStreamMetaIdx);

    // request metaInfo
    //if( requestName.getPrefix(metaName.size()).equals(metaName))
    if( 0 <= Namespacer::findComponent(requestName,NameComponents::NameComponentStreamMetaIdx))
    {
        Data data(requestName.append(NdnUtils::componentFromInt(frameBuffer_->getLastPktNo())));
        face.putData(data);
        ++responseCount_;

        LOG(INFO) << "Response: MetaInfo " << requestName.toUri() << endl << endl;
        return ;
    }

    // data not exist
    ptr_lib::shared_ptr<DataBlock> naluData;
    ndn::Name nalType;
    //naluData = frameBuffer_->acquireData( *interest.get(), nalType );
    naluData = frameBuffer_->acquireData( responseName, nalType );

    FrameNumber firstPktNo, lastPktNo;
    frameBuffer_->getCachedRange(firstPktNo, lastPktNo);
    if( !naluData )
    {
        cout << "No Data: " << interest->getName().toUri()
             << " Cached frameNo: " << firstPktNo << " to " << lastPktNo << endl;
        //Data emptyData(requestName);
        //face.putData(emptyData);
        return ;
    }

    const Blob content ( naluData->dataPtr(), naluData->size() );

    requestName.append(NameComponents::NameComponentNalIdx);
    requestName.append(nalType);
    requestName.append(NdnUtils::componentFromInt(lastPktNo));
    //requestName.append(NdnUtils::componentFromInt(lastPktNo));
    Data ndnData(requestName);

    ndnData.setContent( content );

    keyChain_.sign(ndnData, certificateName_);


    face.putData(ndnData);
    ++responseCount_;

    LOG(INFO) << "Response: " << requestName.toUri()
         << " ( size: " << dec << ndnData.getContent().size() << " )"
         << endl << endl;
}


// onRegisterFailed.
void
Publisher::onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix )
{
    LOG(INFO) << "Register failed for prefix " << prefix->toUri() << endl;
}


void
Publisher::onRegisterSuccess(const ptr_lib::shared_ptr<const Name>& prefix,
                             uint64_t registeredPrefixId)
{
    LOG(INFO) << "onRegister Success " << prefix->toUri() << " " << registeredPrefixId << endl;
}
//protected functions
//**************************************************

void
Publisher::view()
{}

void
Publisher::excuteCapture()
{
    int64_t millisecondTimestamp;
    capturer.getFrame(outbufYUV,outlenYUV,millisecondTimestamp);

    if( outlenYUV == 0 )
    {
        LOG(WARNING) << "Capturer fail" << endl;
        return ;
    }
    // get YUV frame
    else
    {
        //cout << "Get YUV " <<endl;
        if(isBackupYUV && fp_yuv )
            fwrite(outbufYUV,1,outlenYUV,fp_yuv);

        // encode
        encoder.getFrame(outbufYUV, outlenYUV, outbuf264, outlen264);
        if (outlen264 == 0)
        {
            //cout << "NO 264" << endl;
        }
        else // get NALU data
        {
            //cout << "Get 264 " << " ( size = " << outlen264 << " )" <<endl;
            ++currentFrameNo_;
            frameBuffer_->appendData((const unsigned char*)outbuf264, (const unsigned int)outlen264,millisecondTimestamp);

            if( isBackup264 && fp_264 )
            {
                fwrite(outbuf264,1,outlen264,fp_264);
            }
        }
    }//else
    //cout << endl;
}

