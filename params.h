#ifndef PARAMS_H
#define PARAMS_H

#include <sstream>
#include <vector>

namespace ndnlog {
    typedef enum _NdnLoggerLevel {
        NdnLoggerLevelTrace = 0,
        NdnLoggerLevelDebug = 1,
        NdnLoggerLevelStat = 2,
        NdnLoggerLevelInfo = 3,
        NdnLoggerLevelWarning = 4,
        NdnLoggerLevelError = 5
    } NdnLoggerLevel;

    typedef enum _NdnLoggerDetailLevel {
        NdnLoggerDetailLevelNone = NdnLoggerLevelError+1,
        NdnLoggerDetailLevelDefault = NdnLoggerLevelInfo,
        NdnLoggerDetailLevelStat = NdnLoggerLevelStat,
        NdnLoggerDetailLevelDebug = NdnLoggerLevelDebug,
        NdnLoggerDetailLevelAll = NdnLoggerLevelTrace
    } NdnLoggerDetailLevel;
}

class Params {
public:
    virtual ~Params(){}
    virtual void write(std::ostream &os) const {}

    friend std::ostream &operator<<(std::ostream &os, Params const &p)
    {
        p.write(os);
        return os;
    }
};


// video thread parameters
class VideoCoderParams : public Params {
public:
    double codecFrameRate_;
    unsigned int gop_;
    unsigned int startBitrate_, maxBitrate_;
    unsigned int encodeWidth_, encodeHeight_;
    bool BFramesOn_, dropFramesOn_;

    void write(std::ostream &os) const
    {
        os
        << codecFrameRate_ << "FPS; GOP: "
        << gop_ << "; Start bitrate: "
        << startBitrate_ << "kbit/s; Max bitrate:"
        << maxBitrate_ << "kbit/s; "
        << encodeWidth_ << "x" << encodeHeight_ << "; BFrm: "
        << (BFramesOn_?"YES":"NO" ) << "; Drop:"
        << (dropFramesOn_?"YES":"NO");
    }
};

// video thread parameters
class VideoCapturerParams : public Params {
public:
    std::string dev_;
    std::string format_;
    double frameRate_;
    unsigned int width_, height_;

    void write(std::ostream &os) const
    {
        os
        << "Dev: " << dev_
        << "; format: " << format_
        << "; FrameRate: " << frameRate_ << "FPS; "
        << width_ << "x" << height_ << ";";
    }
};

// media thread parameters
class MediaThreadParams : public Params {
public:
    virtual ~MediaThreadParams(){}

    std::string threadName_ = "";

    virtual void write(std::ostream& os) const
    {
        os << "name: " << threadName_;
    }

    virtual MediaThreadParams* copy()
    {
        MediaThreadParams *params = new MediaThreadParams();
        *params = *this;
        return params;
    }
};

// video thread parameters
class VideoThreadParams : public MediaThreadParams {
public:
    VideoCoderParams coderParams_;
    double deltaAvgSegNum_, deltaAvgParitySegNum_;
    double keyAvgSegNum_, keyAvgParitySegNum_;

    void write(std::ostream& os) const
    {
        MediaThreadParams::write(os);

        os << "; " << coderParams_;
    }
    MediaThreadParams*
    copy()
    {
        VideoThreadParams *params = new VideoThreadParams();
        *params = *this;
        return params;
    }
};

// audio thread parameters
class AudioThreadParams : public MediaThreadParams {
public:
    MediaThreadParams*
    copy()
    {
        AudioThreadParams *params = new AudioThreadParams();
        *params = *this;
        return params;
    }
};

// video thread parameters
class MediaStreamParams : public Params
{
public:
    typedef enum _MediaStreamType {
        MediaStreamTypeAudio = 0,
        MediaStreamTypeVideo = 1
    } MediaStreamType;

    void write(std::ostream &os) const
    {
        os << "type: "
           << (type_==MediaStreamTypeVideo?
                   "video" : "audio")
           << "; name: " << streamName_;
    }

    MediaStreamType type_;
    std::string streamName_ = "";
private:

};

// general consumer parameters
class PublisherParams : public Params {
public:
    unsigned int interestLifetime_ = 0;
    unsigned int bufferSlotsNum_ = 0, slotSize_ = 0;
    unsigned int jitterSizeMs_ = 0;

    void write(std::ostream &os) const
    {
        os << "interest lifetime: " << interestLifetime_
        << "; jitter size: " << jitterSizeMs_ << "ms"
        << "; buffer size (slots): " << bufferSlotsNum_
        << "; slot size: " << slotSize_;
    }
};


// general app-wide parameters
class GeneralParams : public Params {
public:
    // general
    ndnlog::NdnLoggerDetailLevel loggingLevel_ = ndnlog::NdnLoggerDetailLevelAll;
    int         glogLevel_;
    std::string logFile_ = "";
    std::string logPath_ = "";
    std::string transType_ = "byFrame";

    bool    useTlv_ = false,
            useRtx_ = false,
            useFec_ = false,
            useCache_ = false,
            useAudio_ = false,
            useVideo_ = false,
            useAvSync_ = false,
            skipIncomplete_ = false;

    // network
    std::string host_ = "";
    unsigned int portNum_ = 0;
    std::string dev_ = "/dev/video0";

    void write(std::ostream &os) const
    {
        os
        << "; Host: " << host_
        << "; Port #: " << portNum_
        << "; Dev : " << dev_
        << "; Type : " << transType_
        << "; log level: " << 0
        << "; log file: " << logFile_
        << "; gloglevel: " << glogLevel_
        << "; TLV: " << (useTlv_?"ON":"OFF")
        << "; RTX: " << (useRtx_?"ON":"OFF")
        << "; FEC: " << (useFec_?"ON":"OFF")
        << "; Cache: " << (useCache_?"ON":"OFF")
        << "; Audio: " << (useAudio_?"ON":"OFF")
        << "; Video: " << (useVideo_?"ON":"OFF")
        << "; A/V Sync: " << (useAvSync_?"ON":"OFF")
        << "; Skipping incomplete frames: "
        << (skipIncomplete_?"ON":"OFF");
        //<< "; Prefix: " << prefix_;
    }
};


#endif // PARAMS_H
