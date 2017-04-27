#include <arpa/inet.h>

#include "video-publisher-stream.h"
#include "name-components.h"
#include "namespacer.h"
#include "glogger.h"
//#include "utils.h"
#include "mtndn-utils.h"
#include <thread>

//using namespace std::placeholders;
using namespace func_lib;

#define REQUEST_FIRST_FRAME_ 0


VideoPublisherStream::VideoPublisherStream( const GeneralParams &generalParams,
                                IExternalCapturer* const externalCapturer):
    Publisher(generalParams),
    videoCapturer_(new VideoCapturer(externalCapturer)),
    videoEncoder_(new VideoEncoder)
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

    LOG(INFO) << "VideoPublisherStream Constructor DONE." << endl;
}


VideoPublisherStream::~VideoPublisherStream ()
{
    //if( stat != 0 )
        stop();
    if( isBackup264 && fp_264 )
        fclose ( fp_264 );
    if( isBackupYUV && fp_yuv )
        fclose ( fp_yuv );

    LOG(INFO) << "VideoPublisherStream Destructor DONE." << endl;
}


int
VideoPublisherStream::init(const PublisherSettings &settings,
                           const MediaThreadParams* videoThreadParams)
{
    LOG(INFO) << "VideoPublihser init" << std::endl;
    if( RESULT_NOT_OK(Publisher::init(settings,videoThreadParams)) )
        return RESULT_ERR;

    videoThreadParams_ = *(VideoThreadParams*)(videoThreadParams);

    videoCapturer_->init();
    videoCapturer_->registerRawFrameConsumer(videoEncoder_.get());
    videoEncoder_->init(videoThreadParams_.coderParams_);
    videoEncoder_->setEncodedFrameConsumer(this);

    return RESULT_OK;
}

int
VideoPublisherStream::start()
{
    LOG(INFO) << "[VideoPublisherStream] Start " << std::endl;

    videoCapturer_->start();

    return RESULT_OK;
}

int
VideoPublisherStream::stop()
{
    LOG(INFO) << "[VideoPublisherStream] Stop" << std::endl;
    face_->unregisterPrefix(registedId_);

    if(isBackupYUV)
        fclose(fp_yuv);
    if (isBackup264)
        fclose(fp_264);

    return RESULT_OK;
}

void
VideoPublisherStream::onEncodedFrameDelivered(vector<uint8_t> &encodedImage,
                                   int64_t captureTimestamp)
{
    //cout << "Get 264 " << " ( size = " << outlen264 << " )" <<endl;
    ++currentFrameNo_;

    // caching
    int res = frameBuffer_->appendData(
                (const unsigned char*)encodedImage.data(),
                (const unsigned int)encodedImage.size(),
                captureTimestamp);

    // also send frame (packet(s)) to network
    if( req_.isStreaming() )
    {
        int64_t lifeTime = MtNdnUtils::millisecSinceEpoch();
        if( lifeTime >= req_.getFinishTsMs() )
        {
            req_.setState( false ); // stop streaming
        }
        else
        {
            FrameNumber firstPktNo, lastPktNo;
            frameBuffer_->getCachedRange(firstPktNo, lastPktNo);
            for( FrameNumber pktNo = lastPktNo-res+1; pktNo <= lastPktNo; ++pktNo )
            {
                pushFrame( Name(getStreamName()).append(
                               MtNdnUtils::componentFromInt(pktNo)) );
            }
        }
    }

    if( isBackup264 && fp_264 )
    {
        fwrite(encodedImage.data(),1,encodedImage.size(),fp_264);
    }
}


//protected
//**************************************************
void
VideoPublisherStream::processInterest(
        const ptr_lib::shared_ptr<const Name>& prefix,
        const ptr_lib::shared_ptr<const Interest>& interest,
        Face& face, uint64_t interestFilterId,
        const ptr_lib::shared_ptr<const InterestFilter>& filter)
{
    LOG(INFO) << "RCVE " << interest->getName().toUri() << endl;
    ++requestCounter_;

    int64_t lifeTime = interest->getInterestLifetimeMilliseconds();
    bool isReqStream = (lifeTime >= 4000);

    //request stream
    if( isReqStream )
    {
        // start streaming and set dead time
        req_.setState(true);
        req_.setFinishTsMs( max(req_.getFinishTsMs(), MtNdnUtils::millisecSinceEpoch() + lifeTime) );

        //processRequest(prefix, interest, face, interestFilterId, filter);
    }
    //request packet
    else
    {
        Name requestName( interest->getName().toUri() );
        bool isReqMeta = (0 <= Namespacer::findComponent(
                              requestName,
                              NameComponents::NameComponentStreamMetaIdx) );

        // request meta
        if( isReqMeta )
        {
            Data data(requestName.append(MtNdnUtils::componentFromInt(frameBuffer_->getLastPktNo())));
            face_->getFace()->putData(data);
            ++responseCount_;

            LOG(INFO) << "Response: MetaInfo " << requestName.toUri() << endl << endl;
            return ;
        }
        //request frame
        else
        {
            pushFrame( interest->getName() );
        }
    }
}

//private
//**************************************************
void
VideoPublisherStream::pushFrame( const Name& reqName )
{
    // request stream data
    Name requestName(reqName);
    ptr_lib::shared_ptr<DataBlock> naluData;
    Name nalType;

    naluData = frameBuffer_->acquireData( requestName, nalType );

    FrameNumber firstPktNo, lastPktNo;
    frameBuffer_->getCachedRange(firstPktNo, lastPktNo);

    //no data
    if( !naluData )
    {
        cout << "No Data: " << requestName.toUri()
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

    face_->getFace()->getCommandKeyChain()->sign(
                ndnData,
                face_->getFace()->getCommandCertificateName());


    face_->getFace()->putData(ndnData);
    ++responseCount_;

    LOG(INFO) << "Response: " << requestName.toUri()
         << " ( size: " << dec << ndnData.getContent().size() << " )"
         << endl << endl;
}
