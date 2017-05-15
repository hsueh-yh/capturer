#ifndef MANAGER_H_
#define MANAGER_H_

#include <string>

#include "interfaces.h"
#include "params.h"

class Manager {
public:
    static Manager &getSharedInstance();

    ~Manager();

    std::string
    addLocalStream(const GeneralParams &generalParams,
                   const PublisherParams &publisherParams,
                   const VideoCapturerParams captureParams,
                   const VideoCoderParams coderParams,
                   const MediaStreamParams &mediaStreamParams,
                   IExternalCapturer *const capturer);

    std::string
    removeLocalStream(const std::string streamPrefix);

    int
    removeAll();


private:
    Manager();
    Manager(Manager const&) = delete;
    void operator=(Manager const&) = delete;

    bool initialized_, failed_;
};

#endif //MANAGER_H_
