#ifndef NAMESPACER_H
#define NAMESPACER_H

// Name:  .../monitor/<location>/<streams>/video(audio)/<frameNumber>/<segmentNumber>/<PrefixMetaInfo>

#include <string>
#include <ndn-cpp/name.hpp>

#include "name-components.h"
#include "frame-data.h"
#include "common.h"
#include "utils.h"

class Namespacer {
public:

    static boost::shared_ptr<std::string>
        buildPath(bool precede, const std::string *component1, ...);

    static int findComponent(const Name &prefix,
                             const std::string &component);

    //*****************************************************
    //  get prefix with string
    //*****************************************************

    static boost::shared_ptr<std::string>
        getLocationPrefix(const std::string &prefix,
                          const std::string &location);

    static boost::shared_ptr<std::string>
        getStreamPath(const std::string &prefix,
                      const std::string &location,
                      const std::string &streamName);

    static boost::shared_ptr<std::string>
        getStreamVideoPath(const std::string &prefix,
                           const std::string &location,
                           const std::string &streamName);

    static boost::shared_ptr<std::string>
        getStreamAudioPath(const std::string &prefix,
                           const std::string &location,
                           const std::string &streamName);

    static void
        getFramePrefix(const Name &prefix, Name &subPrefix);

    static boost::shared_ptr<std::string>
        getSegmentPrefix(const Name &prefix, Name &subPrefix);

    //*****************************************************
    //  get name component with corresponding struct
    //*****************************************************

    static void getSegmentationNumbers(const ndn::Name &prefix,
                                       PacketNumber &packetNumber,
                                       SegmentNumber &segmentNumber);

    static void getFrameNumber(const ndn::Name &prefix,
                                       PacketNumber &packetNumber);

    static void getSegmentNumber(const ndn::Name &prefix,
                                       SegmentNumber &segmentNumber);

    static void getPrefixMetaInfo(const ndn::Name &prefix,
                                  PrefixMetaInfo &prefixMetaInfo);

};

#endif // NAMESPACER_H
