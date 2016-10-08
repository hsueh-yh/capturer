#include <stdarg.h>
#include "namespacer.h"


// Name:  .../monitor/<location>/<streams>/video(audio)/<frameNumber>/<segmentNumber>/<PrefixMetaInfo>


shared_ptr<std::string>
Namespacer::buildPath(bool precede, const std::string *component1, ...)
{
    shared_ptr<std::string> path(new std::string(""));
        va_list ap;
        va_start(ap, component1);
        const std::string *s = component1;
        std::string delim = "/";

        if( precede )
            path.get()->append(delim);

        while (s)
        {
            path.get()->append(*s);

            s = va_arg(ap, std::string*);

            if (s)
                path.get()->append(delim);
        }

        va_end(ap);

        return path;
}

/**
  * @brief find component from prefix with componentString
  * @param prefix, name Prefix
  * @param componentString, component string may find in prefix
  * @return position of the componentString in prefix( 0 ~ prefix.size()-1 ), -1 means not found
  */
int
Namespacer::findComponent(const ndn::Name &prefix,
                                   const std::string &componentString)
{
    int pos = -1;
    int nComponents = prefix.size();
    Name::Component searchComponent((const uint8_t*)componentString.c_str(),
                                    componentString.size());

    for (int i = nComponents-1; i >= 0; i--)
    {
        Name::Component c = prefix.get(i);

        if (c == searchComponent)
        {
            pos = i;
            break;
        }
    }

    return pos;
}

boost::shared_ptr<std::string>
Namespacer::getLocationPrefix(const std::string &prefix,
                          const std::string &location)
{
    return buildPath(
                prefix[0] != '/',
                      &NameComponents::NameComponentApp,
                      &location);
}

boost::shared_ptr<std::string>
Namespacer::getStreamPath(const std::string &prefix,
                      const std::string &location,
                      const std::string &streamName)
{
    return buildPath( prefix[0] != '/',
                      &prefix,
                      &NameComponents::NameComponentApp,
                      &location,
                      &streamName);
}

boost::shared_ptr<std::string>
Namespacer::getStreamVideoPath(const std::string &prefix,
                      const std::string &location,
                      const std::string &streamName)
{
    return buildPath( prefix[0] != '/',
                      &prefix,
                      &NameComponents::NameComponentApp,
                      &location,
                      &streamName,
                      &NameComponents::NameComponentStreamFrameVideo);
}

boost::shared_ptr<std::string>
Namespacer::getStreamAudioPath(const std::string &prefix,
                      const std::string &location,
                      const std::string &streamName)
{
    return buildPath( prefix[0] != '/',
                      &prefix,
                      &NameComponents::NameComponentApp,
                      &location,
                      &streamName,
                      &NameComponents::NameComponentStreamFrameAudio);
}

void
Namespacer::getSegmentationNumbers(const ndn::Name &prefix,
                                        PacketNumber &packetNumber,
                                        SegmentNumber &segmentNumber)
{
    int p = -1;
    packetNumber = -1;
    segmentNumber = -1;

    p = findComponent(prefix, NameComponents::NameComponentStreamFrameVideo);

    if (p < 0)
        p = findComponent(prefix, NameComponents::NameComponentStreamFrameAudio);

    if (p > 0)
    {
        //the packet number is next to NameComponents::NameComponentStreamFrameVideo(Audio)
        if (p+1 < prefix.size())
        {
            Name::Component packetNoComp = prefix.get(p+1);
            packetNumber = NdnRtcUtils::frameNumber(packetNoComp);
        }

        if (p+2 < prefix.size())
        {
            Name::Component segmentNoComp = prefix.get(p+2);
            segmentNumber = segmentNoComp.toNumber();
            segmentNumber = NdnRtcUtils::segmentNumber(segmentNoComp);
        }
    }
}

void
Namespacer::getFrameNumber(const ndn::Name &prefix,
                                   PacketNumber &packetNumber)
{
    int p = -1;
    packetNumber = -1;
    p = findComponent(prefix, NameComponents::NameComponentStreamFrameVideo);

    if (p < 0)
        p = findComponent(prefix, NameComponents::NameComponentStreamFrameAudio);

    if (p > 0)
    {
        //the packet number is next to NameComponents::NameComponentStreamFrameVideo(Audio)
        if (p+1 < prefix.size())
        {
            Name::Component packetNoComp = prefix.get(p+1);
            packetNumber = NdnRtcUtils::frameNumber(packetNoComp);
        }
    }
}

void
Namespacer::getSegmentNumber(const ndn::Name &prefix,
                                   SegmentNumber &segmentNumber)
{
    int p = -1;
    segmentNumber = -1;

    p = findComponent(prefix, NameComponents::NameComponentStreamFrameVideo);

    if (p < 0)
        p = findComponent(prefix, NameComponents::NameComponentStreamFrameAudio);

    if (p > 0)
    {
        //the segment number is next two to NameComponents::NameComponentStreamFrameVideo(Audio)
        if (p+2 < prefix.size())
        {
            Name::Component segmentNoComp = prefix.get(p+2);
            segmentNumber = segmentNoComp.toNumber();
            segmentNumber = NdnRtcUtils::segmentNumber(segmentNoComp);
        }
    }
}

void
Namespacer::getPrefixMetaInfo(const ndn::Name &prefix,
                              PrefixMetaInfo& prefixMetaInfo)
{
    int p = -1;

    p = findComponent(prefix, NameComponents::NameComponentStreamFrameVideo);

    if (p < 0)
        p = findComponent(prefix, NameComponents::NameComponentStreamFrameAudio);

    if (p > 0)
    {
        //the prefixMetaInfo is in next three of NameComponents::NameComponentStreamFrameVideo(Audio)
        if (p+3 < prefix.size())
        {
            Name::Component packetNoComp = prefix.get(p+3);
            prefixMetaInfo.totalSegmentNum_ = NdnRtcUtils::intFromComponent(packetNoComp);
        }

        if (p+4 < prefix.size())
        {
            Name::Component segmentNoComp = prefix.get(p+4);
            prefixMetaInfo.playbackNo_ = NdnRtcUtils::intFromComponent(segmentNoComp);
        }
        if (p+5 < prefix.size())
        {
            Name::Component segmentNoComp = prefix.get(p+5);
            prefixMetaInfo.deltaFrameNo_ = NdnRtcUtils::intFromComponent(segmentNoComp);
        }
    }
}

void
Namespacer::getFramePrefix(const Name &prefix, Name& subPrefix)
{
    int p = -1;
    p = findComponent(prefix,NameComponents::NameComponentStreamFrameVideo);
    if ( p < 0 )
        p = findComponent(prefix,NameComponents::NameComponentStreamFrameAudio);
    if( p > 0 )
    {
        if( prefix.size() > p+2 )
            subPrefix = prefix.getSubName(0,p+2);
    }
}

boost::shared_ptr<std::string>
Namespacer::getSegmentPrefix(const Name &prefix, Name &subPrefix)
{
    int p = -1;
    p = findComponent(prefix,NameComponents::NameComponentStreamFrameVideo);
    if ( p < 0 )
        p = findComponent(prefix,NameComponents::NameComponentStreamFrameAudio);
    if( p > 0 )
    {
        if( prefix.size() > p+3 )
            subPrefix = prefix.getSubName(0,p+3);
    }
}
