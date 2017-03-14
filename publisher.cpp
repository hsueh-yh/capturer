#include <arpa/inet.h>

#include "publisher.h"
#include "name-components.h"
#include "namespacer.h"
#include "glogger.h"
//#include "utils.h"
#include "mtndn-utils.h"
#include <thread>

//using namespace std::placeholders;
using namespace func_lib;

#define REQUEST_FIRST_FRAME_ 0

/*
Publisher::Publisher (const string streamName,
                      boost::asio::io_service& ioService,
                      ptr_lib::shared_ptr<FaceWrapper> face,
                      KeyChain &keyChain, const Name &certificateName) :
    streamPrefix_(streamName),
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
*/

Publisher::Publisher(const GeneralParams &generalParams):
    generalParams_(generalParams),
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


int
Publisher::init(const PublisherSettings &settings, const MediaThreadParams* videoThreadParams)
{
    settings_ = settings;
    face_ = settings_.faceProcessor_->getFaceWrapper();

    frameBuffer_.reset(new FrameBuffer(getStreamName()));
    frameBuffer_->init(100);    //default 100

    LOG(INFO) << "Register prefix " << getStreamName() << endl;
    registedId_ = face_->registerPrefix(
                getStreamName(),
                (const OnInterestCallback&)func_lib::bind(
                                                   &Publisher::onInterest,
                                                   this,
                                                   func_lib::_1,
                                                   func_lib::_2,
                                                   func_lib::_3,
                                                   func_lib::_4,
                                                   func_lib::_5),
                (const OnRegisterFailed&)func_lib::bind(&Publisher::onRegisterFailed,
                                                         this,
                                                         func_lib::_1)/*,
                (const OnRegisterSuccess&)bind(&Publisher::onRegisterSuccess,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2)*/ );

    if( registedId_ != 0 )
    {
        LOG(INFO) << "Register prefix SUCCESS ( ID = " << registedId_ << " )" << endl;
    }
    else
    {
        LOG(INFO) << "Register prefix FAILED ( ID = " << registedId_ << " )" << endl;
    }

    LOG(INFO) << "Publisher initialize DONE" << endl;

    return RESULT_OK;
}

int
Publisher::start()
{
}

int
Publisher::stop()
{
    LOG(INFO) << "[Publisher] Stop" << std::endl;
    face_->unregisterPrefix(registedId_);
    //ioService_.stop();

    if(isBackupYUV)
        fclose(fp_yuv);
    if (isBackup264)
        fclose(fp_264);
}


////////////////////////////////////////////////////////////////
// onInterest.
void
Publisher::onInterest(
        const ptr_lib::shared_ptr<const Name>& prefix,
        const ptr_lib::shared_ptr<const Interest>& interest,
        Face& face,
        uint64_t interestFilterId,
        const ptr_lib::shared_ptr<const InterestFilter>& filter )
{
    LOG(INFO) << "RCVE " << prefix->toUri() << endl;
}


/*
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
        Data data(requestName.append(MtNdnUtils::componentFromInt(frameBuffer_->getLastPktNo())));
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
    requestName.append(MtNdnUtils::componentFromInt(lastPktNo));
    //requestName.append(MtNdnUtils::componentFromInt(lastPktNo));
    Data ndnData(requestName);

    ndnData.setContent( content );

    keyChain_.sign(ndnData, certificateName_);


    face.putData(ndnData);
    ++responseCount_;

    LOG(INFO) << "Response: " << requestName.toUri()
         << " ( size: " << dec << ndnData.getContent().size() << " )"
         << endl << endl;
}
*/

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
