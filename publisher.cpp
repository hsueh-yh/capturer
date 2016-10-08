#include <arpa/inet.h>

#include "publisher.h"
//#include "namespacer.h"
#include "logger.hpp"


#define REQUEST_FIRST_FRAME_ 0

Publisher::Publisher ( KeyChain &keyChain, const Name& certificateName ) :
    streamPrefix_("/com/monitor/location1/stream0"),
    keyChain_ ( keyChain ),
    certificateName_ ( certificateName ),
    responseCount_ ( 0 ),
    requestCounter_(0),
    isBackupYUV(true),
    isBackup264(true),
    cacheSize_(200),
    cachedBegin_(0),
    cachedEnd_(0)

{
    if( isBackup264 )
    {
        fp_264 = fopen ( "backup.264", "rb" );
        if ( fp_264 == NULL )
            cout << "Open file backup.264 error!" << endl;
    }
    if( isBackupYUV )
    {
        fp_yuv = fopen ( "backup.yuv", "rb" );
        if ( fp_yuv == NULL )
            cout << "Open file backup.yuv error!" << endl;
    }
}


Publisher::~Publisher ()
{
    if( isBackup264 )
        fclose ( fp_264 );
    if( isBackupYUV )
        fclose ( fp_yuv );
    //fclose ( outfp );
}


bool
Publisher::init()
{
    capturer.init();
    capturer.start();
    encoder.init(AV_CODEC_ID_H264);
    outbufYUV = (unsigned char*) malloc (width*height*3/2);

    outbuf264 = (unsigned char*) malloc (width*height*3/2);

    stat = 1;
}

int
Publisher::start()
{
    excuteCapture();
}

int
Publisher::stop()
{
    if(isBackupYUV)
        fclose(fp_yuv);
    if (isBackup264)
        fclose(fp_264);

    encoder.stop();
    capturer.stop();

    stat = 0;
}


// onInterest.
void Publisher::operator()
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

    cout << "Request : " << requestName.toUri() << endl;


    boost::shared_ptr<SegmentData> segment;
    segment = framebuffer_->acquireSegment( *interest.get() );

    if( !segment )
    {
        cout << "No segment: " << interest->getName().toUri() << endl;
        return ;
    }


    const Blob content ( segment->getData(), segment->size() );

    // Make and sign a Data packet.
    Data data(requestName);

    data.setContent( content );

    keyChain_.sign(data, certificateName_);

    cout << "Response: " << requestName.toUri()
         << " size: "
         << segment->size()
         << endl;

    face.putData(data);
    ++responseCount_;

//	for( int i = 0; i <30; i++ )
//		printf("%2X ",data.getContent().buf()[i]);
//	cout << endl << endl;

    //fwrite(data.getContent().buf(), data.getContent().size(),1,outfp);
}


// onRegisterFailed.
void
Publisher::operator()(const ptr_lib::shared_ptr<const Name>& prefix )
{
    ++responseCount_;
    cout << "Register failed for prefix " << prefix->toUri() << endl;
}


//protected functions
//**************************************************

void
Publisher::view()
{}


void
Publisher::excuteCapture()
{
    capturer.getFrame(outbufYUV,outlenYUV);
    if( outlenYUV != 0 )
    {
        cout << "get YUV " <<endl;
        if(isBackup264)
            fwrite(outbufYUV,1,outlenYUV,fp_yuv);
    }
    else
    {
        LOG(WARNING) << "Capturer fail" << endl;
        return ;
    }

    if( isBackup264 && outlenYUV != 0 )
    {
        encoder.getFrame(outbufYUV, outlenYUV, outbuf264, outlen264);
        if( outlen264 != 0 )
        {
            cout << "get 264 "<< " ( size = " << outlen264 << " )" <<endl;
            fwrite(outbuf264,1,outlen264,fp_264);
        }
        else
            cout << "not 264" << endl;
    }
    //cout << endl;
}

