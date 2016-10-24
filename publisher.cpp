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
    currentFrameNo_(-1)

{
    if( isBackupYUV )
    {
        fp_yuv = fopen ( "backup.yuv", "wb+" );
        if ( fp_yuv == NULL )
            cout << "Open file backup.yuv error!" << endl;
    }
    if( isBackup264 )
    {
        fp_264 = fopen ( "backup.264", "wb+" );
        if ( fp_264 == NULL )
            cout << "Open file backup.264 error!" << endl;
    }
    if( fp_yuv == NULL && fp_264 == NULL )
        LOG(INFO) << "Publisher open file error!" << endl;
    else
        LOG(INFO) << "Publisher open file SUCCESS!" << endl;


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
    framebuffer_.reset(new FrameBuffer);
    framebuffer_->init(500);    //default 100
    LOG(INFO) << "Register prefix " << streamPrefix_.toUri() << endl;
    registedId_ = face_->registerPrefix
          (streamPrefix_,
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
        excuteCapture( ++currentFrameNo_ );
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
//    FrameNumber frameNo;
//    SegmentNumber segNo;
//    Namespacer::getSegmentationNumbers(requestName,frameNo,segNo);

    // not in cache
//    if( !isCached(frameNo) )
//        return ;

    LOG(INFO) << "Request : " << requestName.toUri() << endl;

    ndn::Name metaName(streamPrefix_);
    metaName.append(NameComponents::NameComponentStreamFrameVideo);
    metaName.append(NameComponents::NameComponentStreamMetainfo);

    // request metaInfo
    if( requestName.getPrefix(requestName.size()-1).equals(metaName))
    {
        Data data(requestName.append(NdnRtcUtils::componentFromInt(currentFrameNo_)));
        face.putData(data);
        ++responseCount_;

        LOG(INFO) << "Response: MetaInfo " << requestName.toUri() << endl;
        return ;
    }

    // data not exist return empty data
    ptr_lib::shared_ptr<SegmentData> segment;
    segment = framebuffer_->acquireSegment( *interest.get() );

    if( !segment )
    {
        FrameNumber start, end;
        framebuffer_->getCachedRange(start, end);
        cout << "No segment: " << interest->getName().toUri()
             << " Cached frameNo: " << start << " to " << end << endl;
        Data emptyData(requestName);
        face.putData(emptyData);
        return ;
    }


    ptr_lib::shared_ptr<PrefixMetaInfo> prefixMeta;
    prefixMeta = framebuffer_->acquireFrameMeta(*interest.get());

    cout << "***************"
         << prefixMeta->totalSegmentNum_ << " "
         << prefixMeta->playbackNo_ << " "
         << prefixMeta->deltaFrameNo_<< endl;
    Namespacer::setPrefixMetaInfo(requestName,*prefixMeta.get());

    const Blob content ( segment->getBuf(), segment->size() );

    // Make and sign a Data packet.
    Data data(requestName);

    data.setContent( content );

    keyChain_.sign(data, certificateName_);

//    unsigned int len = data.getContent().size();
//    cout << len << endl;
//    for( int i = 0; i < len; ++i )
//    {
//        printf("%X ",data.getContent().buf()[i]);
//        fflush(stdout);
//    }

    face.putData(data);
    ++responseCount_;

    LOG(INFO) << "Response: " << requestName.toUri()
         << " ( size: "
         << segment->size() << " )"
         << endl << endl;



//	for( int i = 0; i <30; i++ )
//		printf("%2X ",data.getContent().buf()[i]);
//	cout << endl << endl;

    //fwrite(data.getContent().buf(), data.getContent().size(),1,outfp);
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
Publisher::excuteCapture( unsigned int frameNo )
{
    capturer.getFrame(outbufYUV,outlenYUV);
    if( outlenYUV != 0 )
    {
        //cout << "get YUV " <<endl;
        if(isBackupYUV && fp_yuv )
            fwrite(outbufYUV,1,outlenYUV,fp_yuv);
    }
    else
    {
        LOG(WARNING) << "Capturer fail" << endl;
        return ;
    }

    if( outlenYUV != 0 )
    {
        encoder.getFrame(outbufYUV, outlenYUV, outbuf264, outlen264);
        if (outlen264 == 0)
            cout << "no 264" << endl;
        else
        {
            FrameData::FrameMetadata frameHeader;
            frameHeader.encodedHeight_ = encoder.getEncodeHight();
            frameHeader.encodedWidth_ = encoder.getEncodeWidth();
            frameHeader.packetRate_ = encoder.getPacketRate();
            frameHeader.timestamp_ = NdnRtcUtils::microsecondTimestamp();
            frameHeader.unixTimestamp_ = NdnRtcUtils::unixTimestamp();
            FrameData frame( outbuf264, outlen264, &frameHeader );
            ndn::Name framePrefix(streamPrefix_);
            framePrefix.append(NameComponents::NameComponentStreamFrameVideo);
            framePrefix.append(NdnRtcUtils::componentFromInt(frameNo));
            framebuffer_->recvFrame(frame, framePrefix);

            if( isBackup264 && fp_264 )
            {
                //cout << "get 264 " << " ( size = " << outlen264 << " )" <<endl;
                fwrite(outbuf264,1,outlen264,fp_264);
            }
        }
    }
    //cout << endl;
}


static void
onRegisterFailed(const ptr_lib::shared_ptr<const Name>& prefix)
{
  cout << "Register failed for prefix " << prefix->toUri() << endl;
}
