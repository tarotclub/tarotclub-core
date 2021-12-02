#ifndef I_SERVICE_H
#define I_SERVICE_H

#include <string>
#include <memory>
#include "Lobby.h"
#include "IServer.h"

class IService
{

public:
    ~IService() { /* Nothing to do */ }

    virtual std::string GetName() = 0;
    virtual void Initialize(std::shared_ptr<IServer> server, std::shared_ptr<Lobby> lobby) = 0;
    virtual void Stop() = 0;

};

#endif // I_SERVICE_H
