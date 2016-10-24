#include "name-components.h"
#include "namespacer.h"
#include "utils.h"

// Name:  .../monitor/<location>/<streams>/video(audio)/<frameNumber>/<segmentNumber>/<PrefixMetaInfo>

//const std::string NameComponents::NameComponentGlobal = "com";
const std::string NameComponents::NameComponentApp = "monitor";
const std::string NameComponents::NameComponentStreamFrameVideo = "video";
const std::string NameComponents::NameComponentStreamFrameAudio = "audio";
const std::string NameComponents::NameComponentStreamMetainfo = "metainfo";
//const std::string NameComponents::KeyComponent;
//const std::string NameComponents::CertificateComponent;

std::string
NameComponents::getLocationPrefix(const std::string& location,
                  const std::string& prefix)
{
    return *Namespacer::buildPath(prefix[0] != '/',
            &prefix,
            &NameComponents::NameComponentApp,
            &location,
            NULL);
}

std::string
NameComponents::getStreamPrefix(const std::string& streamName,
                                const std::string& location,
                                const std::string& prefix)
{
    return *Namespacer::buildPath(prefix[0] != '/',
            &prefix,
            &NameComponents::NameComponentApp,
            &location,
            &streamName,
            NULL);
}

std::string
NameComponents::getStreamVideoPrefix(const std::string& streamName,
                const std::string& location,
                const std::string& prefix)
{
    return *Namespacer::buildPath(prefix[0] != '/',
            &prefix,
            &NameComponents::NameComponentApp,
            &location,
            &streamName,
            &NameComponents::getStreamVideoPrefix,
            NULL);
}

std::string
NameComponents::getStreamAudioPrefix(const std::string& streamName,
                const std::string& location,
                const std::string& prefix)
{
    return *Namespacer::buildPath(prefix[0] != '/',
            &prefix,
            &NameComponents::NameComponentApp,
            &location,
            &streamName,
            &NameComponents::getStreamAudioPrefix,
            NULL);
}
