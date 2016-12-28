#ifndef MANAGER_H_
#define MANAGER_H_

#include <string>

#include "interfaces.h"
#include "params.h"

class Manager {
public:
    std::string
    addLocalStream(const MediaStreamParams& params,
                   IExternalCapturer** const capturer);

private:
    Manager();
    Manager(Manager const&) = delete;
    void operator=(Manager const&) = delete;
};

#endif //MANAGER_H_
